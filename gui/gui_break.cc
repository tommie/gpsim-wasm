/*
   Copyright (C) 1998,1999,2000,2001
   T. Scott Dattalo and Ralf Forsberg

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


#include <cstdio>

#include <config.h>

#ifdef HAVE_GUI

#include "../src/processor.h"
#include "../src/ui.h"
#include "gui.h"
#include "gui_processor.h"
#include "gui_src.h"

class gpsimObject;

class linkXREF : public CrossReferenceToGUI
{
public:
  GUI_Processor *gp;

  virtual void Update(int /* new_value */) override
  {
    int address;

    if (!gp) { printf("gp == null in linkXREF\n"); return; }

    address = *(int *)data;

    if (gp->source_browser)
      gp->source_browser->UpdateLine(address);

    if (gp->program_memory)
      gp->program_memory->UpdateLine(address);
  }
};

/*
 * link_src_to_gpsim
 *
 * After a new program has been loaded by gpsim, this routine is called
 * so that the gui can create links to it. This consists of creating
 * a 'cross_reference' structure and attaching it to each pic instruction.
 * The information in the cross_reference structure is a pointer to the
 * gp.
 */

void link_src_to_gpsim(GUI_Processor *gp)
{
  if (gp) {
    // Create a cross reference between the pic's program memory and the gui.
    int pm_size =  gp->cpu->program_memory_size();

    if (verbose) {
      printf("link_src_to_gpsim\n");
      printf(" processor pma = %d\n", pm_size);
    }

    for (int i = 0; i < pm_size; i++) {
      linkXREF *cross_reference = new linkXREF();

      cross_reference->gp = gp;
      int *address = new int;
      *address = gp->cpu->map_pm_index2address(i);

      cross_reference->data = (gpsimObject*) address;
      gp->cpu->pma->assign_xref(*address, (CrossReferenceToGUI *) cross_reference);
    }
  }
}

#endif //HAVE_GUI
