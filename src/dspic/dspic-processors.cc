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


#include "dspic-processors.h"
#include "dspic-registers.h"

#include <stdio.h>
#include <string>
#include <vector>

#include "../gpsim_time.h"
#include "../packages.h"
#include "../pic-instructions.h"
#include "../registers.h"
#include "../symbol.h"
#include "../trace.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//  Globals
//


using namespace dspic;
using namespace dspic_registers;

ProcessorConstructor pdsPic30F6010(dsPic30F6010::construct,
                                   "__30f6010", "dspic30f6010",  "30f6010", "30f610");


namespace dspic {

Trace *gTrace = nullptr;            // Points to gpsim's global trace object.
Cycle_Counter *gCycles = nullptr;  	// Points to gpsim's global cycle counter.

//-------------------------------------------------------------------
//
// dsPicProcessor -- constructor.

dsPicProcessor::dsPicProcessor(const char *_name, const char *desc)
  : Processor(_name, desc),
    m_stack(this),
    m_status(this, "status"), m_current_disasm_address(0)
{
  gTrace = &get_trace();
  gCycles = &get_cycles();
  pcl = new PCL(this, "PCL");
  pc = new dsPicProgramCounter(this, pcl);
}


//-------------------------------------------------------------------
//
// create
//
// Build the basic dsPicProcessor elements.

void dsPicProcessor::create()
{
  init_program_memory(program_memory_size());
  init_register_memory(register_memory_size() / 2);
  create_sfr_map();
  create_invalid_registers();
}


//-------------------------------------------------------------------
//
void dsPicProcessor::add_sfr_register(dspic_registers::dsPicRegister *pReg,
                                      unsigned int addr, const char *pName,
                                      RegisterValue *rv
                                     )
{
  if (!pReg) {
    return;
  }

  printf("adding sfr %s\n", pReg->name().c_str());
  pReg->set_cpu(this);

  if (addr < register_memory_size()) {
    registers[map_rm_address2index(addr)] = pReg;

    if (pName) {
      pReg->new_name(pName);
    }

    pReg->address = addr;
    pReg->alias_mask = 0;
    addSymbol(pReg);

    if (rv) {
      pReg->value = *rv;
      pReg->por_value = *rv;
    }

    RegisterValue rv = getWriteTT(addr);
    pReg->set_write_trace(rv);
    rv = getReadTT(addr);
    pReg->set_read_trace(rv);
  }
}


//------------------------------------------------------------------------
void dsPicProcessor::create_sfr_map()
{
  unsigned int j;
  // Initialize the General Purpose Registers:
  //add_file_registers(0xf80, 0xf7f, 0);
  unsigned int start_address = 0x0800 / 2;
  unsigned int end_address   = 0x27ff / 2;
  char str[100];

  for (j = start_address; j <= end_address; j++) {
    //The default register name is simply its address
    snprintf(str, sizeof(str), "R%03X", j);
    registers[j] = new dsPicRegister(this, str);
    registers[j]->address = j;
    RegisterValue rv = getWriteTT(j);
    registers[j]->set_write_trace(rv);
    rv = getReadTT(j);
    registers[j]->set_read_trace(rv);
  }

  RegisterValue porv(0, 0);

  for (j = 0; j < 16; j++) {
    char buff[16];
    snprintf(buff, 16, "W%u", j);
    // add_sfr_register(&W[j], j*2, buff,&porv);
  }

  add_sfr_register(pcl,   0x02e);
}


//------------------------------------------------------------------------
void dsPicProcessor::init_program_memory_at_index(unsigned int uIndex,
    const unsigned char *bytes,
    int nBytes)
{
  unsigned int unBytes = nBytes;

  for (unsigned int i = 0; i < unBytes; i += 4)
    Processor::init_program_memory_at_index(uIndex / 2 + (i / 4),
                                            (((unsigned int)bytes[i + 0]) << 0)  |
                                            (((unsigned int)bytes[i + 1]) << 8)  |
                                            (((unsigned int)bytes[i + 2]) << 16));
}


//------------------------------------------------------------------------
void dsPicProcessor::step_one(bool /* refresh */ )
{
  program_memory[pc->value]->execute();
}


void dsPicProcessor::step(unsigned int, bool /* refresh */ )
{
  program_memory[pc->value]->execute();
}


void dsPicProcessor::run(bool /* refresh */ )
{
}


void dsPicProcessor::finish()
{
}


void dsPicProcessor::step_cycle()
{
}


void dsPicProcessor::interrupt()
{
}


unsigned int dsPicProcessor::get_config_word(unsigned int)
{
  return 0xffffffff;
}


void dsPicProcessor::reset(RESET_TYPE )
{
}


//------------------------------------------------------------------------
// dsPIC30F6010
//------------------------------------------------------------------------
dsPic30F6010::dsPic30F6010(const char * /* _name */ , const char * /* desc */ )
{
}


Processor * dsPic30F6010::construct(const char *name)
{
  dsPic30F6010 *p = new dsPic30F6010(name);
  printf("Constructing a dspic 6010\n");
  p->create();
  globalSymbolTable().addModule(p);
  return p;
}


//------------------------------------------------------------
//
void dsPic30F6010::create()
{
  create_iopin_map();
  dsPicProcessor::create();
}


//------------------------------------------------------------
//
void dsPic30F6010::create_iopin_map()
{
  package = new Package(80);
}

};
