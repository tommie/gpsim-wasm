/*
   Copyright (C) 2004 Borut Razem

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
Boston, MA 02111-1307, USA. */

#ifndef _FD2RAW_H
#define _FD2RAW_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

bool win32_fd_to_raw(int fd);

#ifdef __cplusplus
}
#endif

#endif
