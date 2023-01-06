/*
   Copyright (C) 1998,1999,2000 T. Scott Dattalo

This file is part of the libgpsim_modules library of gpsim

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

/*
  gpsim_modules.cc

  gpsim supports modules, or thingies, that are not part of gpsim proper.
This is to say, that the modules are not compiled and linked with the
core gpsim software. Instead, they are compiled and linked separately and
then dynamically loaded by gpsim. This approach provides a flexibility
to the user to create customized objects for simulation purposes. The
big benefit of course, is that the user doesn't have to get bogged down
in to the nitty-gritty details of the way gpsim is designed. The templates
provided here can serve as a relatively simple example of how one may
go about creating customized modules.

Please see the README.MODULES for more details on how modules are intended
to be used.

Here are a list of functions that a gpsim compliant module library should
support:

  void mod_list() - Prints a list of the modules in a library
  Module_Types * get_mod_list()  - Obtain pointer to the list of modules
*/

/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include <iostream>
#include <cstring>

#include <cstdio>

#include "../config.h"    // get the definition for HAVE_GUI

#include "../src/modules.h"

#include "resistor.h"
#include "usart.h"
#ifndef _WIN32
//#include "paraface.h"
#endif
#include "switch.h"
#include "logic.h"
#ifdef HAVE_GUI
#include "led.h"
#include "push_button.h"
#include "video.h"
#include "encoder.h"
#endif
#include "m_stimuli.h"
#include "ttl.h"
#include "i2c-eeprom.h"
#include "i2c.h"
#include "i2c2par.h"


Module_Types available_modules[] = {

#ifndef _WIN32
  // Parallel port interface
  /*
    TSD - removed 16APR06 - The parallel port interface uses the deprecated
    IOPORT class.
  { {"parallel_interface",         "paraface"}, Paraface::construct},
  */
#endif

  // Switch
  { {"switch",         "sw"}, Switches::Switch::construct},

  // Logic
  { {"and2", "and2"}, AND2Gate::construct},
  { {"or2",  "or2"},  OR2Gate::construct},
  { {"xor2", "xor2"}, XOR2Gate::construct},
  { {"not",  "not"},  NOTGate::construct},

#ifdef HAVE_GUI
  // Leds
  { {"led_7segments", "led7s"}, Leds::Led_7Segments::construct},
  { {"led", "led"}, Leds::Led::construct},
  { {"push_button",      "pb"},   PushButton::construct },
#endif
  { {"PortStimulus",     "ps"},   ExtendedStimuli::PortStimulus::construct8 },
/* these do not make sense RRR 20/9/2020 as registers are 8 bits wide
  { {"PortStimulus16",   "ps16"},   ExtendedStimuli::PortStimulus::construct16 },
  { {"PortStimulus32",   "ps32"},   ExtendedStimuli::PortStimulus::construct32 },
*/
  { {"pullup",           "pu"},   PullupResistor::pu_construct },
  { {"pulldown",         "pd"},   PullupResistor::pd_construct },
  { {"pulsegen",         "pg"},   ExtendedStimuli::PulseGen::construct },
  { {"FileRecorder",     "fr"},   ExtendedStimuli::FileRecorder::construct },
  { {"FileStimulus",     "fs"},   ExtendedStimuli::FileStimulus::construct },

#ifdef HAVE_GUI
  // PGS added back 23MAY10
  // TSD Removed 17APR06
  // Video
  { {"PAL_video", "video"}, Video::construct},

  // Encoder
  { {"Encoder", "encoder"}, Encoder::construct},
#endif
  // USART
  { {"usart",            "usart"}, USARTModule::USART_construct},

  // TTL devices
  { {"TTL165", "ttl165"}, TTL::TTL165::construct},
  { {"TTL377", "ttl377"}, TTL::TTL377::construct},
  { {"TTL595", "ttl595"}, TTL::TTL595::construct},

  // I2c EEPROM
  { {"I2C-EEPROM2k", "e24xx024"}, I2C_EEPROM_Modules::I2C_EE_Module::construct_2k},
  { {"I2C-EEPROM16k", "e24xx16b"}, I2C_EEPROM_Modules::I2C_EE_Module::construct_16k},
  { {"I2C-EEPROM256k", "e24xx256"}, I2C_EEPROM_Modules::I2C_EE_Module::construct_256k},
  { {"i2cmaster", "I2CMaster"},   I2C_Module::I2CMaster::construct },
  { {"i2c2par", "I2C2PAR"},   I2C2PAR_Modules::i2c2par::construct },

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

Module_Types *get_mod_list()
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
  size_t number_of = sizeof(available_modules) / sizeof(Module_Types);
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


/************************************************************
 *
 * _fini() - this is called when the library is closed.
 */
void fini()
{
  //cout << "gpsim modules has been closed\n";
  printf("%s\n", __FUNCTION__);
}


void test()
{
  //cout << "This is a test\n";
  printf("%s\n", __FUNCTION__);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
