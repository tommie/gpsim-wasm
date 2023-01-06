/*
   Copyright (C) 1998 T. Scott Dattalo

This file is part of gpsim.

gpsim is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

gpsim is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with gpsim; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include <string>
#include <list>
#include <csignal>
#include <iostream>

#ifdef _WIN32
#include <direct.h>
#include "fd2raw.h"
#include "../src/processor.h"
#define GETCWD _getcwd

#else
#include <sys/file.h>
#include <unistd.h>

#if !defined(_MAX_PATH)
#define _MAX_PATH 1024
#endif

#define GETCWD getcwd
// temp fix to over come 23jan05 changes to configure script that prevent
// readline from being found on older systems.
#define HAVE_READLINE
#define HAVE_NSCLEAN_READLINE

#endif

#include "../config.h"

#ifdef HAVE_GUI
#include <glib.h>
#include <gdk/gdktypes.h>
#include <gdk/gdk.h>
#endif

#include "../src/exports.h"
#include "../src/sim_context.h"
#include "../src/gpsim_interface.h"
#include "../src/icd.h"
#include "../src/ui.h"
#include "../src/value.h"

#include "command.h"
#include "input.h"
#include "cmd_macro.h"


#ifdef HAVE_LIBREADLINE
#define HAVE_READLINE
#endif

#ifdef HAVE_READLINE
/* See if we have namespace-clean readline or not */
#ifdef HAVE_RL_COMPLETION_MATCHES
#define HAVE_NSCLEAN_READLINE
#endif

extern "C" {
#include <readline/readline.h>
#include <readline/history.h>
}

static GIOChannel *channel;
gint g_iWatchSourceID = 0;

#endif /* HAVE_READLINE */


void simulation_cleanup();

const char *get_dir_delim(const char *path);
int parse_string(const char * str);

//------------------------------------------------------------------------
//
LIBGPSIM_EXPORT bool gUsingThreads(); // in ../src/interface.cWatchSourceIDc

void initialize_readline();
void clear_input_buffer();

#ifdef HAVE_GUI
void quit_gui();
#endif

void redisplay_prompt();

char *gnu_readline(char *s, unsigned int force_readline);

int last_command_is_repeatable = 0;
//extern Macro *gCurrentMacro;
extern void  scanPushMacroState(Macro *);

extern int quit_parse;

#ifdef HAVE_SOCKETS
extern void start_server();
extern void stop_server();
#endif // HAVE_SOCKETS


static Boolean *s_bSTCEcho = nullptr;

void EnableSTCEcho(bool bEnable)
{
  *s_bSTCEcho = bEnable;
}


//------------------------------------------------------------------------
// Command Handler - create an interface to the CLI
//------------------------------------------------------------------------
#include "../src/cmd_manager.h"

class CCliCommandHandler : public ICommandHandler {
public:
  const char *GetName() override;
  int Execute(const char * commandline, ISimConsole *out) override;
  int ExecuteScript(const std::list<std::string> &script, ISimConsole *out) override;
};


// This instantiation will get registered so that code
//  in ../src can get access to this class (and consequently the command line).

static CCliCommandHandler sCliCommandHandler;


//------------------------------------------------------------------------
// LLInput - A class for storing a command line command
// This is private to input.cc
//------------------------------------------------------------------------

class LLInput {
public:
  LLInput();
  LLInput(const char *, Macro *);
  ~LLInput();

  Macro *macro;  // macro generating this text
  std::string data;
  LLInput *next_input;
};


//------------------------------------------------------------------------
// LLStack - A class for storing a collection of command line commands.
// This is private to input.cc
//------------------------------------------------------------------------
class LLStack {
public:
  LLStack();
  ~LLStack();

  void Push();
  void Pop();
  void Append(const char *, Macro *);
  LLInput *GetNext();

  void print();
  int  level()
  {
    return msi_StackDepth;
  }

