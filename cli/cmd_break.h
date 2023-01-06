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

#ifndef CLI_CMD_BREAK_H_
#define CLI_CMD_BREAK_H_

#include "command.h"

#include "../src/expr.h"

class gpsimObject;
struct cmd_options;

#define CMDBREAK_BAD_BREAK_NUMBER 0xffff

class cmd_break : public command {
public:
  cmd_break();

  void list(uint64_t value = CMDBREAK_BAD_BREAK_NUMBER);
  unsigned int set_break(cmd_options *co, bool bLog = false);
  unsigned int set_break(cmd_options *co, ExprList_t *pEL, bool bLog = false);
  unsigned int set_break(gpsimObject *v);

private:
  unsigned int set_break(cmd_options *co, Expression *pExpr1, Expression *pExpr2 = nullptr, bool bLog = false);
  unsigned int set_break(int bit_flag, bool bLog);
};

extern cmd_break c_break;

#endif
