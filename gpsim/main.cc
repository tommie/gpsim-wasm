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



// Portions of this files are (C) by Ian King:

/* picdis.c  - pic disassembler         */
/* version 0.1                          */
/* (c) I.King 1994                      */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <iostream>

#include "../config.h"
#include "gpsim.h"
#include "../cli/input.h"
#include "../src/interface.h"
#include "../src/gpsim_interface.h"
#include "../src/fopen-path.h"
#include "../src/breakpoints.h"
#include "../src/errors.h"
#include "../cli/ui_gpsim.h"
#include "../src/value.h"

int quit_state;

extern "C" {
#include <popt.h>
}

extern int gui_init(int argc, char **argv);
extern void gui_main();
extern void cli_main();
// os_dependent.cc'
extern void AddModulePathFromFilePath(char *arg);

void initialize_gpsim();

int parse_string(const char *cmd_string);
extern void initialize_commands();

extern int yydebug;
extern int quit_parse;
extern int abort_gpsim;

#if _DEBUG
char szBuild[] = "Debug";
#else
char szBuild[] = "Release";
#endif

void gpsim_version()
{
  printf("%s %s\n", szBuild, VERSION);
}


// from ui_gpsim.cc
void initialize_ConsoleUI();

#define FILE_STRING_LENGTH 50

//----------------------------------------------------------
// Here are the variables that popt (the command line invocation parsing
// library) will assign values to:

static const char *startup_name = nullptr;
static const char *include_startup_name = nullptr;
static const char *processor_name = nullptr;
static const char *cod_name     = nullptr;
static const char *search_path  = nullptr;
static const char *icd_port     = nullptr;
static const char *defineSymbol = nullptr;
static const char *sExitOn      = nullptr;
static const char *sourceEnabled = nullptr;


struct poptOption myHelpOptions[] = {
  POPT_TABLEEND
};

#define POPT_MYEXAMPLES { NULL, '\0', POPT_ARG_INCLUDE_TABLE, myHelpOptions, \
                        0, "Examples:\n\
  gpsim  myprog.cod                    <-- loads a symbol file\n\
  gpsim -p p16f877 myprog.hex          <-- select processor and load hex\n\
  gpsim  myscript.stc                  <-- loads a script\n", NULL },

//------------------------------------------------------------------------
// see popt documentation about how the poptOption structure is defined.
//  In general, we define legal invocation arguments in this structure.
// Both the long name (like --processor) and the short name (like -p)
// are defined. The popt library will assign values to variables if
// a pointer to a variable is supplied. Some options like the 'echo'
// option have no associated varaible. After the variable name, the
// poptOption structure contains a 'val' field. If this is 0 then popt
// will assign an option to a variable when poptGetNextOpt is called.
// Otherwise, poptGetNextOpt will return the value of 'val' and allow
// gpsim to interpret and further parse the option.
struct poptOption optionsTable[] = {
  {
    "cli", 'i', POPT_ARG_NONE, nullptr, 'i',
    "command line mode only", nullptr
  },
  {
    "command", 'c', POPT_ARG_STRING, &startup_name, 0,
    "startup command file (-c optional)", nullptr
  },
  {
    "define", 'D', POPT_ARG_STRING, &defineSymbol, 'D',
    "define symbol with value that is added to the gpsim symbol table. "
    "Define any number of symbols.", nullptr
  },
  {
    "echo", 'E', POPT_ARG_NONE, nullptr, 'E',
    "Echo lines from a command file to the console.", nullptr
  },
  {
    "help", 'h', 0, nullptr, 'h',
    "display this help and exit", nullptr
  },
  {
    "icd", 'd', POPT_ARG_STRING, &icd_port, 0,
    "use ICD (e.g. -d /dev/ttyS0).", nullptr
  },
  {
    "include", 'I', POPT_ARG_STRING, &include_startup_name, 0,
    "startup command file - does not change directories", nullptr
  },
  {
    "processor", 'p', POPT_ARG_STRING, &processor_name, 0,
    "processor (e.g. -pp16c84 for the 'c84)", "<processor name>"
  },
  {
    "source", 'S', POPT_ARG_STRING, &sourceEnabled, 'S',
    "'enable' or 'disable' the loading of source code. Default is 'enable'. "
    "Useful for running faster regression tests.", nullptr
  },
  {
    "sourcepath", 'L', POPT_ARG_STRING, &search_path, 'L',
    "colon separated list of directories to search.", nullptr
  },
  {
    "symbol", 's', POPT_ARG_STRING, &cod_name, 0,
    ".cod symbol file (-s optional)", nullptr
  },
  {
    "version", 'v', 0, nullptr, 'v',
    "gpsim version", nullptr
  },
  /* POPT_AUTOHELP generates
    error: invalid conversion from `const void*' to `void*' [-fpermis sive]
    on i686-pc-mingw32-c++ (GCC) 4.7.3 */
  {
    nullptr, '\0', POPT_ARG_INCLUDE_TABLE, poptHelpOptions, \
    0, "Help options:", nullptr
  },
  POPT_MYEXAMPLES
  POPT_TABLEEND
};


