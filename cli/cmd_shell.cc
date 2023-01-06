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

#include <ctype.h>
#include <stdio.h>

#include <string>

#include "command.h"
#include "cmd_shell.h"
#include "misc.h"
#include "../src/cmd_manager.h"
#include "../src/gpsim_interface.h"
#include "../src/value.h"

cmd_shell c_shell;

static cmd_options cmd_shell_options[] =
{
  {nullptr, 0, 0}
};

cmd_shell::cmd_shell()
  : command("!", nullptr)
{
  brief_doc = "Shell out to another program or module's command line interface";

  long_doc = "!cmd.exe copy a.c b.c\n"
    "!picxx args\n"
    "\n";

  op = cmd_shell_options;
}

void cmd_shell::shell(String *cmd)
{
  const std::string sTarget = cmd->getVal();
  const char *pArguments = sTarget.c_str();

  if (*pArguments == '\0') {
     CCommandManager::GetManager().ListToConsole();
  } else {
    while (*pArguments != '\0' && !isspace(*pArguments))
      pArguments++;

    std::string name = sTarget.substr(0, pArguments - sTarget.c_str());
    pArguments++;

    int iResult = CCommandManager::GetManager().Execute(name, pArguments);
    if (iResult == CMD_ERR_PROCESSORNOTDEFINED)
      printf("%s module command processor not found\n", name.c_str());
  }
}