  LLInput *LLdata;
  LLStack *next_stack;

private:
  static int msi_StackDepth;
};


int LLStack::msi_StackDepth = 0;

//------------------------------------------------------------------------
// LLInput - linked list input for commands.

LLInput::LLInput()
  : macro(nullptr), next_input(nullptr)
{
}


LLInput::LLInput(const char *s, Macro *m)
  : macro(m), data(s), next_input(nullptr)
{
}


LLInput::~LLInput()
{
}


//------------------------------------------------------------------------
// LLStack - linked list stack for commands.

LLStack::LLStack()
  : LLdata(nullptr), next_stack(nullptr)
{
  msi_StackDepth++;
  //  cout << "Stack depth: " << level() << endl;
}


LLStack::~LLStack()
{
  msi_StackDepth--;
}


static LLStack *Stack = nullptr;

//========================================================================
//
// catch_control_c
//
//
void catch_control_c(int /* sig */ )
{
#ifdef _WIN32
  CSimulationContext::GetContext()->NotifyUserCanceled();

  if (CSimulationContext::GetContext()->GetActiveCPU()->simulation_mode
      == eSM_RUNNING) {
    // If we get a CTRL->C while processing a command file
    // we should probably stop the command file processing.
    clear_input_buffer();
  }

  ::signal(SIGINT, catch_control_c);
#else
  // JRH - I'll let someone else try the above code under Linux.
  // The readline library appears to call ::signal() itself so
  // the call will not be needed here. I'm guessing that the CTRL->C
  // signal handling in readline did not work under Windows so
  // that is why it was originally ifdef'd out.
  //if(simulation_mode != STOPPED)
  //  {
  std::cout << "<CTRL C> break\n";
  CSimulationContext::GetContext()->NotifyUserCanceled();
  //  }
  //else {
  //  cout << "caught control c, but it doesn't seem gpsim was simulating\n";
  //  last_command_is_repeatable=0;
  //  redisplay_prompt();
  //}
#endif
}


void initialize_threads()
{
  if (!g_thread_supported()) {
#if GLIB_MINOR_VERSION < 32
    g_thread_init(nullptr);
#endif
#ifdef HAVE_GUI
    gdk_threads_init();
#endif
  }
}


void initialize_signals()
{
#ifdef _WIN32
  ::signal(SIGINT, catch_control_c);
#else
  static struct sigaction action;
  action.sa_handler = catch_control_c;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;
  sigaction(SIGINT, &action, 0);
#endif
}


void initialize_CLI()
{
  CCommandManager::GetManager().Register(&sCliCommandHandler);
}


//==============================================================
// initialize_gpsim
//
// Not much initialization is needed now.
//

void initialize_gpsim()
{
  s_bSTCEcho = new Boolean("CliTrace", false,
                           "Enable echoing commands from STC files to the console.");
  globalSymbolTable().addSymbol(s_bSTCEcho);
  initialize_CLI();

  if (gUsingThreads()) {
    initialize_threads();
  }

  initialize_signals();
#ifdef HAVE_SOCKETS
  start_server();
#endif
}


void LLStack::print()
{
  if (verbose & 4) {
    LLStack *s = Stack;
    std::cout << "Current state of input buffer:\n";
    int stack_number = 0;

    while (s) {
      LLInput *h = s->LLdata;
      int depth = 0;

      while (h) {
        std::cout << "   " << stack_number << ':' << depth << "  " <<  h->data;
        depth++;
        h = h->next_input;
      }

      stack_number++;
      s = s->next_stack;
    }

    std::cout << "\n ---Leaving dump \n";
  }
}


void LLStack::Push()
{
  LLStack *s = new LLStack();
  s->next_stack = Stack;
  Stack = s;
  print();
}


void LLStack::Pop()
{
  if (Stack) {
    if (Stack->next_stack != nullptr) {
      LLStack *next = Stack->next_stack;
      delete Stack;
      Stack = next;
    }
  }
}