String *scope[8];
bool bUseGUI = true;    // assume that we want to use the gui

void welcome()
{
  printf("\ngpsim - the GNUPIC simulator\nversion: %s %s\n"
         "\n\ntype help for help\n", szBuild, VERSION);
}


void exit_gpsim(int ret)
{
  exit_cli();
#ifdef HAVE_GUI

  if (!bUseGUI)
#endif
  {
    for (int i = 0; i < 8; i++) {
      delete scope[i];
    }
  }

  exit(ret);
}


int main(int argc, char *argv[])
{
  bool bEcho = false;
  bool bSourceEnabled = true;
  int usage = 0;
  char command_str[256];
  char *hex_name = nullptr;
  poptContext optCon;     // context for parsing command-line options
  // Perform basic initialization before parsing invocation arguments
  InitSourceSearchAsSymbol();
  initialize_ConsoleUI();
  initialize_gpsim_core();
  initialize_gpsim();
  initialize_commands();
  // If last arg ends in unprintable character such as \r, strip it
  int len = strlen(argv[argc - 1]);

  if (!isprint(*(argv[argc - 1] + len - 1))) {
    *(argv[argc - 1] + len - 1) = 0;
  }

  optCon = poptGetContext(0, argc, (const char **)argv, optionsTable, 0);

  if (argc >= 2) {
    int c;
    while ((c = poptGetNextOpt(optCon)) >= 0  && !usage) {
      switch (c) {
      default:
        printf("'%c' is an unrecognized option\n", c);

      case '?':
      case 'h':
        usage = 1;
        break;

      case 'L':
        set_search_path(search_path);
#ifndef _WIN32
        free((char *)search_path);
#endif
        break;

      case 'd':
        printf("Use ICD with serial port \"%s\".\n", icd_port);
        break;

      case 'v':
        fprintf(stderr, "%s\n", GPSIM_VERSION_STRING);
        return 0;
        break;

      case 'i':
        bUseGUI = false;
        printf("not using gui\n");
        break;

      case 'D':
        // add symbols defined with '-D' to the symbol table.
        snprintf(command_str, sizeof(command_str),
                 "symbol %s\n", defineSymbol);
        parse_string(command_str);
#ifndef _WIN32
        free((char *)defineSymbol);
#endif
        break;

      case 'S':
        if (strcmp(sourceEnabled, "enable") == 0) {
          bSourceEnabled = true;

        } else if (strcmp(sourceEnabled, "disable") == 0) {
          bSourceEnabled = false;

        } else {
          usage = 1;
        }

#ifndef _WIN32
        free((char *)sourceEnabled);
#endif
        break;

      case 'E':
        bEcho = true;
        EnableSTCEcho(true);
        break;

      case 'e':
        if (sExitOn && strcmp(sExitOn, "onbreak") == 0) {
          get_bp().EnableExitOnBreak(true);

        } else {
          printf("%s is invalid exit condition for -e option.\n", sExitOn);
        }

#ifndef _WIN32
        free((char *)sExitOn);
#endif
        break;
      }

      if (usage) {
        break;
      }
    }

    if (c < -1) {
      /* an error occurred during option processing */
      fprintf(stderr, "Gpsim %s: %s\n",
              poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
              poptStrerror(c));
      usage = 1;
    }
  }

  if (usage) {
    poptPrintHelp(optCon, stdout, 0);
    exit(1);
  }

  welcome();

  if (bEcho) {
    for (int index = 0; index < argc; index++) {
      printf("%s ", argv[index]);
    }

    printf("\n");
  }

  if (poptPeekArg(optCon)) { // unprocessed argument, does not have to be a hex file
    hex_name = strdup(poptGetArg(optCon));

    if (strstr(hex_name, ".cod") && cod_name == nullptr) {
      cod_name = hex_name;
      hex_name = nullptr;
    }
  }

  if (poptPeekArg(optCon)) { // unexpected unprocessed argument
    char *arg;
    fprintf(stderr, "Gpsim Unexpected arguments not used: \"");

    while ((arg = (char *)poptGetArg(optCon))) {
      fprintf(stderr, "%s ", arg);
    }

    fprintf(stderr, "\"\n");
  }

  poptFreeContext(optCon);
  initialize_readline();
  // must be done after initialize_gpsim_core()
  Boolean *bEnableSourceLoad = dynamic_cast<Boolean*>(globalSymbolTable().find("EnableSourceLoad"));

  if (bEnableSourceLoad) {
    *bEnableSourceLoad = bSourceEnabled;
  }

  // initialize the gui
#ifdef HAVE_GUI

  if (bUseGUI) {
    if (gui_init(argc, argv) != 0) {
      std::cerr << "Error initialising GUI, reverting to cmd-line mode.\n";
      bUseGUI = false;
    }

    // Move this from above to accurately report whether the GUI
    // has initialized. With out this, gpsim would generate a segmentation
    // fault on exit under Linux when executed in a telnet session
    // and with out the -i option.
    get_interface().setGUImode(bUseGUI);

  } else
#endif
  {
    // so scope setup does not cause error when -i is used
    for (int i = 0; i < 8; i++) {
      char buf[10];
      snprintf(buf, sizeof(buf), "scope.ch%d", i);
      scope[i] = new String(buf, "");
      globalSymbolTable().addSymbol(scope[i]);
    }
  }

  AddModulePathFromFilePath(argv[0]);
  initialization_is_complete();
  yydebug = 0;
  quit_parse = 0;
  abort_gpsim = 0;

  try {
    // Convert the remaining command line options into gpsim commands
    if (cod_name) {
      if (processor_name)
        std::cout << "WARNING: command line processor named \"" << processor_name <<
             "\" is being ignored\nsince the .cod file specifies the processor\n";

      if (hex_name) {
        std::cout << "WARNING: Ignoring the file \"" << hex_name << "\" ";
        std::cout <<  "since \"" << cod_name << "\" already specified\n";
        free((void *)hex_name);
        hex_name = nullptr;
      }

      snprintf(command_str, sizeof(command_str), "load s \"%s\"\n", cod_name);
      parse_string(command_str);

    } else  if (processor_name) {
      if (hex_name) {
        snprintf(command_str, sizeof(command_str),
                 "load %s \"%s\"\n", processor_name, hex_name);
        parse_string(command_str);
        free((void *)hex_name);
        hex_name = nullptr;

      } else {
        snprintf(command_str, sizeof(command_str),
                 "processor %s \n", processor_name);
        parse_string(command_str);
      }
    }

    if (icd_port) {
      snprintf(command_str, sizeof(command_str),
               "icd open \"%s\"\n", icd_port);
      parse_string(command_str);
    }

    if (startup_name) {
      snprintf(command_str, sizeof(command_str),
               "load c \"%s\"\n", startup_name);
      parse_string(command_str);
    }

    if (include_startup_name) {
      snprintf(command_str, sizeof(command_str),
               "load i \"%s\"\n", include_startup_name);
      parse_string(command_str);
    }

    // otherwise see if load will work
    if (hex_name) {
      snprintf(command_str, sizeof(command_str),
               "load  \"%s\"\n", hex_name);
      parse_string(command_str);
    }

    if (abort_gpsim) {
      exit_gpsim(0);
    }

    // Now enter the event loop and start processing user
    // commands.
#ifdef HAVE_GUI
    gui_main();
#else
    cli_main();
#endif

  } catch (char * err_message) {
    std::cout << "FATAL ERROR: " << err_message << '\n';

  } catch (FatalError &err) {
    std::cout << err.what() << '\n';
    exit_gpsim(1);
  }

  exit_gpsim(0);
  return 0;
}
