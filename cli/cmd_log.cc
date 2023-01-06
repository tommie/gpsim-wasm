/*
   Copyright (C) 1999 T. Scott Dattalo

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


#include <iostream>
#include <list>
#include <string>

#include "command.h"
#include "cmd_log.h"
#include "misc.h"

#include "../src/trace.h"
#include "../src/value.h"

#include "cmd_break.h"

cmd_log c_log;

#define LOG_ON	    1
#define LOG_OFF	    2
#define WRITE       3
#define READ        4
#define LOG_LXT     5


static cmd_options cmd_trace_options[] =
{
  {"on",  LOG_ON,      OPT_TT_BITFLAG},
  {"off", LOG_OFF,     OPT_TT_BITFLAG},
  {"w",   WRITE,       OPT_TT_BITFLAG},
  {"r",   READ,        OPT_TT_BITFLAG},
  {"lxt", LOG_LXT,     OPT_TT_BITFLAG},
  {nullptr, 0, 0}
};

cmd_log::cmd_log()
  : command("log", nullptr)
{
  brief_doc = "Log/record events to a file";

  long_doc = "\n\
The log command will record simulation history in a file. It's similar to the\n\
break command\n\
  log [[on|lxt [file_name]]|[off]]\n\
    Enables or disables logging. Specify no options to see log status.\n\
    The lxt option encodes the log file so that an external viewer\n\
    like gtkwave can be used to view the file.\n\
  log w|r reg [, expr]\n\
    Specify a register to log. See the break command for expression syntax\n\
\n  Examples:\n\
\tlog               - Display log status\n\
\tlog on            - Begin logging in file gpsim.log\n\
\tlog on file.log   - Begin logging in file file.log\n\
\tlog lxt           - Begin lxt logging in file gpsim.lxt\n\
\tlog lxt file.lxt  - Begin lxt logging in file file.lxt\n\
\tlog off           - Stop logging\n\
\tlog w temp_hi     - Log all writes to reg temp_hi\n";

  op = cmd_trace_options;
}

void cmd_log::log()
{
  GetTraceLog().status();
}

void cmd_log::log(cmd_options *opt, ExprList_t *eList)
{
  if (!opt) {
    log();
    return;
  }

  switch(opt->value) {
  case LOG_ON:
  case LOG_LXT:
    {
      int fmt = opt->value == LOG_ON ? TRACE_FILE_FORMAT_ASCII : TRACE_FILE_FORMAT_LXT;

      if (eList) {
        ExprList_itor ei = eList->begin();
        Expression *pFirst = *ei;
        LiteralString *pString = nullptr;

        if (pFirst) {
          pString = dynamic_cast<LiteralString*>(pFirst);
          if (pString) {
            String *pS = (String *)pString->evaluate();
            GetTraceLog().enable_logging(pS->getVal(),fmt);
            delete pFirst;
            delete pS;
          }
        }
      } else
        GetTraceLog().enable_logging(0, fmt);
    }
    break;

  case LOG_OFF:
    GetTraceLog().disable_logging();
    break;

  default:
    c_break.set_break(opt, eList, true);
  }

}


void cmd_log::log(cmd_options *opt)
{
  switch(opt->value) {
  case LOG_ON:
    GetTraceLog().enable_logging(0);
    break;

  case LOG_OFF:
    GetTraceLog().disable_logging();
    break;

  case LOG_LXT:
    GetTraceLog().enable_logging(0, TRACE_FILE_FORMAT_LXT);
    break;

  default:
    std::cout << " Invalid log option\n";
  }
}
