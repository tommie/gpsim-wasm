/*
   Copyright (C) 1999 T. Scott Dattalo

This file is part of the libgpsim library of gpsim

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see 
<http://www.gnu.org/licenses/lgpl-2.1.html>.
*/


#ifndef __TRACE_ORB_H__
#define __TRACE_ORB_H__

//------------------------------------
// *** KNOWN CHANGE ***
//
//  THIS FILE IS (OR SHOULD BE) TEMPORARY
//
// Here are a set of support functions called by the CLI. Eventually
// these will be replaced with an indirect interface such as CORBA.

#define TRACE_FILE_FORMAT_ASCII 0
#define TRACE_FILE_FORMAT_LXT 1

void trace_enable_logging(char *file=0, int mode=TRACE_FILE_FORMAT_ASCII);
#endif
