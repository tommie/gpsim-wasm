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

#include <stdio.h>

#include <iostream>
#include <string>

#include "command.h"
#include "cmd_icd.h"
#include "misc.h"
#include "../src/icd.h"

cmd_icd c_icd;

#define ICD_OPEN_CMD 1

static cmd_options cmd_icd_options[] =
{
  {"open",	ICD_OPEN_CMD,	OPT_TT_STRING},
  {nullptr, 0, 0}
};

cmd_icd::cmd_icd()
  : command("icd", nullptr)
{ 
  brief_doc = "ICD command.";

  long_doc = "\nicd [open <port>]\n\
\tThe open command is used to enable ICD mode and specify the serial\n\
\tport where the ICD is. (e.g. \"icd open /dev/ttyS0\").\n\
\tWithout options (and after the icd is enabled), it will print some\n\
\tinformation about the ICD.\n";

  op = cmd_icd_options; 
}

void cmd_icd::icd()
{
  if (icd_detected()) {
    printf("ICD version \"%s\" was found.\n", icd_version());
    printf("Target controller is %s.\n", icd_target());
    printf("Vdd: %.1f\t", icd_vdd());
    printf("Vpp: %.1f\n", icd_vpp());

    if (icd_has_debug_module())
      puts("Debug module is present");
    else
      puts("Debug moudle is NOT present.");

  } else {
    printf("ICD has not been opened (use the \"icd open\" command)\n");
  }
}

void cmd_icd::icd(cmd_options_str *cos)
{
  switch (cos->co->value) {
  case ICD_OPEN_CMD:
    std::cout << "ICD open " << cos->str << '\n';
    icd_connect(cos->str.c_str());
    break;

  default:
    std::cout << " Invalid set option\n";
  }
}
