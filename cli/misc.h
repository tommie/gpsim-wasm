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

#ifndef CLI_MISC_H_
#define CLI_MISC_H_

#include <string>
#include "../src/cmd_gpsim.h"

// miscellaneous definitions that are used

struct cmd_options {
  const char *name;
  int  value;
  int  token_type;
};

   /* Command option with a numeric parameter */

struct cmd_options_num {
  cmd_options *co;
  int n;
};

   /* Command option with a string parameter */

class cmd_options_str {
public:
  explicit cmd_options_str(const char *);
  ~cmd_options_str();

  cmd_options *co;
  std::string str;
};

#endif
