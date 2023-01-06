/*
   Copyright (C) 1999 T. Scott Dattalo

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

#include <iostream>
#include <string>

#include "command.h"
#include "misc.h"
#include "../src/gpsim_object.h"
#include "../src/processor.h"
#include "../src/stimuli.h"
#include "../src/value.h"
#include "cmd_attach.h"

cmd_attach attach;

static cmd_options cmd_attach_options[] =
{
  {nullptr, 0, 0}
};


cmd_attach::cmd_attach()
  : command("attach", nullptr)
{
  brief_doc = "Attach stimuli to nodes";

#ifndef Q
  long_doc = "attach node1 stimulus_1 [stimulus_2 stimulu_N]\n"
    "\tAttach is used to define connections between one or more stimulus\n"
    "\tand a node. One node and at least one stimulus must be specified, but\n"
    "\tin general two or more stimuli are used. Attach can be viewed as\n"
    "\twiring stimuli together, with the node acting as the wire. A stimulus\n"
    "\tis either a CPU or module I/O pin or a stimulus name.\n\n"
    "\tstimulus_n                 May be one of four forms:\n"
    "\tpin(<number>) or pin(<symbol>)\n"
    "\t    This refers to a pin of the current active CPU.\n"
    "\t    <number> is the pin number\n"
    "\t    <symbol> is an integer symbol whose value is a pin number\n"
    "\n"
    "\t<connection> or pin(<connection>)\n"
    "\t    These two forms are treated exactly the same\n"
    "\t            ( i.e. the pin() has no meaning).\n"
    "\t    <connection> is a stimulus name or an I/O pin name.\n"
    "\t            I/O pin name can be just the pin name for the CPU or\n"
    "\t                <module_name>.pin_name for a module\n"
    "\n"
    "\texample:\n"
    "\n"
    "\t**gpsim> load instructions_14bit.cod     # load code\n"
    "\t**gpsim> module library libgpsim_modules #load module lib\n"
    "\t**gpsim> module load usart U1            # create USART\n"
    "\t**gpsim> node n1                         # define a node\n"
    "\t**gpsim> node n2                         #define another node\n"
    "\t**gpsim> symbol TWO=2                    #define symbol with value 2\n"
    "\t**gpsim> attach n1 pin(1) pin(TWO)       #attach CPU pins 1 and 2 to n1\n"
    "\t**gpsim> attach n1 U1.RXPIN              #add usart pin to n1\n"
    "\t**gpsim> attach n2 portb0 pin(U1.TXPIN)  #connect portb0 to UASRT TX pin\n"
    "\t**gpsim> node                   # show results\n";
#else
  long_doc = "attach node1 stimulus_1 [stimulus_2 stimulu_N]\n"
    "\t  attach is used to define the connections between stimuli and nodes.\n"
    "\tAt least one node and one stimulus must be specified. If more stimuli\n"
    "\tare specified then they will all be attached to the node.\n\n"
    "\tstimulus_n                 May be one of four forms:\n"
    "\t  pin(<number>) or pin(<symbol>)\n"
    "\t             The single argument form refers to a pin of the currently\n"
    "\t             active cpu. The <number> argument defined the pin number\n"
    "\t             of active cpu. The <symbol> argument refers to the\n"
    "\t             name of the pin. If the <symbol> is scoped to a specific\n"
    "\t             attribute (i.e. MyProc.PORTA0) the pin of the specified\n"
    "\t             module will be attached.\n"
    "\t  pin(<module>, <number>) or pin(<module>, <symbol>)\n"
    "\t             The dual argument form refers to the pin of the specified\n"
    "\t             module.\n"
    "\t  <module>   Name of the module or string variable that contains the\n"
    "\t             module name.\n"
    "\t  <symbol>   A symbolic integer constant representing the pin number.\n"
    "\t  <number>   A literal integer value of the pin number.\n"
    "\n"
    "\texamples:\n"
    "\n"
    "\t  processor p16f627 P16\n"
    "\t  node pin2pin_test            // Define a new node.\n"
    "\t  attach pin2pin_test pin(porta4) pin(P16, portb0) // Different ways to \n"
    "\t  attach pin2pin_test pin(4) pin(0)                // connect two I/O\n"
    "\t  attach pin2pin_test pin(P16,portb0)              // pins to the node.\n"
    "\t  attach pin2pin_test pin(P16,0)\n"
    "\t  node                         // Display the new \"net list\".\n\n"
    "\tdeprecated:\n"
    "\t  attach pin2pin_test porta4 portb0\n";
#endif // Q

  op = cmd_attach_options;
}

extern void stimuli_attach(gpsimObject *pNode, gpsimObjectList_t *pPinList);

void cmd_attach::attach(gpsimObject *pNode, gpsimObjectList_t *pPinList)
{
  stimuli_attach(pNode, pPinList);

  //cout <<"deleting stimulus list\n";
  pPinList->clear();
  delete pPinList;
}

stimulus *toStimulus(int pinNumber)
{
  Processor *pMod = attach.GetActiveCPU();
  stimulus *pStim = pMod ? pMod->get_pin(pinNumber) : nullptr;
  if (!pStim)
  {
    std::cout << "unable to select pin " << pinNumber << '\n';
  //  GetUserInterface().DisplayMessage("unable to select pin\n");
  }
  return pStim;
}

stimulus *toStimulus(gpsimObject *pObj)
{
  Value *pVal = dynamic_cast<Value *>(pObj);
  if (!pVal) {
    std::cout << (pObj ? pObj->name() : "") << " cannot be converted to a pin number\n";
    return nullptr;
  }
  return toStimulus((int64_t)*pVal);
}
