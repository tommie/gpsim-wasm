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

#include <stdio.h>

#include "dspic-registers.h"
#include "dspic-processors.h"
#include "../gpsim_time.h"
#include "../processor.h"
#include "../trace.h"

#if defined(PROPAGATE_UNKNOWNS)
bool gbPropagateUnknown = false;
#endif

namespace dspic {
  extern Cycle_Counter *gCycles;     // Points to gpsim's global cycle counter.
}

namespace dspic_registers {

  //------------------------------------------------------------------------
  // dsPicRegister

  unsigned int dsPicRegister::iMask = 0xffff;

  //--------------------------------------------------
  bool dsPicRegister::get_bit(unsigned int bit_number)
  {
    return ((value.get() >>  (bit_number & 0x0f)) & 1 ) ? true : false;
  }

  dsPicRegister::dsPicRegister(Processor *pCpu, const char *pName, const char *pDesc)
    : Register(pCpu, pName, pDesc)
  {
    value.data = 0;
    value.init = 0xffff;
    por_value.data = 0;
    por_value.init = 0xffff;
  }


  //--------------------------------------------------
  // PCL - low word of program counter.
  //--------------------------------------------------

  PCL::PCL(Processor *pCpu, const char *pName, const char *pDesc)
    : dsPicRegister(pCpu, pName, pDesc)
  {
    value = RegisterValue(0, 0);
    por_value = value;
  }

  void PCL::put(unsigned int new_value)
  {
    emplace_value_trace<trace::WriteRegisterEntry>();
    cpu_dsPic->pc->computed_goto(new_value);
  }

  void PCL::put_value(unsigned int new_value)
  {
    value.put(new_value & 0xffff);
    cpu_dsPic->pc->put_value( (cpu_dsPic->pc->get_value() & 0xffff0000) | value.get());
  }

  unsigned int PCL::get()
  {
    return (value.get() + 1) & 0xffff;
  }

  unsigned int PCL::get_value()
  {
    value.put(cpu_dsPic->pc->get_value() & 0xffff);
    return value.get();
  }

  //------------------------------------------------------------------------
  //
  // Program Counter
  //

  dsPicProgramCounter::dsPicProgramCounter(dspic::dsPicProcessor *pcpu, PCL *pPCL)
    : Program_Counter("pc", "Program Counter", pcpu),  m_pcl(pPCL), m_cpu(pcpu)
  {
  }

  //--------------------------------------------------
  // jump - update the program counter. All branching instructions
  //        except computed gotos and returns go through here.

  void dsPicProgramCounter::jump(unsigned int new_address)
  {
    emplace_trace<trace::BranchPCEntry>();
    value = new_address;
    value = (value >= memory_size) ? value - memory_size : value;

    m_pcl->value.put(value & 0xffff);

    dspic::gCycles->increment();
    dspic::gCycles->increment();
  }

  void dsPicProgramCounter::computed_goto(unsigned int new_address)
  {
    printf("dspic %s.\n", __FUNCTION__);

    emplace_trace<trace::SetPCEntry>(new_address);

    // Use the new_address and the cached pclath (or page select bits
    // for 12 bit cores) to generate the destination address:

    value = (new_address >> 1);
    value = (value >= memory_size) ? value - memory_size : value;

    // see Update pcl comment in Program_Counter::increment()
    m_pcl->value.put((value << 1) & 0xffff);

    // The instruction modifying the PCL will also increment the
    // program counter. So, pre-compensate the increment with a
    // decrement:

    value--;
    dspic::gCycles->increment();
  }

  void dsPicProgramCounter::put_value(unsigned int new_value)
  {
    printf("dspic program counter::%s. (0x%x)\n", __FUNCTION__, new_value);

    emplace_trace<trace::SetPCEntry>(new_value);

    value = new_value;
    value = (value >= memory_size) ? value - memory_size : value;
    m_pcl->value.put(value & 0xff);
  }

  unsigned int dsPicProgramCounter::get_value()
  {
    printf("dspic program counter::%s.\n", __FUNCTION__);

    return value * 2;
  }

  //--------------------------------------------------
  // increment - update the program counter. All non-branching
  // instructions pass through here.

  void dsPicProgramCounter::increment()
  {
    // Trace the value of the program counter before it gets changed.
    emplace_trace<trace::IncrementPCEntry>();
    value = value + 1;
    value = (value >= memory_size) ? value - memory_size : value;

    // Update pcl. Note that we don't want to pcl.put() because that
    // will trigger a break point if there's one set on pcl. (A read/write
    // break point on pcl should not be triggered by advancing the program
    // counter).

    m_pcl->value.put(value & 0xffff);
    dspic::gCycles->increment();
  }

  //----------------------------------------------------------------------
  // Stack
  //----------------------------------------------------------------------
  Stack::Stack(dspic::dsPicProcessor *pCpu)
    : m_cpu(pCpu)
  {
  }

  void Stack::push()
  {
    unsigned int pc = m_cpu->pc->get_value();
    unsigned int rm_size = m_cpu->register_memory_size() >> 1;
    unsigned int tos = m_cpu->W[15].get_value();
    unsigned int tos_index = tos >> 1;

    // Push the current PC onto the stack
    m_cpu->registers[tos_index       % rm_size]->put(pc & 0xffff);
    m_cpu->registers[(tos_index + 1) % rm_size]->put((pc >> 16) & 0xffff);
    m_cpu->W[15].put(tos + 4);
  }

  void Stack::pop()
  {
  }

  //----------------------------------------------------------------------
  // Status
  //----------------------------------------------------------------------
  Status::Status(Processor *pCpu, const char *pName, const char *pDesc)
    : dsPicRegister(pCpu, pName, pDesc)
  {
  }

  //----------------------------------------------------------------------
  // WRegister
  //----------------------------------------------------------------------
  WRegister::WRegister()
    : dsPicRegister(nullptr, nullptr, nullptr)
  {
  }
}