void LLStack::Append(const char *s, Macro *m)
{
  LLInput *d = new LLInput(s, m);
  LLInput *h = LLdata;

  if (h) {
    /* go to the end of the list */
    while (h->next_input) {
      h = h->next_input;
    }

    /* add d to the end */
    h->next_input = d;

  } else {
    LLdata = d;
  }
}


LLInput *LLStack::GetNext()
{
  if (Stack) {
    if (Stack->LLdata) {
      LLInput *d = Stack->LLdata;

      // remove this item from the list
      if (d) {
        Stack->LLdata = d->next_input;
      }

      return d;
    }

    if (Stack->next_stack != nullptr) {
      Pop();
      return GetNext();
    }
  }

  return nullptr;
}


/*******************************************************
 */
void add_string_to_input_buffer(const char *s, Macro *m = nullptr)
{
  if (!Stack) {
    Stack = new LLStack();
  }

  Stack->Append(s, m);
}


/*******************************************************
 */
void start_new_input_stream()
{
  if (!Stack) {
    Stack = new LLStack();

  } else {
    Stack->Push();
  }
}


/*******************************************************
 */
void clear_input_buffer()
{
  if (Stack) {
    LLInput *pLine;

    while ((pLine = Stack->GetNext()) != nullptr) {
      delete pLine;
    }
  }
}


/*******************************************************
 * start_parse
 *
 * This routine will run a string through the command parser
 *this is useful if you want to execute a command but do not
 *wish to go through the readline stuff.
 */
int init_parser();

int start_parse()
{
  int retval = init_parser();

  if (quit_parse) {
    exit_gpsim(0);
  }

  return retval;
}


int parse_string(const char *str)
{
  add_string_to_input_buffer(str);
  int iReturn = start_parse();

  if (iReturn == 1) {
    // If the str was a 'load c x.stc' file and the parsing
    // was aborted we need to remove the remaining
    // strings.
    clear_input_buffer();
  }

  return iReturn;
}


int parse_string_only(const char * str)
{
  LLStack *OldStack = Stack;
  Stack = nullptr;
  int iRet = parse_string(str);
  delete Stack;
  Stack = OldStack;
  return iRet;
}


void process_command_file(const char * file_name, bool bCanChangeDirectory)
{
  FILE *cmd_file;
  const char *dir_path_end;

  if (verbose & 4) {
    std::cout << __FUNCTION__ << "()\n";
  }

  dir_path_end = get_dir_delim(file_name);

  if (dir_path_end && bCanChangeDirectory) {
    char directory[256];

    strncpy(directory, file_name, dir_path_end - file_name);
    directory[dir_path_end - file_name] = 0;
    printf("directory is \"%s\"\n", directory);

    if (chdir(directory) < 0) {
      perror(directory);
    }

    file_name = dir_path_end + 1;
    printf("file_name is \"%s\"\n", file_name);
  }

  cmd_file = fopen(file_name, "r");

  if (cmd_file) {
    if (verbose) {
      std::cout << "processing a command file\n";
    }

    start_new_input_stream();
    //Stack.Push();
    char *s;
    char str[256];

    while ((s = fgets(str, 256, cmd_file)) != 0) {
      if (str[0] == 0 ||
          str[0] == '\n' ||
          ((str[1] == '\n' && str[0] == '\r'))) {
        // skip the blank lines
        continue;
      }

#ifndef WIN32
      // Let us be compatible with Windows EOLs
      // on Linux.
      int iLast = strlen(str) - 1;

      if (iLast >= 2 && str[iLast] == '\n' && str[iLast - 1] == '\r') {
        // Windows type EOL
        // convert <CR><LF> to <LF>
        str[iLast] = '\000';
        str[iLast - 1] = '\n';
      }

#endif
      add_string_to_input_buffer(s);
    }

    fclose(cmd_file);

  } else {
    std::cout << "failed to open command file " << file_name << '\n';
    char cw[_MAX_PATH];

    if (GETCWD(cw, _MAX_PATH) == 0) {
      perror("getcwd()");

    } else {
      std::cout << "current working directory is " << cw << '\n';
    }
  }

  if (Stack) {
    Stack->print();
  }
}


