/*
   Copyright (C) 2006 T. Scott Dattalo

This file is part of the libgpsim_dspic library of gpsim

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


/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include <iostream>
#include <stdio.h>
#include <string.h>

#include "../../config.h"    // get the definition for HAVE_GUI

#include "../modules.h"
//#include "dspic-processors.h"

/*
class Module_Types
{
public:

  char *names[2];
  Module * (*module_constructor) (void);
};
*/
Module_Types available_modules[] = {
  // No more modules
  { {0, 0}, 0}
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/********************************************************************************
 * mod_list - Display all of the modules in this library.
 *
 * This is a required function for gpsim compliant libraries.
 */

Module_Types * get_mod_list()
{
  return available_modules;
}


/********************************************************************************
 * mod_list - Display all of the modules in this library.
 *
 * This is a required function for gpsim compliant libraries.
 */

void mod_list()
{
  unsigned int number_of = sizeof(available_modules) / sizeof(Module_Types);
  size_t i, j, l;
  size_t k, longest;

  for (i = 0, longest = 0; i < number_of; i++) {
    k = strlen(available_modules[i].names[1]);

    if (k > longest) {
      longest = k;
    }
  }

  k = 0;

  do {
    for (i = 0; (i < 4) && (k < number_of); i++) {
      std::cout << available_modules[k].names[1];

      if (i < 3) {
        l = longest + 2 - strlen(available_modules[k].names[1]);

        for (j = 0; j < l; j++) {
          std::cout << ' ';
        }
      }

      k++;
    }

    std::cout << '\n';
  } while (k < number_of);
}


/************************************************************
 *
 * _init() - this is called when the library is opened.
 */

void init()
{
  //cout << "gpsim modules has been opened\n";
  printf("%s\n", __FUNCTION__);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

