%{

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
#include <iostream>

#include <string>
#include <unistd.h>
#ifdef _WIN32
/* declaration of isatty() */
#include <io.h>
#endif

#include <config.h>

#include "../src/operator.h"
#include "../src/symbol.h"
#include "../src/stimuli.h"
#include "../src/processor.h"

#include "command.h"
#include "cmd_macro.h"

#include "parse.h"
#include "input.h"
#include "scan.h"

/* Since our parser is reentrant, it needs to pass us a pointer
 * to the yylval that it would like us to use */
#ifdef YY_PROTO
#define YY_DECL int yylex YY_PROTO(( YYSTYPE* yylvalP ))
#else
#define YY_DECL int yylex ( YYSTYPE* yylvalP )
#endif
extern int yyparse(void);
#define exit exit_gpsim

/* This is the max length of a line within a macro definition */
static char macroBody[65536], *macroBodyPtr=0;
static char* max_bodyPtr = &macroBody[0] + sizeof(macroBody)-1;

struct LexerStateStruct {
  struct cmd_options *options;
  command *cmd;
  int input_mode;
  int end_of_command;
  int have_parameters;
  int mode;

  struct LexerStateStruct *prev;
  struct LexerStateStruct *next;

};

static char *        m_pLastFullCommand = NULL;

void SetLastFullCommand(const char *pCmd)
{
  if (strlen(pCmd)>1) {
    if (m_pLastFullCommand)
      free (m_pLastFullCommand);
    m_pLastFullCommand = strdup(pCmd);
  }
}
const char * GetLastFullCommand()
{
  return m_pLastFullCommand;
}

static LexerStateStruct *pLexerState = 0;
static int sLevels = 0;

extern int quit_parse;
extern int parser_spanning_lines;
extern int last_command_is_repeatable;

static std::string strip_trailing_whitespace(char *s);
static int handle_identifier(YYSTYPE* yylvalP, std::string &tok, cmd_options **op );
static int process_intLiteral(YYSTYPE* yylvalP, char *buffer, int conversionBase);
static int process_booleanLiteral(YYSTYPE* yylvalP, bool value);
static int process_macroBody(YYSTYPE* yylvalP, const char *text);
static int process_floatLiteral(YYSTYPE* yylvalP, char *buffer);
static int process_stringLiteral(YYSTYPE* yylvalP, const char *buffer);
static int process_quotedStringLiteral(YYSTYPE* yylvalP, char *buffer);
static int process_shellLine(YYSTYPE* yylvalP, const char *buffer);
static int recognize(int token,const char *);
static void SetMode(int newmode);

void scanPopMacroState();
int cli_corba_init (char *ior_id);

extern Macro *isMacro(const std::string &s);
static Macro *gCurrentMacro=0;

#define YYDEBUG 1



//========================================================================
// MacroChain class
//
//

class MacroChain
{
public:

  struct Link {
    Link *prev;
    Link *next;
    Macro *m;
  };

  MacroChain()
    : param(nullptr)
  {
    head.prev = head.next = nullptr;
    curr = &head;
  }

  void push(Macro *m) {
    if (verbose & 4 && m) {
      std::cout << "Pushing " << m->name() << " onto the macro chain\n";
    }

    Link *pL = new Link();
    pL->m = m;
    pL->prev = &head;
    pL->next = head.next;
    head.next = pL;
    param = pL;
    curr = &head;
  }

  void pop()
  {
    Link *pL = head.next;
    if (pL) {

      if (verbose & 4 && pL->m) {
        std::cout << "Popping " << pL->m->name() << " from the macro chain\n";
      }

      head.next = pL->next;
      if (pL->next)
        pL->next->prev = &head;
      delete pL;
    }
  }

  Macro *nextParamSource()
  {
    if (curr)
      curr = curr->next;
    if (verbose & 4 && curr && curr->m ) {
      std::cout << " selecting parameter source " << curr->m->name() << '\n';
    }
    if(curr)
      return curr->m;
    return 0;
  }
  void popParamSource()
  {
    if (verbose & 4 && curr && curr->m ) {
      std::cout << " popping parameter source " << curr->m->name() << '\n';
    }
    if (curr)
      curr = curr->prev;
  }

  void resetParamSource()
  {
    if (verbose & 4) {
      std::cout << " resetparameter source\n";
    }
    curr = &head;
  }

private:
  Link *curr;
  Link head;
  Link *param;
};

static MacroChain theMacroChain;



%}