/*********************************************
  Function: gpsim_open()

  Returns:  1   - success
            0   - failure
*/

int gpsim_open(Processor * , const char *pFileName,
               const char * pProcessorType, const char *pProcessorName)
{
  if (!pFileName) {
    return 0;
  }

  if (verbose) {
    printf(" gpsim_open file:%s proc name:%s\n", pFileName, (pProcessorName ? pProcessorName : "nil"));
  }

  // Check for the command file, file extension.
  if (IsFileExtension(pFileName, "stc") || IsFileExtension(pFileName, "STC")) {
    process_command_file(pFileName, true);
    // A stc file could have any sequence of commands.
    // Just ignore the return value of parse_string().
    parse_string("\n");
    return 1;

  } else if (IsFileExtension(pFileName, "asm")) {
    fprintf(stderr, "File %s has extension .asm, not a suitable file\n",
            pFileName);
    return 0;

  } else {
    // Assume a Program file
    return
      CSimulationContext::GetContext()->LoadProgram(pFileName, pProcessorType, 0, pProcessorName);
  }

  return 0;
}


//*********************************************
// gpsim_read
//
//  This function is called from the parser. It will either read
// a line of text from a command file or from stdin -- depending
// on the current source for data. Once a string is obtained, it
// is copied into a buffer that the parser has passed to us.

int gpsim_read(char *buf, unsigned int max_size)
{
  LLInput *d = Stack ? Stack->GetNext() : nullptr;

  if (Stack && verbose) {
    Stack->print();
  }

  if (!d || d->data.empty()) {
    if (verbose & 4) {
      std::cout << "gpsim_read -- no more data\n";
    }

    return 0;
  }

  scanPushMacroState(d->macro);
  //gCurrentMacro = d->macro;
  const std::string &str = d->data;
  unsigned int count = str.size();
  count = (count < max_size) ? count : max_size - 1;
  strncpy(buf, str.c_str(), count);
  buf[count] = 0;
  SetLastFullCommand(buf);

  if (*s_bSTCEcho) {
    std::cout << str;
  }

  if (verbose & 4) {
    std::cout << "gpsim_read returning " << count << ':' << str << '\n';

    if (d->macro) {
      std::cout << "   and it's a macro named:" << d->macro->name() << '\n';
    }
  }

  delete d;
  return count;
}


void have_line(char *);


//**************************************************
void cli_main()
{
  do {
#ifdef HAVE_READLINE
#ifdef WIN32

    if (channel->is_readable) {
      // Channel is not readable when a Windows
      // stdin is redirected.
      rl_callback_read_char();

    } else {
      char line[256];
      fgets(line, sizeof(line), stdin);
      have_line(line);
    }

#else
    rl_callback_read_char();
#endif
#else
    char line[256];
    fgets(line, sizeof(line), stdin);
    have_line(line);
#endif
  } while (!quit_parse);
}


/* **************************************************************** */
/*                                                                  */
/*                  Interface to Readline Completion                */
/*                                                                  */
/* **************************************************************** */

/* Generator function for command completion.  STATE lets us know whether
   to start from scratch; without any state (i.e. STATE == 0), then we
   start at the top of the list. */
