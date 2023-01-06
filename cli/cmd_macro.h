/*
   Copyright (C) 2004 T. Scott Dattalo

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

#ifndef CLI_CMD_MACRO_H_
#define CLI_CMD_MACRO_H_

#include <list>
#include <string>

#include "command.h"
#include "../src/gpsim_object.h"

typedef std::list<std::string> StringList_t;

class Macro : public gpsimObject {
public:
  explicit Macro(const char *new_name);
  StringList_t arguments;     // declared when macro is defined
  StringList_t body;          // what the macro contains.
  StringList_t parameters;    // passed to macro during invocation

  void invoke();
  void prepareForInvocation();
  int substituteParameter(const std::string &s, std::string &replaced);
  int nParameters();
  void add_argument(const char *new_arg);
  void add_parameter(const char *s);
  void add_body(const char *new_line);
  void print();
};


class cmd_macro : public command {
public:
  cmd_macro();

  void list();
  void define(const char *name);
  void add_parameter(const char *parameter);
  void add_body(const char *line);
  void end_define(const char *opt_name = nullptr);

  int is_repeatable() override { return 1; }
};

extern cmd_macro c_macro;

#endif
