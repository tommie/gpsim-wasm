/* -*- Mode: C++; c-file-style: "bsd"; comment-column: 40 -*- */
/*
   Copyright (C) 2000 Daniel Christian, T. Scott Dattalo

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

#ifndef SRC_FOPEN_PATH_H_
#define SRC_FOPEN_PATH_H_

#include "exports.h"

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

LIBGPSIM_EXPORT void InitSourceSearchAsSymbol();
LIBGPSIM_EXPORT void set_search_path(const char *path);
LIBGPSIM_EXPORT FILE *fopen_path(const char *filename, const char *perms);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // SRC_FOPEN_PATH_H_