D       [0-9]
S       [ \t]
NL      ((\n)|(\r\n))
SNL     ({S}|{NL})
BS      (\\)
CONT    ({EL}|{BS})
CCHAR   (#)
COMMENT ({CCHAR}.*)
SNLCMT  ({SNL}|{COMMENT})
INDIRECT (\*)
IDENTIFIER ([\']?[/_a-zA-Z\.][/_a-zA-Z0-9\.\-]*)
INDEXERLEFT    (\[)
INDEXERRIGHT    (\])
EXPON   ([DdEe][+-]?{D}+)
DEC     ({D}+)
DEC2    ([dD]\'{D}+\')
HEX1    ((0[Xx])[0-9a-fA-F]+)
HEX2    ("$"[0-9a-fA-F]+)
FLOAT   (({D}+\.?{D}*{EXPON}?)|(\.{D}+{EXPON}?))
BIN1    (0[bB][01]+)
BIN2    ([bB]\'[01]+\')
SHELLCHAR (^[!])
SHELLLINE   ({SHELLCHAR}.*)
QUOTEDTOKEN (("\"".*\")|("\'".*'))
QUOTEDTOKEN2 (([\\]"\"".*\")|([\\]"\'".*'))


%{
/* Lexer States */
%}
%x MACROBODY
%x DECLARATION

%{
//************************************************************************
//************************************************************************
%}

%%

%{
// Comments. Ignore all text after a comment character
%}

{COMMENT}
  {
    return recognize(COMMENT_T,"comment");
  }


{S}+      {   /* ignore white space */ }

\n {
      if(verbose)
          std::cout << "got EOL\n";

      pLexerState->input_mode = 0;  // assume that this is not a multi-line command.
      if(pLexerState->cmd &&
         pLexerState->cmd->can_span_lines() &&
         pLexerState->have_parameters &&
         !pLexerState->end_of_command )

        pLexerState->input_mode = CONTINUING_LINE;

      else {
        pLexerState->cmd = 0;
        return recognize(EOLN_T, " end of line");
      }
}

<INITIAL>  {
      // Got an eol.
      if(verbose)
          std::cout << "got INITIAL\n";

      pLexerState->input_mode = 0;  // assume that this is not a multi-line command.
      if(pLexerState->cmd &&
         pLexerState->cmd->can_span_lines() &&
         pLexerState->have_parameters &&
         !pLexerState->end_of_command )

        pLexerState->input_mode = CONTINUING_LINE;

      //else
      //  return recognize(EOLN_T, " end of line");
    }

q{S}+\n { /* short cut for quiting */
      quit_parse  =1;
      return QUIT;
    }

abort_gpsim_now {
  /* a sure way to abort a script */
   return ABORT;
   }

"="                 {return(recognize(EQU_T, "="));}
"&&"                {return(recognize(LAND_T, "&&"));}
"||"                {return(recognize(LOR_T, "||"));}

":"                 {return(recognize(COLON_T, ":"));}
"!"                 {return(recognize(LNOT_T,"!"));}
"~"                 {return(recognize(ONESCOMP_T,"~"));}
"+"                 {return(recognize(PLUS_T,"+"));}
"-"                 {return(recognize(MINUS_T,"-"));}
"*"                 {return(recognize(MPY_T,"*"));}
"/"                 {return(recognize(DIV_T,"/"));}
"^"                 {return(recognize(XOR_T,"^"));}
"&"                 {return(recognize(AND_T,"&"));}
"|"                 {return(recognize(OR_T,"|"));}
"<<"                {return(recognize(SHL_T,"<<"));}
">>"                {return(recognize(SHR_T,">>"));}

"=="                {return(recognize(EQ_T, "=="));}
"!="                {return(recognize(NE_T, "!="));}
"<"                 {return(recognize(LT_T, "<"));}
">"                 {return(recognize(GT_T, ">"));}
"<="                {return(recognize(LE_T, "<="));}
">="                {return(recognize(GE_T, ">="));}

"["                 {return(recognize(INDEXERLEFT_T, "["));}
"]"                 {return(recognize(INDEXERRIGHT_T, "]"));}

{BIN1}              {return(process_intLiteral(yylvalP,&yytext[2], 2));}
{BIN2}              {return(process_intLiteral(yylvalP,&yytext[2], 2));}
{DEC}               {return(process_intLiteral(yylvalP,&yytext[0], 10));}
{DEC2}              {return(process_intLiteral(yylvalP,&yytext[2], 10));}
{HEX1}              {return(process_intLiteral(yylvalP,&yytext[2], 16));}
{HEX2}              {return(process_intLiteral(yylvalP,&yytext[1], 16));}
{FLOAT}             {return process_floatLiteral(yylvalP,yytext);}
"true"              {return(process_booleanLiteral(yylvalP,true));}
"false"             {return(process_booleanLiteral(yylvalP,false));}
"reg"               {return(recognize(REG_T,"reg"));}
"pin"               {return(recognize(GPSIMOBJECT_T, "pin")); /*return(recognize(STIMULUS_T, "pin"));*/}
"port"              {return(recognize(PORT_T, "port"));}

"endm"              {scanPopMacroState();}

^[!].*              {return(process_shellLine(yylvalP,&yytext[1]));}
{QUOTEDTOKEN}       {return(process_quotedStringLiteral(yylvalP,&yytext[1]));}
{QUOTEDTOKEN2}       {return(process_quotedStringLiteral(yylvalP,&yytext[2]));}


%{
//========================================================================
// Macro processing
%}

<MACROBODY>^[ \t]?+"endm" {SetMode(INITIAL); return(recognize(ENDM,"endm")); }

<MACROBODY>\r            {/*discard CR's*/}
<MACROBODY>\n            {*macroBodyPtr++ = '\n';
                          *macroBodyPtr = 0;
                           macroBodyPtr = macroBody;
                           return(process_macroBody(yylvalP,macroBody));}


<MACROBODY>.             {  *macroBodyPtr++ = *yytext;
                             if(verbose&4)
                               printf("adding [%c]\n", *yytext);
                             if (macroBodyPtr > max_bodyPtr) {
                               std::cout << "buffer overflow in macro definition\n";
                               exit_gpsim(0);
                             }
                         }



%{
//========================================================================
// Declaration Processing
%}
<DECLARATION>\n {SetMode(INITIAL); return(recognize(EOLN_T, "end of declaration")); }
<DECLARATION>"int"   {return recognize(DECLARE_INT_T,  "int type");   }
<DECLARATION>"float" {return recognize(DECLARE_FLOAT_T,"float type"); }
<DECLARATION>"bool"  {return recognize(DECLARE_BOOL_T, "bool type");  }
<DECLARATION>"char"  {return recognize(DECLARE_FLOAT_T,"char type");  }
<DECLARATION>{IDENTIFIER}  { return process_stringLiteral(yylvalP, yytext); }
%{

// The 'echo' command is handled by the lexer instead of the
// parser (like the other commands). All it does is just display
// the contents of yytext beyond the "echo".

%}

"echo"{S}.*{NL} {
  fprintf(yyout,"%s",&yytext[5]);
  return recognize(EOLN_T, " end of line");

  }

"echo"{NL} {
  fprintf(yyout,"\n");
  return recognize(EOLN_T, " end of line");
  }


%{
// Indirect register access.... this should be an expression operator.
%}

%{
  /*
{INDIRECT} {

  return INDIRECT;
  }
*/
%}


%{
// If this is a command that is spanning more than one line
// then the 'end' command will finish it.
%}

"end"{S}* {
    if (pLexerState->cmd && pLexerState->cmd->can_span_lines() ) {
      pLexerState->end_of_command = 1;
      return(END_OF_COMMAND);
    }
    printf("Warning: found \"end\" while not in multiline mode\n");
  }

%{
// Identifiers. These are either gpsim commands or user macros.
%}

{IDENTIFIER} {
  std::string tok = strip_trailing_whitespace(yytext);

  int ret=0;
  if(strlen(tok.c_str()))
    ret = handle_identifier (yylvalP, tok, &pLexerState->options);
  else
    ret = recognize(0,"invalid identifier");

  if(ret)
    return ret;
  }


%{
/* Default is to recognize the character we are looking at as a single char */
%}
.                       {return(recognize(*yytext,"Single character"));}

%%

/* make it work with flex 2.5.31 */
#ifndef yytext_ptr
#define yytext_ptr yytext
#endif

#define MAX_STACK_LEVELS  16
static int input_stack_index=0;
YY_BUFFER_STATE input_stack[MAX_STACK_LEVELS];

/************************************************************************
 * yywrap()
 *
 * Revert to an old input stream if there is one. If there is not an old
 * old stream, then the lexer will try to get data from YY_INPUT which
 * is a macro that calls gpsim_read() (see scan.h).
 * An 'old' stream is one that was interrupted by a macro expansion.
 *
 */
#ifdef yywrap
#undef yywrap
#endif
int yywrap (void)
{
  if(input_stack_index) {
    yy_delete_buffer(YY_CURRENT_BUFFER);
    yy_switch_to_buffer(input_stack[--input_stack_index]);
    return 0;
  }

  return 1;
}

/************************************************************************
 * push_input_stack
 *
 * called when macros are being expanded.
 */
static void push_input_stack(void)
{
  if(input_stack_index<MAX_STACK_LEVELS)
    input_stack[input_stack_index++] = YY_CURRENT_BUFFER;
}

/************************************************************************
 *
 */
static int recognize(int token_id,const char *description)
{
  /* add optional debugging stuff here */
  if((bool)verbose && description)
    std::cout << "scan: " << description << '\n';

  return(token_id);
}

/************************************************************************
 */
int translate_token(int tt)
{
  switch(tt)
  {
  case OPT_TT_BITFLAG:
    return recognize(BIT_FLAG,"BIT_FLAG");
  case OPT_TT_NUMERIC:
    return recognize(EXPRESSION_OPTION,"EXPRESSION_OPTION");
  case OPT_TT_STRING:
    return recognize(STRING_OPTION,"STRING_OPTION");
  case OPT_TT_SUBTYPE:
      return recognize(CMD_SUBTYPE,"CMD_SUBTYPE");
  case OPT_TT_SYMBOL:
    return recognize(SYMBOL_OPTION,"SYMBOL_OPTION");
  }

  return 0;

}

static bool bTryMacroParameterExpansion(std::string &s)
{

  // If we're invoking a macro, search the parameters
  std::string replaced;
  Macro *currentMacro = theMacroChain.nextParamSource();

  if (verbose & 4) {
    std::cout << "Searching for parameter named:" << s;
    if (currentMacro)
      std::cout << " in macro: " << currentMacro->name() << '\n';
    else
      std::cout << " but there is no current macro\n";
  }

  if(currentMacro && currentMacro->substituteParameter(s,replaced))
    if(replaced != s) {
      if (verbose & 4)
        std::cout << "  -- found it and replaced it with " << replaced << '\n';
      if (bTryMacroParameterExpansion (replaced))
        return true;
      push_input_stack();
      yy_scan_string(replaced.c_str());
      theMacroChain.resetParamSource();
      return true;
    }
  theMacroChain.popParamSource();
  return false;

}

/*************************************************************************
*
* handle_identifier
*
*  input   string &s
*          cmd_options **op
*  output  int
*
*  1 - If `op' is NULL, then handle identifier hasn't been called
*      for the current command that's being processed. So, the
*      the string `s' is compared to all of the valid commands.
*      If it is valid, then `op' is assigned a pointer to the
*      options associated with the command. If the string is not
*      found, then that's a syntax error and the string is ignored.
*  2 - If `op' is non-NULL, then handle_identifier has been called
*      at least once before for the command that's being processed.
*      So the string `s' is then compared to the options associated
*      with the command. If an option is not found, then the string
*      is returned to the parser (as a type STRING). This places the
*      burden of syntax checking on the parser and/or the individual
*      command.
*
*/

int handle_identifier(YYSTYPE* yylvalP, std::string &s, cmd_options **op )
{
  int retval = 0;

  // If no command has been found so far, then the options (*op)
  // haven't been selected either (and consequently *op is null).

  if(! *op) {


    // If the first character in the string is a ' (single quote character) then
    // this means that the user is explicitly trying to access a user defined symbol
    // (e.g. if there is variable named "help" in the user's symbol table, then the
    // only way to get access to it is by using the single quote character:
    //    'help

    if(s[0] == '\'')

      // Strip away the quote, we won't treat this as a command and the parser
      // doesn't want to know about it.

      s=s.erase(0,1);

    else {

      // Search the commands
      pLexerState->cmd = search_commands(s);
      if(pLexerState->cmd) {
        if (verbose & 2)
          std::cout << "\n  *******\nprocessing command " << (pLexerState->cmd->name()) << "\n  token value " <<
            (pLexerState->cmd->get_token()) << "\n *******\n";

        *op = pLexerState->cmd->get_op();
        pLexerState->have_parameters = 0;
        retval = pLexerState->cmd->get_token();

        // ugh. This is problem when the parser becomes re-entrant.
        last_command_is_repeatable = pLexerState->cmd->is_repeatable();

        return recognize(retval,"good command");
      }

    }

    // Search the macros
    yylvalP->Macro_P = isMacro(s);
    if(yylvalP->Macro_P) {
      return MACROINVOCATION_T;
    }

    if (bTryMacroParameterExpansion(s))
      return 0;

  } else {

    if (verbose & 2)
      std::cout << "search options for command '"
           << (pLexerState->cmd ? pLexerState->cmd->name() : "?")
           << "'\n";

    if (bTryMacroParameterExpansion(s))
      return 0;

    // We already have the command, so search the options.

    struct cmd_options *opt = *op;

    // We also have one or more parameters now (though they
    // may not be correct, but that's the parser's job to determine).

    pLexerState->have_parameters = 1;

    while(opt->name)
      if(strcmp(opt->name, s.c_str()) == 0) {
        if(verbose&2)
          std::cout << "found option '" << opt->name << "'\n";
        yylvalP->co = opt;
        return recognize(translate_token(opt->token_type),"option");
      }
      else
        opt++;
  }

  // If we get here, then the option was not found.
  // So let's check the symbols
  Processor *cpu;
  if(s[0] == '.' &&  (cpu = get_active_cpu()) != 0)
    s.insert(0,cpu->name());

  std::string s1(s);

  gpsimObject *obj = globalSymbolTable().find(s1);
  if(obj) {
    yylvalP->Symbol_P = obj;

    if (verbose & 2)
      std::cout << "found symbol '" << obj->name() << "'\n";

    return recognize(SYMBOL_T,"symbol");
  }

  //cout << "didn't find it in the symbol list\n";

  // Either 1) there's a typo or 2) the command is creating
  // a new symbol or node or something along those lines.
  // In either case, let's let the parser deal with it.

  if(verbose&2)
    std::cout << " returning unknown string: " << s << '\n';

  return process_stringLiteral(yylvalP,s.c_str());

  return 0;

}


/*****************************************************************
 * Process an integer literal.  This routine constructs the
 * YYSTYPE object.  The caller is responsible from returning the
 * LITERAL_INT_T token identifer to the parser.
 */
static int process_intLiteral(YYSTYPE* yylvalP, char *buffer, int conversionBase)
{
  char c;
  char *pt = buffer;
  int64_t literalValue=0;
  int64_t nxtDigit;

  while (*pt) {
    c = toupper(*pt++);

    nxtDigit = (c) <= '9' ? c-'0' : c-'A'+10;
    if ((nxtDigit >= conversionBase) || (nxtDigit<0)) {
      if(!(c == '\''))
      {
	std::cout << "Error conversion to integer " << buffer << '\n';
        literalValue = 0;
      }
      break;
    }

    literalValue *= conversionBase;
    literalValue += nxtDigit;
  }

  yylvalP->Integer_P = new Integer(literalValue);

  return(recognize(LITERAL_INT_T,"literal int"));
}

/*****************************************************************
 *
 */
static int process_macroBody(YYSTYPE* yylvalP, const char *text)
{
  yylvalP->s = strdup(text);
  return recognize(MACROBODY_T,"macro body");
}

/*****************************************************************
 *
 */
static int process_booleanLiteral(YYSTYPE* yylvalP, bool value)
{
  yylvalP->Boolean_P = new Boolean(value);
  return(recognize(LITERAL_BOOL_T, "boolean literal"));
}


/*****************************************************************
 *
 */
static int process_floatLiteral(YYSTYPE* yylvalP, char *buffer)
{
  double floatValue;
#if 0
  errno = 0;
  floatValue = atof(buffer);

  if (errno != 0) {
    /* The conversion failed */
    throw Error("Bad floating point literal");
  }
#else
  char *endptr = nullptr;
  floatValue = strtod(buffer, &endptr);
  if (endptr == buffer)
    throw Error("Bad floating point literal");
#endif

  yylvalP->Float_P = new Float(floatValue);
  return(recognize(LITERAL_FLOAT_T, "float literal"));
}


/*****************************************************************
 *
 */
static int process_stringLiteral(YYSTYPE* yylvalP, const char *buffer)
{
  yylvalP->String_P = new String(buffer);
  return(recognize(LITERAL_STRING_T, "string literal"));
}


static int process_quotedStringLiteral(YYSTYPE* yylvalP, char *buffer)
{

  char * pCloseQuote = strrchr(buffer, '\"');
  if(pCloseQuote == NULL)
    pCloseQuote = strrchr(buffer, '\'');
  *pCloseQuote = 0;
  if (*(pCloseQuote-1) == '\\')
      *(pCloseQuote-1) = 0;
  yylvalP->String_P = new String(buffer);
  return(recognize(LITERAL_STRING_T, "string literal"));
}


/*****************************************************************
 *
 */
static int process_shellLine(YYSTYPE* yylvalP, const char *buffer)
{
  yylvalP->String_P = new String(buffer);
  return(recognize(SHELL, "shell line"));
}


static std::string
strip_trailing_whitespace(char *s)
{
  std::string retval = s;

  size_t pos = retval.find_first_of(" \t");

  if (pos != std::string::npos)
    retval.resize(pos);

  return retval;
}


//------------------------------------------------------------------------
static void SetMode(int newmode)
{
  BEGIN(newmode);
  if(pLexerState)
    pLexerState->mode = newmode;
}

void initialize_commands(void);

void init_cmd_state(void)
{

  if (pLexerState) {
    if (verbose)
      std::cout << "scan: clearing lexer state and flushing buffer\n";
    pLexerState->cmd = 0;
    pLexerState->options = 0;
    pLexerState->input_mode = 0;
    pLexerState->end_of_command = 0;
    pLexerState->have_parameters = 0;
    pLexerState->mode = 0;
  }

}

void FlushLexerBuffer() {
#ifdef YY_FLUSH_BUFFER
  YY_FLUSH_BUFFER;
#else
  yy_flush_buffer( YY_CURRENT_BUFFER );
#endif
}

static void pushLexerState()
{
  if(verbose)
    std::cout << "pushing lexer state: from level " << sLevels
         << " to " << (sLevels + 1) << '\n';

  sLevels++;

  LexerStateStruct  *pLS = new LexerStateStruct();

  if(pLexerState)
    pLexerState->next = pLS;

  pLS->prev = pLexerState;

  pLexerState = pLS;
  pLS->next = 0;

  init_cmd_state();
}

static void popLexerState()
{
  if(verbose)
    std::cout << "popping lexer state: from level " << sLevels
         << " to " << (sLevels-1) << '\n';

  sLevels--;

  if(pLexerState) {

    LexerStateStruct  *pLS = pLexerState;

    pLexerState = pLS->prev;

    if(pLexerState) {
      pLexerState->next = 0;
      pLexerState->cmd = 0;
      pLexerState->options = 0;
    }
    SetMode(pLS->mode);

    delete pLS;
  }

}

int
scan_read (char *buf, unsigned max_size)
{
  static int lastRet = -1;
  // hack
  int ret = gpsim_read(buf,max_size);

  if (lastRet == ret && ret == 0) {

    *buf = '\n';
    ret = 1;
  }
  lastRet = ret;
  return ret;
}

int init_parser()
{

  pushLexerState();

  int ret = yyparse();

  popLexerState();

  return ret;

}

// Tell us what the current buffer is.

YY_BUFFER_STATE
current_buffer (void)
{
  return YY_CURRENT_BUFFER;
}

// Create a new buffer.

YY_BUFFER_STATE
create_buffer (FILE *f)
{
  return yy_create_buffer (f, YY_BUF_SIZE);
}

// Start reading a new buffer.

void
switch_to_buffer (YY_BUFFER_STATE buf)
{
  yy_switch_to_buffer (buf);
}

// Delete a buffer.

void
delete_buffer (YY_BUFFER_STATE buf)
{
  yy_delete_buffer (buf);
}

// Restore a buffer (for unwind-prot).

void
restore_input_buffer (void *buf)
{
  switch_to_buffer ((YY_BUFFER_STATE) buf);
}

// Delete a buffer (for unwind-prot).

void
delete_input_buffer (void *buf)
{
  delete_buffer ((YY_BUFFER_STATE) buf);
}
//------------------------------------------------------------------------
// called by the parser error handler.
command *getLastKnownCommand()
{
  if(pLexerState)
    return pLexerState->cmd;

  return 0;
}
//----------------------------------------
//
void lexer_setMacroBodyMode(void)
{
  macroBodyPtr = &macroBody[0];
  if(verbose&4)
    std::cout << "setting lexer MACROBODY mode\n";
  SetMode(MACROBODY);
}

//----------------------------------------
//
void lexer_setDeclarationMode()
{
  if(verbose&4)
    std::cout << "setting lexer DECLARATION mode\n";
  SetMode(DECLARATION);
}

//----------------------------------------
static bool isWhiteSpace(char c)
{
  return (c==' ' || c == '\t');
}

//----------------------------------------
// getNextMacroParameter(char *s, int l)
//
// returns true if a macro parameter can be extracted
// from yyinput buffer. If it does return true, then the
// extracted macro parameter will get copied to
// the string 's'.
//
// This routine will lexically analyze a character string
// and split it up into chunks that can be passed to a
// macro invocation. It might be possible to add a new
// lex state and do this work in the lexer...
//
// If input stream looks something like:
//
//  expression1, expression2, expression3, ...
//
// then this function will return true and copies 'expression1'
// to 's'.

static bool getNextMacroParameter(char *s, int l)
{

  char c;

  // delete all leading white space.
  do {
    c = yyinput();
  } while(isWhiteSpace(c));

  if(c==',')
    goto done;


  unput(c);

  if(!c)
    return false;

  {
    int nParen=0;
    bool bDone = false;

    do {
      c = yyinput();

      if(c == '(')
        nParen++;
      if(c == ')' && --nParen < 0 )
        bDone = true;

      if(c==',')
        break;

      if(c==0 || c=='\n' ) {
        bDone=true;
        unput(c);
      } else
        *s++ = c;
    } while(--l>0  && !bDone);
  }
done:
  *s=0;

  return true;
}

void lexer_InvokeMacro(Macro *m)
{

  if(!m)
    return;

  if(verbose &4)
    std::cout << "Invoking macro: " << m->name() << '\n';

  theMacroChain.push(m);

  m->prepareForInvocation();

  int i=0;
  bool bValidParameter = false;
  do {

    i++;
    char s[256];

    bValidParameter = getNextMacroParameter(s,sizeof(s));

    if(bValidParameter) {
      m->add_parameter(s);

      if(verbose &4)
        std::cout << "macro param: " << s << '\n';
    }
  } while (bValidParameter && i<m->nParameters());

  m->invoke();
}

void scanPushMacroState(Macro *m)
{
  gCurrentMacro = m;
}

void scanPopMacroState()
{

  theMacroChain.pop();
}
