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

#ifndef CLI_CMD_SYMBOL_H_
#define CLI_CMD_SYMBOL_H_

#include "command.h"
#include "../src/expr.h"

class gpsimObject;

class cmd_symbol : public command
{
public:
  cmd_symbol();

  void dump_all();
  void dump_one(const char *sym_name);
  void dump_one(gpsimObject *);
  void dump(gpsimObject *, ExprList_t*);
  void EvaluateAndDisplay(Expression *pExpr);

  void Set(gpsimObject *s, ExprList_t *e, Expression *pExpr);

  void add_one(const char *sym_name, Expression *);
};

extern cmd_symbol c_symbol;

#endif