char *command_generator(const char *text, int state)
{
  //static char *empty="";
  static int i = 0;
  const int cMaxStringLen = 64;

  /* If this is a new word to complete, initialize now.  */
  if (state == 0) {
    i = 0;
  }

  /* Return the next name which partially matches from the command list. */
  while (i < number_of_commands) {
    if (strstr(command_list[i]->name(), text) == command_list[i]->name()) {
      return (g_strndup(command_list[i++]->name(), cMaxStringLen));
    }

    i++;
  }

  // If no names matched, and this is the first item on a line (i.e. state==0)
  // then return a copy of the input text. (Note, it was emperically determined
  // that 'something' must be returned if there are no matches at all -
  // otherwise readline crashes on windows.)
#ifdef _WIN32

  if (state == 0) {
    return g_strndup(text, cMaxStringLen);
  }

#endif
  return nullptr;
}


/* Attempt to complete on the contents of TEXT.  START and END show the
   region of TEXT that contains the word to complete.  We can use the
   entire line in case we want to do some simple parsing.  Return the
   array of matches, or NULL if there aren't any. */
char **gpsim_completion(const char *text, int start, int /* end */ )
{
  char **matches = nullptr;

#ifdef HAVE_READLINE

  /* If this word is at the start of the line, then it is a command
     to complete.  Otherwise it is the name of a file in the current
     directory. */
  if (start == 0)
#ifdef HAVE_NSCLEAN_READLINE
    matches = rl_completion_matches(text, command_generator);

#else
    matches = completion_matches((char *)text, (CPFunction *)command_generator);
#endif
#endif
#if defined(_WIN32) || defined(WIN32)

  if (start) {
    char *empty = strdup("");
    matches = (char **) malloc(2 * (sizeof(&empty)));
    matches[0] = empty;
    matches[1] = 0;
  }

#endif
  return matches;
}


//#ifdef HAVE_GUI

//============================================================================
//
// keypressed().
//
// When the user presses a key, this function will get called.
// If a simulation is running, then we'll pass the key to
// the stimulus engine.

static gboolean keypressed(GIOChannel * /* source */ , GIOCondition /* condition */ , gpointer /* data */ )
{
#ifdef HAVE_READLINE
  rl_callback_read_char();
#endif
  return TRUE;
}
//#endif


//============================================================================
//
// have_line
// When <cr> is pressed at the command line, the text string on the command
// is copied into a buffer and passed to the command parser.
//

void have_line(char *s)
{
  if (!s) {
    return;
  }

  static char last_line[256] = {0};

  if (strlen(s) == 0) {
    if (*last_line && last_command_is_repeatable) {
      add_string_to_input_buffer(last_line);
    }

  } else {
    // save a copy in the history buffer:
    strncpy(last_line, s, 255);
#ifdef HAVE_READLINE
    add_history(s);
#endif
    add_string_to_input_buffer(s);
  }

  add_string_to_input_buffer("\n");
  start_parse();
#if defined (_WIN32) || defined (HAS_RL_FREE)
  rl_free(s);
#else
  free(s);
#endif
}


/**********************************************************************
 **/
void exit_cli()
{
  if (get_use_icd()) {
    icd_disconnect();
  }

#ifdef HAVE_GUI
  quit_gui();
#endif
#ifdef HAVE_READLINE
  rl_callback_handler_remove();
#ifdef HAVE_GUI
  g_source_remove(g_iWatchSourceID);
  g_io_channel_unref(channel);
#endif
#endif
  CSimulationContext::GetContext()->GetContext()->Clear();
#ifdef HAVE_SOCKETS
  stop_server();
#endif
  globalSymbolTable().deleteSymbol("CliTrace");
  std::cout << "Exiting gpsim\n";
  simulation_cleanup();
}


/* redisplay_prompt will redisplay the current data in the readline buffer.
   This function is used to restore a prompt that's been obliterated by diagnostic
   data.
*/

void redisplay_prompt()
{
#ifdef HAVE_READLINE
  rl_forced_update_display();
#endif
}


