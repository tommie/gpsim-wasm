/*
   Copyright (C) 1998-2000 Scott Dattalo

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

#include <assert.h>
#include <stdio.h>
#include <iostream>

#include "pic-registers.h"
#include "14bit-registers.h"
#include "clock_phase.h"
#include "gpsim_time.h"
#include "modules.h"
#include "pic-instructions.h"
#include "pic-processor.h"
#include "processor.h"
#include "tmr0.h"
#include "trace.h"
#include "ui.h"
#include "value.h"


//#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("0x%06" PRINTF_GINT64_MODIFIER "X %s() ",cycles.get(),__FUNCTION__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

//------------------------------------------------------------------------
// member functions for the Program_Counter base class
//------------------------------------------------------------------------
//

//--------------------------------------------------

Program_Counter::Program_Counter(const char *name, const char *desc, Module *pM)
  : Value(name, desc, pM)
{
  if (verbose) {
    std::cout << "pc constructor\n";
  }
}


Program_Counter::~Program_Counter()
{
  if (get_module()) {
    get_module()->removeSymbol(this);
  }
}


//--------------------------------------------------
// Report a bounds warning/error message
void Program_Counter::bounds_error ( const char * func, const char * test, unsigned int val )
{
  printf ( "%s PC=0x%x %s memory size 0x%x\n", func, val, test, memory_size);
}


//--------------------------------------------------
// increment - update the program counter. All non-branching instructions pass through here.
//

void Program_Counter::increment()
{
  Dprintf(("PC=0x%x\n", value));
  emplace_trace<trace::IncrementPCEntry>();
  value = (value + 1);

  if (value == memory_size) { // Some processors start at highest memory and roll over
    bounds_error ( __FUNCTION__, "==", value );
    value = 0;

  } else if (value > memory_size) { // assume this is a mistake
    bounds_error ( __FUNCTION__, ">", value );
  }

  // Update PCL sfr to reflect current PC
  update_pcl();
  cpu_pic->mCurrentPhase->setNextPhase(cpu_pic->mExecute1Cycle);
}


//--------------------------------------------------
// update_pcl - Updates the PCL from within the Program_Counter class.
// There is a separate method for this as the Program_Counter counts
// instructions (words) while the PCL can also point to bytes on
// 16 bit devices. So the PCL on 16-bit devices is always the double
// as the current Program_Counter
//

void Program_Counter::update_pcl()
{
  // For 12/14 bit devices the PCL will simply get set to the
  // current "value" of Program_Counter
  // Update pcl. Note that we don't want to pcl.put() because that
  // will trigger a break point if there's one set on pcl. (A read/write
  // break point on pcl should not be triggered by advancing the program
  // counter).
  cpu_pic->pcl->value.put(value & 0xff);
}


//--------------------------------------------------
// skip - Does the same thing that increment does, except that it records the operation
// in the trace buffer as a 'skip' instead of a 'pc update'.
//

void Program_Counter::skip()
{
  Dprintf(("PC=0x%x\n", value));
  emplace_trace<trace::SkipPCEntry>();

  if ((value + 2) >= memory_size) {
    bounds_error ( __FUNCTION__, ">=", value );
  } else {
    cpu_pic->mExecute2ndHalf->firstHalf(value + 2);
  }
}


//--------------------------------------------------
// start_skip - The next instruction is going to be skipped
//

void Program_Counter::start_skip()
{
}


//========================================================================

phaseExecute2ndHalf::phaseExecute2ndHalf(Processor *pcpu)
  : ProcessorPhase(pcpu), m_uiPC(0)
{
}


phaseExecute2ndHalf::~phaseExecute2ndHalf()
{
}


ClockPhase *phaseExecute2ndHalf::firstHalf(unsigned int uiPC)
{
  Dprintf(("first half of 2 cycle instruction new PC=0x%x\n", uiPC));
  ((pic_processor *)m_pcpu)->pc->value = uiPC;
  ((pic_processor *)m_pcpu)->pc->update_pcl();
  m_pcpu->mCurrentPhase->setNextPhase(this);
  return this;
}


ClockPhase *phaseExecute2ndHalf::advance()
{
  Dprintf(("second half of 2 cycle instruction\n"));
  m_pcpu->mCurrentPhase->setNextPhase(m_pcpu->mExecute1Cycle);
  get_cycles().increment();
  return m_pNextPhase;
}


//--------------------------------------------------
// jump - update the program counter. All branching instructions except computed gotos
//        and returns go through here.

void Program_Counter::jump(unsigned int new_address)
{
  Dprintf(("PC=0x%x new 0x%x\n", value, new_address));
  emplace_trace<trace::BranchPCEntry>();

  // Use the new_address and the cached pclath (or page select bits for 12 bit cores)
  // to generate the destination address:

  // see Update pcl comment in Program_Counter::increment()

  if (new_address >= memory_size) {
    bounds_error ( __FUNCTION__, ">=", new_address );
  } else {
    cpu_pic->mExecute2ndHalf->firstHalf(new_address);
  }
}


//--------------------------------------------------
// interrupt - update the program counter. Like a jump, except pclath is ignored.
//

void Program_Counter::interrupt(unsigned int new_address)
{
  Dprintf(("PC=0x%x 0x%x\n", value, new_address));
  emplace_trace<trace::BranchPCEntry>();

  if (new_address >= memory_size) {
    bounds_error ( __FUNCTION__, ">=", new_address );
  } else {
    cpu_pic->mExecute2ndHalf->firstHalf(new_address);
  }
}


//--------------------------------------------------
// computed_goto - update the program counter. Anytime the pcl register is written to
//                 by the source code we'll pass through here.
//

void Program_Counter::computed_goto(unsigned int new_address)
{
  Dprintf(("PC=0x%x new=0x%x\n", value, new_address));
  emplace_trace<trace::SetPCEntry>(new_address);
  // Use the new_address and the cached pclath (or page select bits for 12 bit cores)
  // to generate the destination address:
  value = new_address | cpu_pic->get_pclath_branching_modpcl() ;

  if (value >= memory_size) {
    bounds_error ( __FUNCTION__, ">=", value );
  }

  // Update PCL. As this is different for 12/14 and 16 bit devices
  // this will get handled by a method on its own so it is possible
  // to cope with different mappings PC-->PCL (direct, <<1, etc.)
  update_pcl();
  // The instruction modifying the PCL will also increment the program counter.
  // So, pre-compensate the increment with a decrement:
  value--;
  // The computed goto is a 2-cycle operation. The first cycle occurs within
  // the instruction (i.e. via the ::increment() method). The second cycle occurs
  // here:
  cpu_pic->mExecute2ndHalf->advance();
}


//--------------------------------------------------
// new_address - write a new value to the program counter. All returns pass through here.
//

void Program_Counter::new_address(unsigned int new_address)
{
  Dprintf(("PC=0x%x new 0x%x\n", value, new_address & 0xffff));
  emplace_trace<trace::SetPCEntry>(new_address);

  if (new_address >= memory_size) {
    bounds_error ( __FUNCTION__, ">=", new_address );
  } else {
    cpu_pic->mExecute2ndHalf->firstHalf(new_address);
  }
}


//--------------------------------------------------
// get_next - get the next address that is just pass the current one
//            (used by 'call' to obtain the return address)

unsigned int Program_Counter::get_next()
{
  unsigned int new_address = value + cpu_pic->program_memory[value]->instruction_size();

  if (new_address >= memory_size) {
    bounds_error ( __FUNCTION__, ">=", new_address );
  }

  return new_address;
}


//--------------------------------------------------
// put_value - Change the program counter without affecting the cycle counter
//             (This is what's called if the user changes the pc.)

void Program_Counter::put_value(unsigned int new_value)
{
  // FIXME
#define PCLATH_MASK              0x1f
  Dprintf(("PC=0x%x new 0x%x\n", value, new_value & 0xffff));
  emplace_trace<trace::SetPCEntry>(new_value);

  if (new_value >= memory_size) {
    bounds_error ( __FUNCTION__, ">=", new_value );
  }

  value = new_value;
  cpu_pic->pcl->value.put(value & 0xff);
  cpu_pic->pclath->value.put((new_value >> 8) & PCLATH_MASK);
}


void Program_Counter::reset()
{
  value = reset_address;
  value = (value >= memory_size) ? value - memory_size : value;
  cpu_pic->mExecute2ndHalf->firstHalf(value);
}


//========================================================================
//
// Helper registers
//

PCHelper::PCHelper(Processor *pCpu, ProgramMemoryAccess *new_pma)
  : Register(pCpu, "PC", "Program Counter"), pma(new_pma)
{
  assert(pma);
}


void PCHelper::put_value(unsigned int new_value)
{
  //  if(pma)
  pma->set_PC(new_value);
}


unsigned int PCHelper::get_value()
{
  // if(pma)
  return pma->get_PC();
  //return 0;
}


//--------------------------------------------------
// member functions for the OPTION base class
//--------------------------------------------------
OPTION_REG::OPTION_REG(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc)
{
  por_value = RegisterValue(0xff, 0);
  wdtr_value = RegisterValue(0xff, 0);
  // The chip reset will place the proper value here.
  value = RegisterValue(0, 0);  // por_value;
}


// make sure intial por_value does it's stuff
void OPTION_REG::initialize()
{
  cpu_pic->tmr0.new_prescale();
  cpu_pic->wdt->set_postscale((value.get() & PSA) ? (value.get() & (PS2 | PS1 | PS0)) : 0);
  cpu_pic->option_new_bits_6_7(value.get() & (T0CS | BIT6 | BIT7));
}


void OPTION_REG::put(unsigned int new_value)
{
  emplace_value_trace<trace::WriteRegisterEntry>();
  unsigned int old_value = value.get();
  value.put(new_value);

  // First, check the tmr0 clock source bit to see if we are  changing from
  // internal to external (or vice versa) clocks.
  //if( (value ^ old_value) & T0CS)
  //    cpu_pic->tmr0.new_clock_source();

  // %%%FIX ME%%% - can changing the state of TOSE cause the timer to
  // increment if tmr0 is being clocked by an external clock?

  // Now check the rest of the tmr0 bits.
  if ((value.get() ^ old_value) & (T0CS | T0SE | PSA | PS2 | PS1 | PS0)) {
    cpu_pic->tmr0.new_prescale();
  }

  if ((value.get() ^ old_value) & (PSA | PS2 | PS1 | PS0)) {
    cpu_pic->wdt->set_postscale((value.get() & PSA) ? (value.get() & (PS2 | PS1 | PS0)) : 0);
  }

  if ((value.get() ^ old_value) & (T0CS | BIT6 | BIT7)) {
    cpu_pic->option_new_bits_6_7(value.get() & (T0CS | BIT6 | BIT7));
  }
}


void OPTION_REG::reset(RESET_TYPE)
{
  putRV(por_value);
}


// On 14bit enhanced cores the prescaler does not affect the watchdog
OPTION_REG_2::OPTION_REG_2(Processor *pCpu, const char *pName, const char *pDesc)
  : OPTION_REG(pCpu, pName, pDesc)
{
}


void OPTION_REG_2::initialize()
{
  cpu_pic->tmr0.new_prescale();
  cpu_pic->option_new_bits_6_7(value.get() & (T0CS | BIT6 | BIT7));
}


void OPTION_REG_2::put(unsigned int new_value)
{
  emplace_value_trace<trace::WriteRegisterEntry>();
  unsigned int old_value = value.get();
  value.put(new_value);

  // First, check the tmr0 clock source bit to see if we are  changing from
  // internal to external (or vice versa) clocks.
  //if( (value ^ old_value) & T0CS)
  //    cpu_pic->tmr0.new_clock_source();

  // %%%FIX ME%%% - can changing the state of TOSE cause the timer to
  // increment if tmr0 is being clocked by an external clock?

  // Now check the rest of the tmr0 bits.
  if ((value.get() ^ old_value) & (T0CS | T0SE | PSA | PS2 | PS1 | PS0)) {
    cpu_pic->tmr0.new_prescale();
  }

  if ((value.get() ^ old_value) & (T0CS | BIT6 | BIT7)) {
    cpu_pic->option_new_bits_6_7(value.get() & (T0CS | BIT6 | BIT7));
  }
}
