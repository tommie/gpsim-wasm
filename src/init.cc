/*
   Copyright (C) 1998,1999 T. Scott Dattalo

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

#include "../config.h"
#include "trace.h"
#include "gpsim_interface.h"
#include "sim_context.h"

// in attribute.cc:
extern void init_attributes();
extern void destroy_attributes();

//==============================================================
// simulation_cleanup()
//
//  Called just before gpsim exits.
//

void simulation_cleanup(void)
{

  // Flush the log file (if there is one).

  GetTraceLog().close_logfile();
  destroy_attributes();

}


//========================================================================
// initialize_gpsim_core()
//
int initialize_gpsim_core()
{

  init_attributes();
  CSimulationContext::GetContext()->Initialize();
  return 0;
}
