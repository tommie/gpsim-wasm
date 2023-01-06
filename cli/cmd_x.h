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

#ifndef CLI_CMD_X_H_
#define CLI_CMD_X_H_

#include "command.h"
class Expression;

class cmd_x : public command {
public:
  cmd_x();

  void x();
  void x(int reg, Expression *pExpr = nullptr);
  
  void x(char *reg_name, int val);
  void x(char *reg_name);
  void x(Expression * , Expression *pExpr = nullptr);

  int is_repeatable() override { return 1; }
  bool int_from_expression(Expression *, unsigned int &val);
};

extern cmd_x c_x;

class cmd_reg : public command 
{
public:
    cmd_reg();
};

extern cmd_reg c_reg;
#endif

