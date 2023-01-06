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
Declarations, missing in MSVC header files,
normaly defined in <time.h> and <sys/time.h>
*/

#ifndef _UXTIME_H
#define _UXTIME_H

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Structure returned by gettimeofday(2) system call
 */

#ifdef _MSC_VER
#ifndef _WINSOCKAPI_
struct timeval {
  unsigned long tv_sec; /* seconds */
  long tv_usec;         /* and microseconds */
};

struct timezone {
  int tz_minuteswest;
  int tz_dsttime;
};
#endif

int gettimeofday(struct timeval *tv, struct timezone *tz);
#endif

#ifndef _TIMESPEC_DEFINED
#define _TIMESPEC_DEFINED

struct timespec {
  time_t  tv_sec;   /* Seconds */
  long    tv_nsec;  /* Nanoseconds */
};

#endif

int nanosleep(const struct timespec *rqtp, struct timespec *rmtp);

#ifdef __cplusplus
}
#endif

#endif
