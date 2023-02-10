/*
   Copyright (C) 1998 T. Scott Dattalo

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

#ifndef CLI_INPUT_H_
#define CLI_INPUT_H_

#include <config.h>
class Processor;

#define CONTINUING_LINE 1

int gpsim_read(char *buf, unsigned int max_size);
void initialize_readline();
void exit_cli();
void exit_gpsim(int);
void SetLastFullCommand(const char *pCmd);
const char *GetLastFullCommand();
void EnableSTCEcho(bool bEnable);

#ifndef HAVE_GUI
char *gnu_readline(char *s, unsigned int force_readline);
#endif

int gpsim_open(Processor *cpu, const char *file,
               const char *pProcessorType, const char *pProcessorName);

#endif
