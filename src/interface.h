/*
   Copyright (C) 1998 T. Scott Dattalo

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

#ifndef SRC_INTERFACE_H_
#define SRC_INTERFACE_H_

/*
 * interface.h
 */

#include <stdio.h>

#include "exports.h"
#include "modules.h"
#include "cmd_gpsim.h"

typedef enum _REGISTER_TYPE
{
    REGISTER_RAM,
    REGISTER_EEPROM
} REGISTER_TYPE;

struct file_context {
  char *name;           /* file name */
  FILE *file_ptr;
  int *line_seek;       /* an array of offsets into the file that point to
                         * the start of the source lines. */
  int max_line;
};

extern unsigned int gpsim_is_initialized;
LIBGPSIM_EXPORT void initialization_is_complete();

#define INVALID_VALUE 0xffffffff

void gpsim_set_bulk_mode(int flag);
extern const char *get_dir_delim(const char *path);

LIBGPSIM_EXPORT int initialize_gpsim_core();

#endif /* SRC_INTERFACE_H_ */
