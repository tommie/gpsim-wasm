/*
   Copyright (C) 2003-2006 Borut Razem

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

/*
Definitions, missing in MSVC libraries
*/

#include <windows.h>

#include "uxtime.h"

#ifdef __cplusplus
extern "C" {
#endif

int
gettimeofday(struct timeval *tv, struct timezone *tz)
{
  ULARGE_INTEGER curr_time;
  FILETIME curr_file_time;

  /* get current system time as file time */
  GetSystemTimeAsFileTime(&curr_file_time);
  curr_time.HighPart = curr_file_time.dwHighDateTime;
  curr_time.LowPart = curr_file_time.dwLowDateTime;

  curr_time.QuadPart -= 0x19db1ded53e8000LL;

  tv->tv_sec = (long)(curr_time.QuadPart / 10000000);
  tv->tv_usec = (long)((curr_time.QuadPart / 10) % 1000000);

  return 0;
}

#ifdef __cplusplus
}
#endif
