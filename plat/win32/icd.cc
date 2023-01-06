/*
   Copyright (C) 2003 BorutRazem

This is based on the program icdprog 0.3 made by Geir Thomassen

gpsim is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

gpsim is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with gpasm; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/*
Dummy icd implementation for WIN32 platform.
*/

#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <list>
#include <fcntl.h>
#include "icd.h"

bool get_use_icd() {
  return false;
}

const char *icd_target()
{
    return "no target";
}


const char *icd_version()
{
    return "%0.00.00";
}


int icd_halt(void)
{
    return 1;
}


float icd_vdd(void)
{
    return 0.0;
}


float icd_vpp(void)
{
    return 0.0;
}


int icd_clear_break(void)
{
    return 1;
}


int icd_connect(const char *port)
{
    return 1;
}


int icd_detected(void)
{
    return 0;
}


int icd_disconnect(void)
{
    return 1;
}


int icd_has_debug_module(void)
{
    return 0;
}


int icd_reset(void)
{
    return 1;
}


int icd_run(void)
{
    return 1;
}


int icd_stopped(void)
{
    return 0;
}


int icd_set_break(int address)
{
    return 1;
}


int icd_step(void)
{
    return 1;
}


void icd_set_bulk(int flag)
{
}
