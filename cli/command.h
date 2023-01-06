/*
   Copyright (C) 1998,199 T. Scott Dattalo

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

#ifndef CLI_COMMAND_H_
#define CLI_COMMAND_H_

#include <string>
#include <list>
#include "misc.h"
#include "../src/expr.h"

class Processor;
class Value;
struct cmd_options;

enum COMMAND_MODES
{
  NO_COMMAND,
  COMMAND_PENDING,
  COMMAND_ERROR
};

// Token types for command options
#define OPT_TT_BITFLAG  1   //
#define OPT_TT_NUMERIC  2   // indicates that a numeric parameter
                            // is associated with the option
#define OPT_TT_STRING   3   // indicates that a string parameter
                            // is associated with the option
#define OPT_TT_SUBTYPE  4   // indicates that this command can
                            // be subtyped (e.g. the stimulus command
                            // is subtyped by the type of stimulus
                            // that is being created).
#define OPT_TT_SYMBOL   5   // A symbol is the option parameter

class command
{
public:
    struct cmd_options *op;
    std::string brief_doc;
    std::string long_doc;

    int  token_value;

    virtual bool can_span_lines() {return 0;}

    command(const char *_name, const char *_abbr);
    virtual ~command() {}

    struct cmd_options *get_op()
    {
        return op;
    }

    const char *name() { return m_pName; }
    const char *abbreviation() { return m_pAbbreviation; }
    static Processor * GetActiveCPU(bool bDisplayWarnings = false);

    int get_token() {return token_value;}

    // Assume command is not repeatable
    virtual int is_repeatable() { return 0; }
    virtual double evaluate(Expression *);
    virtual void evaluate(ExprList_t *eList, uint64_t *, int *);
    virtual Value *toValue(Expression *expr);

private:
    const char   *m_pName;
    const char   *m_pAbbreviation;
};

class cmd_options_expr
{
public:
    cmd_options_expr(cmd_options *, Expression *);
    ~cmd_options_expr();

    cmd_options *co;
    Expression *expr;
};

extern command *command_list[];
extern int number_of_commands;
extern command *search_commands(const std::string &s);
extern int quit_gpsim;
extern void execute_line(char *);

#define DEBUG_PARSER 0

//========================================
// typedefs

typedef std::list<std::string> StringList_t;

#endif
