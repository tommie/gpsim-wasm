/*
   Copyright (C) 1998,1999 T. Scott Dattalo

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

// Portions of this were obtained from octave.

#if !defined (__SCAN_H_)
#define __SCAN_H_

int scan_read (char *buf, unsigned max_size);

#ifdef YY_INPUT
#undef YY_INPUT
#endif
#define YY_INPUT(buf,result,max_size) \
  if ((result = scan_read (buf, max_size)) < 0) \
    YY_FATAL_ERROR ("gpsim_read () in flex scanner failed");

#endif
