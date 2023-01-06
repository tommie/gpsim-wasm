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

#ifndef CLI_CMD_DUMP_H_
#define CLI_CMD_DUMP_H_

#include "command.h"
#include "../src/hexutils.h"
class gpsimObject;
class Register;

class cmd_dump : public command, IntelHexProgramFileType {
public:
  enum {
    DUMP_EEPROM = 1,
    DUMP_RAM,
    DUMP_SFRS
  };

  cmd_dump();
  //  void dump(void);

  void dump(int bit_flag);
  int dump(int bit_flag, gpsimObject* module, const char * filename);

private:
  void dump_sfrs();
  void dump_regs(Register **fr, unsigned int mem_size, int reg_size, int mem_type);
};

extern cmd_dump dump;

#endif

