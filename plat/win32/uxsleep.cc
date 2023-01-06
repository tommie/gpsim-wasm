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
#include <errno.h>

#include "uxtime.h"

#ifdef __cplusplus
extern "C" {
#endif

int nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
  int res = 0;
  HANDLE dummy_event;
  int rc;
  DWORD req, start_time, end_time, now, rem;

  if (rqtp->tv_sec < 0 || rqtp->tv_nsec < 0 || rqtp->tv_nsec > 999999999)
    {
      errno = EINVAL;
      return -1;
    }

  dummy_event = CreateEvent (NULL, TRUE, FALSE, NULL); 
  req = rqtp->tv_sec * 1000 + (rqtp->tv_nsec + 500000) / 1000000;
  start_time = GetTickCount();
  end_time = start_time + req;

  rc = WaitForSingleObject(dummy_event, req);
  now = GetTickCount();
  CloseHandle(dummy_event);
  rem = (rc == WAIT_TIMEOUT || now >= end_time) ? 0 : end_time - now;
  if (rc == WAIT_OBJECT_0)
    {
      errno = EINTR;
      res = -1;
    }

  if (rmtp)
    {
      rmtp->tv_sec = rem / 1000;
      rmtp->tv_nsec = (rem % 1000) * 1000000;
    }
  return res;
} 


unsigned int usleep(unsigned int useconds)
{
  struct timespec req;

  req.tv_sec = useconds / 1000000;
  req.tv_nsec = (useconds % 1000000) * 1000;

  return nanosleep(&req, 0);
}


unsigned int sleep(unsigned int seconds)
{
  Sleep(seconds * 1000);

  return 0;
}

#ifdef __cplusplus
}
#endif