#if defined HAVE_READLINE && defined HAVE_GUI
static int gpsim_rl_getc(FILE * /* in */ )
{
  gchar buf[6];
  gsize bytes_read;
  g_io_channel_read_chars(channel, buf, 1, &bytes_read, nullptr);
  return buf[0];
  // JRH, 7-1-2005 - I tried this but am not comfortable that it
  // is working as I intended.
  // return bytes_read == 0 ? EOF : (buf[0] & 0x000000ff);
}


#endif

/* Tell the GNU Readline library how to complete.  We want to try to complete
   on command names if this is the first word in the line, or on filenames
   if not. */
void initialize_readline()
{
  const char *gpsim_prompt = "gpsim> ";
  const char *gpsim_cli_prompt = "**gpsim> ";
  const char *prompt = get_interface().bUsingGUI() ? gpsim_prompt : gpsim_cli_prompt;
#ifdef HAVE_READLINE
  // Lets us have a gpsim section to .inputrc
  // JRH - not tested
  //  rl_terminal_name = "gpsim";
#ifdef _WIN32
  /* set console to raw mode */
  win32_fd_to_raw(fileno(stdin));
#endif
#if defined HAVE_READLINE && defined HAVE_GUI
  rl_getc_function = gpsim_rl_getc;
  channel = g_io_channel_unix_new(fileno(stdin));
#endif
#ifdef _WIN32

  // The channel is not readable if it is a redirected
  // standard input, in which case we will not be getting
  // any keypress events.
  if (channel->is_readable) {
    g_iWatchSourceID = g_io_add_watch(channel, G_IO_IN, keypressed, nullptr);
  }

#else
#if defined HAVE_READLINE && defined HAVE_GUI
  g_iWatchSourceID = g_io_add_watch(channel, G_IO_IN, keypressed, nullptr);
#endif
#endif
  rl_callback_handler_install(prompt, have_line);
  /* Tell the completer that we want a crack first. */
  rl_attempted_completion_function = gpsim_completion;
#endif //HAVE_READLINE
}


#if defined(HAVE_READLINE) && defined(HAVE_PERL)
// JRH - An experiment
void EnableKeypressHook(bool bEnable)
{
  if (bEnable) {
    g_iWatchSourceID = g_io_add_watch(channel, G_IO_IN, keypressed, nullptr);

  } else {
    //    g_source_remove_by_funcs_user_data(keypressed, NULL);
    g_source_remove(g_iWatchSourceID);
    g_io_channel_unref(channel);
  }
}
#endif


//------------------------------------------------------------------------
// CLI command handler
//
// The command handler is an interface that gets 'registered' with gpsim.
// This means that clients interested in gpsim's cli can look up this
// registered handler and get access to the command line. This is primarily
// used by symbol files that embed gpsim scripts. See src/modules.cc.
//

const char *CCliCommandHandler::GetName()
{
  return "gpsimCLI";
}


int CCliCommandHandler::Execute(const char *commandline, ISimConsole * /* out */ )
{
  add_string_to_input_buffer("\n");
  start_new_input_stream();
  parse_string_only(commandline);
  add_string_to_input_buffer("\n");
  return 1;
}


int CCliCommandHandler::ExecuteScript(const std::list<std::string> &script, ISimConsole * /* out */ )
{
  if (verbose & 4) {
    std::cout << "GCLICommandHandler::Execute Script:\n";
  }

  if (script.empty()) {
    return CMD_ERR_OK;
  }

  // We need to execute the script now. There may be other commands
  // currently pending, so a new command stream is created and the
  // commands that are in this script are placed there. The current
  // command stream is temporarily disabled and then re-enabled at
  // the end of this function.
  LLStack *saveStack = Stack;
  Stack = nullptr;
  start_new_input_stream();
  add_string_to_input_buffer("\n");

  for (const auto &cmd : script) {
    add_string_to_input_buffer(cmd.c_str());
  }

  // Start parsing the script that we just placed into the command stream
  start_parse();
  delete Stack;
  // Restore the original command stream.
  Stack = saveStack;
  return CMD_ERR_OK;
}
