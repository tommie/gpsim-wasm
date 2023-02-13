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

#include "../pic-instructions.h"

#include "dspic-processors.h"
#include "dspic-instructions.h"

#include <assert.h>
#include <stdio.h>
#include <string>
#include "../gpsim_object.h"
#include "dspic-registers.h"

class Cycle_Counter;
class Processor;
class Trace;

using namespace dspic;
using namespace dspic_instructions;

namespace dspic {
  extern Cycle_Counter *gCycles;     // Points to gpsim's global cycle counter.
}


struct dsPicInstructionConstructor {
  unsigned int inst_mask;
  unsigned int opcode;
  instruction * (*inst_constructor) (Processor *cpu, unsigned int inst, unsigned int addr);
};


struct dsPicInstructionConstructor op_dsPic[] = {

  { 0xff8000,  0xb00000,  ADDR::construct },  // Lit
  { 0xf80000,  0x400000,  ADDR::construct },  // Wb + Lit -> Wd
  { 0xff7fff,  0xcb0000,  ADD::construct },  // add accumulators

  { 0xff8000,  0xb48000,  ADDC::construct },  // f to W
  { 0xff8000,  0xb08000,  ADDC::construct },  // Lit
  { 0xf80000,  0x480000,  ADDC::construct },  // Wb + Lit -> Wd

  { 0xff8000,  0xb60000,  AND::construct },  // f to W
  { 0xff8000,  0xb20000,  AND::construct },  // Lit
  { 0xf80000,  0x600060,  AND::construct },  // Wb + Lit -> Wd
  { 0xf80000,  0x600000,  AND::construct },

  { 0xff8000,  0xd58000,  ASR::construct },  // f to W
  { 0xff8000,  0xd18000,  ASR::construct },  // Lit
  { 0xff8070,  0xde8000,  ASR::construct },  // Wb + Lit -> Wd

  { 0xff0000,  0xa90000,  BCLR::construct },
  { 0xff0b80,  0xa10000,  BCLR::construct },

  { 0xf00000,  0x300000,  BRA::construct },   // branch literal offset
  { 0xfffff0,  0x016000,  BRA::construct },   // computed branch
  { 0xfc0000,  0x0c0000,  BRA::construct },   // branch on accumulator state

  { 0xff0000,  0xa80000,  BSET::construct },
  { 0xff0b80,  0xa00000,  BSET::construct },

  { 0xff0780,  0xad0000,  BSW::construct },

  { 0xff0000,  0xaa0000,  BTG::construct },

  { 0xff0000,  0xaf0000,  BTS::construct },
  { 0xff0000,  0xab0000,  BTST::construct },
  { 0xff0780,  0xa30000,  BTST::construct },
  { 0xff0780,  0xa50000,  BTST::construct },

  { 0xff0000,  0xac0000,  BTSTS::construct },
  { 0xff0780,  0xa40000,  BTSTS::construct },

  { 0xff0001,  0x020000,  CALL::construct },
  { 0xfffff0,  0x010000,  CALL::construct },

  { 0xff8000,  0xef0000,  CLR::construct },
  { 0xff807f,  0xeb0000,  CLR::construct },
  { 0xff4000,  0xc30000,  CLR::construct },

  { 0xffffff,  0xfe6000,  CLRWDT::construct },

  { 0xff8000,  0xee8000,  COM::construct },
  { 0xff8000,  0xea8000,  COM::construct },

  { 0xffa000,  0xe30000,  CP::construct },
  { 0xff83e0,  0xe10060,  CP::construct },
  { 0xff8380,  0xe10000,  CP::construct },

  { 0xffa000,  0xe20000,  CP0::construct },
  { 0xfffb80,  0xe00000,  CP0::construct },

  { 0xffa000,  0xe38000,  CPB::construct },
  { 0xff83e0,  0xe18060,  CPB::construct },
  { 0xff8380,  0xe18000,  CPB::construct },

  { 0xfe03f0,  0xe60000,  CPS::construct },

  { 0xfffff0,  0xfd4000,  DAW::construct },

  { 0xff8000,  0xed0000,  DEC::construct },
  { 0xff8000,  0xe90000,  DEC::construct },

  { 0xff8000,  0xed8000,  DEC::construct }, // DEC2
  { 0xff8000,  0xe98000,  DEC::construct }, // DEC2

  { 0xffc000,  0xfc0000,  DISI::construct },

  { 0xff8030,  0xd80000,  DIV::construct },
  { 0xff8030,  0xd88000,  DIV::construct },
  { 0xff87f0,  0xd90000,  DIV::construct },

  { 0xffc000,  0x080000,  DO::construct },
  { 0xfffff0,  0x088000,  DO::construct },

  { 0xfc4c03,  0xf04003,  ED::construct },
  { 0xfc4c03,  0xf04002,  ED::construct },

  { 0xfff870,  0xfd0000,  EXCH::construct },

  { 0xfff800,  0xdf0000,  FB::construct },  // FBCL
  { 0xfff800,  0xcf8000,  FB::construct },  // FF1L
  { 0xfff800,  0xcf0000,  FB::construct },  // FF1R

  { 0xff0000,  0x040000,  GOTO::construct },
  { 0xfffff0,  0x014000,  GOTO::construct },

  { 0xff8000,  0xec0000,  DEC::construct },
  { 0xff8000,  0xe80000,  DEC::construct },

  { 0xff8000,  0xec8000,  DEC::construct }, // INC2
  { 0xff8000,  0xe88000,  DEC::construct }, // DEC2

  { 0xff8000,  0xb70000,  IOR::construct },  // f to W
  { 0xff8000,  0xb30000,  IOR::construct },  // Lit
  { 0xf80000,  0x700000,  IOR::construct },  // Wb + Lit -> Wd

  { 0xff0000,  0xca0000,  LAC::construct },
  { 0xffc001,  0xfa0000,  LNK::construct },

  { 0xff8000,  0xd50000,  LSR::construct },  // Note LSR is similar to ASR
  { 0xff8000,  0xd10000,  LSR::construct },  // Lit
  { 0xff8070,  0xde0000,  LSR::construct },  // Wb + Lit -> Wd

  { 0xf84000,  0xc00000,  MAC::construct },
  { 0xfc4003,  0xf00000,  MAC::construct },

  { 0xff8000,  0xbf8000,  MOV::construct },
  { 0xffa000,  0xb7a000,  MOV::construct },
  { 0xf80000,  0x800000,  MOV::construct },
  { 0xf80000,  0x880000,  MOV::construct },
  { 0xfff000,  0xb3c000,  MOV::construct },  //MOV.B
  { 0xf00000,  0x200000,  MOV::construct },
  { 0xf80000,  0x900000,  MOV::construct },
  { 0xf80000,  0x980000,  MOV::construct },
  { 0xf80000,  0x780000,  MOV::construct },
  { 0xfff880,  0xbe0000,  MOV::construct },  //MOV.D
  { 0xffc071,  0xbe8000,  MOV::construct },  //MOV.D

  { 0xff4000,  0xc70000,  MOVSAC::construct },

  { 0xf84003,  0xc00003,  MPY::construct },  // degenerate to MAC
  { 0xf84003,  0xc04003,  MPY::construct },  // MPY.N and MSC

  { 0xffa000,  0xbc0000,  MUL::construct },
  { 0xff8000,  0xb98000,  MUL::construct },  //MUL.SS
  { 0xff8060,  0xb90000,  MUL::construct },  //MUL.SU
  { 0xff8000,  0xb88000,  MUL::construct },  //MUL.US
  { 0xff8060,  0xb80000,  MUL::construct },  //MUL.UU

  { 0xff8000,  0xee0000,  NEG::construct },
  { 0xff8000,  0xea0000,  NEG::construct },
  { 0xff7fff,  0xcb1000,  NEG::construct },

  { 0xff0000,  0x000000,  NOP::construct },
  { 0xff0000,  0xff0000,  NOP::construct },

  { 0xff0001,  0xf90000,  POP::construct },
  { 0xf8407f,  0x78004f,  POP::construct },
  { 0xfff8ff,  0xbe004f,  POP::construct },
  { 0xffffff,  0xfe8000,  POP::construct },  // POP.S

  { 0xff0001,  0xf80000,  PUSH::construct },
  { 0xf87f80,  0x781f80,  PUSH::construct },
  { 0xfffff1,  0xbe9f80,  PUSH::construct },
  { 0xffffff,  0xfea000,  PUSH::construct },  // PUSH.S

  { 0xfffffe,  0xfe4000,  PWRSAV::construct },

  { 0xff0000,  0x070000,  RCALL::construct },
  { 0xfffff0,  0x012000,  RCALL::construct },

  { 0xffc000,  0x090000,  REPEAT::construct },
  { 0xfffff0,  0x098000,  REPEAT::construct },

  { 0xffffff,  0xfe0000,  RESET::construct },

  { 0xffffff,  0x064000,  RET::construct },  // RETFIE
  { 0xff8000,  0x050000,  RET::construct },  // RETLW
  { 0xffffff,  0x060000,  RET::construct },  // RETURN

  { 0xff8000,  0xd68000,  ROT::construct },  // RLC
  { 0xff8000,  0xd28000,  ROT::construct },  // RLC
  { 0xff8000,  0xd60000,  ROT::construct },  // RLNC
  { 0xff8000,  0xd20000,  ROT::construct },  // RLNC
  { 0xff8000,  0xd78000,  ROT::construct },  // RRC
  { 0xff8000,  0xd38000,  ROT::construct },  // RRC
  { 0xff8000,  0xd70000,  ROT::construct },  // RNC
  { 0xff8000,  0xd30000,  ROT::construct },  // RNC

  { 0xff0000,  0xcc0000,  SAC::construct },
  { 0xff0000,  0xcd0000,  SAC::construct },  // SAC.R

  { 0xfff800,  0xfb0000,  SE::construct },

  { 0xff8000,  0xef8000,  SETM::construct },
  { 0xff807f,  0xeb8000,  SETM::construct },

  { 0xff7fc0,  0xc80040,  SFTAC::construct },
  { 0xff7ff0,  0xc80000,  SFTAC::construct },

  { 0xff8000,  0xd40000,  SL::construct },
  { 0xff8000,  0xd00000,  SL::construct },
  { 0xff8070,  0xdd0040,  SL::construct },
  { 0xff8070,  0xdd0000,  SL::construct },

  { 0xff8000,  0xb50000,  SUB::construct },
  { 0xff8000,  0xb10000,  SUB::construct },
  { 0xf80000,  0x500000,  SUB::construct },

  { 0xff7fff,  0xcb3000,  SUB::construct },

  { 0xff8000,  0xb58000,  SUB::construct }, //SUBB
  { 0xff8000,  0xb18000,  SUB::construct },
  { 0xf80000,  0x580000,  SUB::construct },

  { 0xff8000,  0xbd8000,  SUB::construct }, //SUBBR
  { 0xf80000,  0x180000,  SUB::construct },

  { 0xff8000,  0xbd0000,  SUB::construct }, //SUBR
  { 0xf80000,  0x100000,  SUB::construct },

  { 0xffbff0,  0xfd8000,  SWAP::construct },

  { 0xff8000,  0xba8000,  TBLRD::construct }, //TBLRDH
  { 0xff8000,  0xba0000,  TBLRD::construct }, //TBLRDL

  { 0xff8000,  0xbb8000,  TBLWT::construct }, //TBLWTH
  { 0xff8000,  0xbb0000,  TBLWT::construct }, //TBLWTL

  { 0xffffff,  0xfa8000,  ULNK::construct },

  { 0xff8000,  0xb68000,  XOR::construct },  // f to W
  { 0xff8000,  0xb28000,  XOR::construct },  // Lit
  { 0xf80000,  0x680000,  XOR::construct },  // Wb + Lit -> Wd

  { 0xffc000,  0xfb8000,  ZE::construct }

};

const int NUM_OP_DSPIC = sizeof(op_dsPic) / sizeof(op_dsPic[0]);


instruction * dsPicProcessor::disasm(unsigned int address, unsigned int inst)
{
    for (int i = 0; i < NUM_OP_DSPIC; i++)
    {
        if ((op_dsPic[i].inst_mask & inst) == op_dsPic[i].opcode)
            return op_dsPic[i].inst_constructor(this, inst, address);
    }

    return new invalid_instruction(this, inst, address);
}



namespace dspic_instructions
{
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //
  //     Addressing Modes
  //
  //  The various addressing modes for the dspic instructions are
  // first defined.
  //
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  //--------------------------------------------------
  const RegisterValue AddressingMode::m_unknown = RegisterValue(0,0xffff);

  AddressingMode::AddressingMode(dspic::dsPicProcessor *cpu,
				 unsigned int addr)
    : m_cpu(cpu), m_mode(0), m_addr(addr)

  {
  }
  AddressingMode * AddressingMode::construct(dspic::dsPicProcessor *new_cpu,
					    unsigned int new_mode,
					    unsigned int addr)
  {

    switch (new_mode&7) {

    case eDirect:
      return new RegDirectAddrMode(new_cpu, addr);
    case eIndirect:
      return new RegIndirectAddrMode(new_cpu, addr);
    case eIndirectPostDec:
      return new RegIndirectPostDecAddrMode(new_cpu, addr);
    case eIndirectPostInc:
      return new RegIndirectPostIncAddrMode(new_cpu, addr);
    case eIndirectPreDec:
      return new RegIndirectPreDecAddrMode(new_cpu, addr);
    case eIndirectPreInc:
      return new RegIndirectPreIncAddrMode(new_cpu, addr);
      //case eIndirectRegOffset:
      //case eIndirectRegOffset_:
      break;
    case eLiteral:
    case eLiteral_:
      return new LiteralAddressingMode(new_cpu, addr&0x1f);
    }

    return 0;
  }


  //--------------------------------------------------


  LiteralAddressingMode::LiteralAddressingMode(dspic::dsPicProcessor *cpu,
					       unsigned int addr)
    : AddressingMode(cpu, addr),
      m_rv(addr,0)
  {
  }
  char *LiteralAddressingMode::name(char *buff,int len)
  {
    if (buff)
      snprintf(buff,len,"#0x%x",m_addr);
    return buff;
  }
  //--------------------------------------------------
  RegisterAddressingMode::RegisterAddressingMode(dspic::dsPicProcessor *cpu,
						 unsigned int addr,
						 const char *cPformat)
    : AddressingMode(cpu, addr&0xf),
      m_cPformat(cPformat)
  {
  }

  char *RegisterAddressingMode::name(char *buff,int len)
  {
    if (buff)
      snprintf(buff,len,m_cPformat,m_cpu->registers[m_addr]->name().c_str());
    return buff;
  }

  //--------------------------------------------------
  RegDirectAddrMode::RegDirectAddrMode(dspic::dsPicProcessor *cpu,
				       unsigned int addr)
    : RegisterAddressingMode(cpu, addr,"%s")
  {
  }
  RegisterValue RegDirectAddrMode::get()
  {
    return m_cpu->registers[m_addr]->getRV();
  }
  void RegDirectAddrMode::put(RegisterValue &n_rv)
  {
    m_cpu->registers[m_addr]->putRV(n_rv);
  }

  //--------------------------------------------------
  RegIndirectAddrMode::RegIndirectAddrMode(dspic::dsPicProcessor *cpu,
					   unsigned int addr)
    : RegisterAddressingMode(cpu, addr,"[%s]")
  {
  }
  RegisterValue RegIndirectAddrMode::get()
  {
    RegisterValue rv = m_cpu->registers[m_addr]->getRV();
    return rv.init ? m_unknown : m_cpu->registers[rv.data]->getRV();
  }
  void RegIndirectAddrMode::put(RegisterValue &n_rv)
  {
    RegisterValue rv = m_cpu->registers[m_addr]->getRV();
    if (rv.init == 0)
      m_cpu->registers[rv.data]->putRV(n_rv);
  }
  //--------------------------------------------------
  RegIndirectPostDecAddrMode::RegIndirectPostDecAddrMode(dspic::dsPicProcessor *cpu,
							 unsigned int addr)
    : RegisterAddressingMode(cpu, addr,"[%s--]")
  {
  }
  RegisterValue RegIndirectPostDecAddrMode::get()
  {
    RegisterValue rv = m_cpu->registers[m_addr]->getRV();
    RegisterValue retRV = rv.init ? m_unknown : m_cpu->registers[rv.data]->getRV();
    rv.data = (rv.data-2) & 0xffff;
    m_cpu->registers[m_addr]->putRV(rv);
    return retRV;
  }
  void RegIndirectPostDecAddrMode::put(RegisterValue &n_rv)
  {
    RegisterValue rv = m_cpu->registers[m_addr]->getRV();
    if (rv.init == 0)
      m_cpu->registers[rv.data]->putRV(n_rv);
    rv.data = (rv.data-2) & 0xffff;
    m_cpu->registers[m_addr]->putRV(rv);
  }
  //--------------------------------------------------
  RegIndirectPostIncAddrMode::RegIndirectPostIncAddrMode(dspic::dsPicProcessor *cpu,
							 unsigned int addr)
    : RegisterAddressingMode(cpu, addr,"[%s++]")
  {
  }
  RegisterValue RegIndirectPostIncAddrMode::get()
  {
    RegisterValue rv = m_cpu->registers[m_addr]->getRV();
    RegisterValue retRV = rv.init ? m_unknown : m_cpu->registers[rv.data]->getRV();
    rv.data = (rv.data+2) & 0xffff;
    m_cpu->registers[m_addr]->putRV(rv);
    return retRV;
  }
  void RegIndirectPostIncAddrMode::put(RegisterValue &n_rv)
  {
    RegisterValue rv = m_cpu->registers[m_addr]->getRV();
    if (rv.init == 0)
      m_cpu->registers[rv.data]->putRV(n_rv);
    rv.data = (rv.data+2) & 0xffff;
    m_cpu->registers[m_addr]->putRV(rv);
  }
  //--------------------------------------------------
  RegIndirectPreDecAddrMode::RegIndirectPreDecAddrMode(dspic::dsPicProcessor *cpu,
						       unsigned int addr)
    : RegisterAddressingMode(cpu, addr,"[--%s]")
  {
  }
  RegisterValue RegIndirectPreDecAddrMode::get()
  {
    RegisterValue rv = m_cpu->registers[m_addr]->getRV();
    rv.data = (rv.data-2) & 0xffff;
    m_cpu->registers[m_addr]->putRV(rv);
    RegisterValue retRV = rv.init ? m_unknown : m_cpu->registers[rv.data]->getRV();
    return retRV;
  }
  void RegIndirectPreDecAddrMode::put(RegisterValue &n_rv)
  {
    RegisterValue rv = m_cpu->registers[m_addr]->getRV();
    rv.data = (rv.data-2) & 0xffff;
    m_cpu->registers[m_addr]->putRV(rv);
    if (rv.init == 0)
      m_cpu->registers[rv.data]->putRV(n_rv);
  }
  //--------------------------------------------------
  RegIndirectPreIncAddrMode::RegIndirectPreIncAddrMode(dspic::dsPicProcessor *cpu,
						       unsigned int addr)
    : RegisterAddressingMode(cpu, addr,"[++%s]")
  {
  }
  RegisterValue RegIndirectPreIncAddrMode::get()
  {
    RegisterValue rv = m_cpu->registers[m_addr]->getRV();
    rv.data = (rv.data+2) & 0xffff;
    m_cpu->registers[m_addr]->putRV(rv);
    RegisterValue retRV = rv.init ? m_unknown : m_cpu->registers[rv.data]->getRV();
    return retRV;
  }
  void RegIndirectPreIncAddrMode::put(RegisterValue &n_rv)
  {
    RegisterValue rv = m_cpu->registers[m_addr]->getRV();
    rv.data = (rv.data+2) & 0xffff;
    m_cpu->registers[m_addr]->putRV(rv);
    if (rv.init == 0)
      m_cpu->registers[rv.data]->putRV(n_rv);
  }
  //--------------------------------------------------
  MultiWordInstruction::MultiWordInstruction(Processor *new_cpu,
						 unsigned int new_opcode,
						 unsigned int addr)
    : instruction(new_cpu, new_opcode, addr),
      word2_opcode(0), PMaddress(addr), PMindex(addr/2), initialized(false)
  {
  }


  //--------------------------------------------------
  MultiWordBranch::MultiWordBranch(Processor *new_cpu,
				       unsigned int new_opcode,
				       unsigned int addr)
    : MultiWordInstruction(new_cpu, new_opcode, addr), destination_index(0)
  {
  }

  void MultiWordBranch::runtime_initialize()
  {
    if(cpu_dsPic->program_memory[PMindex+1] != &cpu_dsPic->bad_instruction) {

      word2_opcode = cpu_dsPic->program_memory[PMindex+1]->get_opcode();

      // extract the destination address from the two-word opcode
      destination_index = ((word2_opcode & 0x7f)<<15) | ((opcode>>1) & 0x7fff);
      initialized = true;
    }
  }

  char * MultiWordBranch::name(char *return_str,int len)
  {
    if(!initialized)
      runtime_initialize();

    snprintf(return_str,len,"%s\t0x%05x",
	     gpsimObject::name().c_str(),
	     destination_index<<1);

    return(return_str);
  }


  //--------------------------------------------------
  LiteralBranch::LiteralBranch(Processor *new_cpu,
			       unsigned int new_opcode,
			       unsigned int addr,
			       const char *_name)
    : instruction(new_cpu, new_opcode, addr), mcP_conditionName("")
  {
    new_name(_name);
    unsigned int signExtendedOffset =
      ((new_opcode&0xffff)<<1) | ((new_opcode & (1<<15)) ? 0xfffe0000 : 0);
    m_destination = (addr + 2 + signExtendedOffset) & 0xfffffe;
  }
  char *LiteralBranch::name(char *buff,int len)
  {
    if (buff) {
      char sign = (opcode & (1<<15)) ? '-' : '+';

      int offset= (((sign=='-') ?
		    ((opcode^0xffff)+1) : opcode) << 1) & 0x1fffe;

      snprintf(buff, len, "%s\t%s#0x%06x\t; $%c0x%x",
	       instruction::name().c_str(),
	       mcP_conditionName,
	       m_destination,
	       sign, offset);
    }
    return buff;
  }

  //--------------------------------------------------
  ImmediateInstruction::ImmediateInstruction(Processor *new_cpu,
					     unsigned int new_opcode,
					     unsigned int addr)
    : instruction(new_cpu, new_opcode, addr), m_L(new_opcode & 0xfffe)
  {
  }
  char *ImmediateInstruction::name(char *buff,int len)
  {
    if (buff) {
      snprintf(buff, len, "%s\t#0x%04x", instruction::name().c_str(),m_L);
    }
    return buff;
  }

  //--------------------------------------------------
  RegisterInstruction::RegisterInstruction(Processor *new_cpu,
					   unsigned int new_opcode,
					   unsigned int addr,
					   const char *_name)
    : instruction(new_cpu, new_opcode, addr),
      m_bByteOperation((new_opcode & (1<<14)) != 0),
      m_base(0), m_source(0), m_destination(0)
  {
    new_name(_name);
  }
  //--------------------------------------------------
  //--------------------------------------------------
  RegisterToRegisterInstruction::RegisterToRegisterInstruction
  (Processor *new_cpu,
   unsigned int new_opcode,
   unsigned int addr,
   const char *_name,
   eAddressingModes addrMode
   )
    : RegisterInstruction(new_cpu, new_opcode, addr, _name),
      m_addrMode(addrMode)
  {

    switch(m_addrMode) {
    case eRegisterDirect:
      m_base =  new RegDirectAddrMode(cpu_dsPic,opcode & 0xf);
      m_destination = new RegDirectAddrMode(cpu_dsPic,opcode & 0xf);
      m_source = new LiteralAddressingMode(cpu_dsPic,
		(opcode >> 4) & ((opcode & (1 << 14)) ? 0xff : 0x3ff));
      break;
    case eRegisterIndirect:
      m_base = new RegDirectAddrMode(cpu_dsPic, (opcode>>15) & 0xf);
      m_source = AddressingMode::construct(cpu_dsPic,
					   (opcode >> 4) & 0x7,
					   opcode & 0x1f);
      m_destination = AddressingMode::construct(cpu_dsPic,
						(opcode >> 11) & 0x7,
						(opcode >> 7)  & 0xf);
      break;
    default:
      assert(0);
    }
  }

  char *RegisterToRegisterInstruction::name(char *buff,int len)
  {
    if (!buff)
      return buff;

    char cpBase[256];
    char cpSource[256];
    char cpDestination[256];

    switch(m_addrMode) {
    case eRegisterDirect:
      snprintf(buff, len, "%s%s\t%s, %s",
	       instruction::name().c_str(),
	       (m_bByteOperation ? ".b" :""),
	       m_source->name(cpBase,sizeof(cpBase)),
	       m_destination->name(cpDestination,sizeof(cpDestination))
	       );
      break;
    case eRegisterIndirect:
      snprintf(buff, len, "%s%s\t%s,%s,%s",
	       instruction::name().c_str(),
	       (m_bByteOperation ? ".b" :""),
	       m_base->name(cpBase,sizeof(cpBase)),
	       m_source->name(cpSource,sizeof(cpSource)),
	       m_destination->name(cpDestination,sizeof(cpDestination))
	       );
      break;
    default:
      break;
    }

    return buff;
  }
  //--------------------------------------------------

  ADDR::ADDR (Processor *new_cpu,
	      unsigned int new_opcode,
	      unsigned int addr,
	      eAddressingModes addrMode)
    : RegisterToRegisterInstruction(new_cpu, new_opcode, addr,"add",addrMode)
  {
  }

  void ADDR::execute()
  {
    RegisterValue baseRV(m_base ? m_base->get() : m_destination->get());
    RegisterValue srcRV (m_source->get());
    RegisterValue resRV (srcRV);

    resRV.data += baseRV.data;
    resRV.init |= baseRV.init;

    m_destination->put(resRV);

    unsigned flags =
      ((resRV.data & 0xffff  )  ? 0  : eZ) |
      ((resRV.data & 0x10000 )  ? eC : 0) |
      (((resRV.data ^ baseRV.data ^ srcRV.data)&0x10) ? eDC : 0) |
      ((((resRV.data & ~baseRV.data & ~srcRV.data) |
	 (~resRV.data & baseRV.data & srcRV.data)) & 0x8000) ? eOV : 0) |
      ((resRV.data & 0x8000 )  ? eN : 0);

    cpu_dsPic->m_status.putFlags(flags, eC|eZ|eOV|eN|eDC, 0);
    cpu_dsPic->pc->increment();
  }

  //--------------------------------------------------
  /*
  ADDL::ADDL (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : RegisterDirectLiteralInstruction(new_cpu, new_opcode, addr,"add")
  {
  }

  void ADDL::execute()
  {
    cpu_dsPic->pc->increment();
  }
  */
  //--------------------------------------------------

  ADD::ADD (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("add");
  }

  void ADD::execute()
  {
    cpu_dsPic->pc->increment();
  }

  //--------------------------------------------------

  ADDC::ADDC (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("addc");
  }

  void ADDC::execute()
  {
  }


  //--------------------------------------------------

  AND::AND (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("and");
  }

  void AND::execute()
  {
  }

  //--------------------------------------------------

  ASR::ASR (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("asr");
  }

  void ASR::execute()
  {
  }

  //--------------------------------------------------

  BCLR::BCLR (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("bclr");
  }

  void BCLR::execute()
  {
  }

  //--------------------------------------------------

  BRA::BRA (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : LiteralBranch(new_cpu, new_opcode, addr, "bra"),
      m_condition(0)
  {
    new_name("bra");
    switch ((opcode>>16) & 0x0f) {
    case OV:    // Overflow
      mcP_conditionName = "OV,";
      break;
    case C:     // Carry and GEU
      mcP_conditionName = "C,";
      break;
    case Z:     // Zero
      mcP_conditionName = "Z,";
      break;
    case N:     // Negative
      mcP_conditionName = "N,";
      break;
    case LE:    // Less than or equal to
      mcP_conditionName = "LE,";
      break;
    case LT:    // Less than
      mcP_conditionName = "LT,";
      break;
    case LEU:   // Less than or equal unsigned
      mcP_conditionName = "LEU,";
      break;
    case UN:    // Unconditionally
      mcP_conditionName = "";
      break;
    case NOV:   // No overflow
      mcP_conditionName = "NOV,";
      break;
    case NC:    // No Carry and LTU (less than unsigned)
      mcP_conditionName = "NC,";
      break;
    case NZ:    // Not zero
      mcP_conditionName = "NZ,";
      break;
    case NN:    // Not Negative
      mcP_conditionName = "NN,";
      break;
    case GT:    // Greater than
      mcP_conditionName = "GT,";
      break;
    case GE:    // Greater than or equal
      mcP_conditionName = "GE,";
      break;
    case GTU:   // Greater than unsigned
      mcP_conditionName = "GTU,";
      break;
    case NU:    // not used...
    default:
      break;

    }
  }

  void BRA::execute()
  {
    if (m_condition)
      cpu_dsPic->pc->jump(m_destination>>1);
    else
      cpu_dsPic->pc->increment();
  }

  //--------------------------------------------------

  BSET::BSET (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("bset");
  }

  void BSET::execute()
  {
  }

  //--------------------------------------------------

  BSW::BSW (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("bsw ");
  }

  void BSW::execute()
  {
  }

  //--------------------------------------------------

  BTG::BTG (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("btg ");
  }

  void BTG::execute()
  {
  }

  //--------------------------------------------------

  BTS::BTS (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("bts");
  }

  void BTS::execute()
  {
  }

  //--------------------------------------------------

  BTST::BTST (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("btst");
  }

  void BTST::execute()
  {
  }

  //--------------------------------------------------

  BTSTS::BTSTS (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("btsts");
  }

  void BTSTS::execute()
  {
  }

  //--------------------------------------------------

  CALL::CALL (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("call");
  }

  void CALL::execute()
  {
  }

  //--------------------------------------------------

  CLR::CLR (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("clr");
  }

  void CLR::execute()
  {
  }

  //--------------------------------------------------

  CLRWDT::CLRWDT (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("clrwdt");
  }

  void CLRWDT::execute()
  {
  }

  //--------------------------------------------------

  COM::COM (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("com");
  }

  void COM::execute()
  {
  }

  //--------------------------------------------------

  CP::CP (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("cp");
  }

  void CP::execute()
  {
  }

  //--------------------------------------------------

  CP0::CP0 (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("cp0");
  }

  void CP0::execute()
  {
  }

  //--------------------------------------------------

  CPB::CPB (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("cpb");
  }

  void CPB::execute()
  {
  }

  //--------------------------------------------------

  CPS::CPS (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("cps");
  }

  void CPS::execute()
  {
  }

  //--------------------------------------------------

  DAW::DAW (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("daw");
  }

  void DAW::execute()
  {
  }

  //--------------------------------------------------

  DEC::DEC (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("dec");
  }

  void DEC::execute()
  {
  }

  //--------------------------------------------------

  DISI::DISI (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("disi");
  }

  void DISI::execute()
  {
  }

  //--------------------------------------------------

  DIV::DIV (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("div");
  }

  void DIV::execute()
  {
  }

  //--------------------------------------------------

  DO::DO (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("do");
  }

  void DO::execute()
  {
  }

  //--------------------------------------------------

  ED::ED (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("ed");
  }

  void ED::execute()
  {
  }

  //--------------------------------------------------

  EXCH::EXCH (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("exch");
  }

  void EXCH::execute()
  {
  }

  //--------------------------------------------------

  FB::FB (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("fb");
  }

  void FB::execute()
  {
  }

  //--------------------------------------------------

  GOTO::GOTO (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : MultiWordBranch(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("goto");
  }

  void GOTO::execute()
  {
    if(!initialized)
      runtime_initialize();

    cpu_dsPic->pc->jump(destination_index);

  }

  //--------------------------------------------------

  INC::INC (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("inc");
  }

  void INC::execute()
  {
  }

  //--------------------------------------------------

  IOR::IOR (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("ior");
  }

  void IOR::execute()
  {
  }

  //--------------------------------------------------

  LAC::LAC (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("lac");
  }

  void LAC::execute()
  {
  }

  //--------------------------------------------------

  LNK::LNK (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : ImmediateInstruction(new_cpu, new_opcode, addr)
  {
    new_name("lnk");
  }

  void LNK::execute()
  {
    unsigned int tos = cpu_dsPic->W[15].get_value();
    unsigned int tos_index = tos>>1;
    cpu_dsPic->registers[tos_index]->put(cpu_dsPic->W[14].get());
    cpu_dsPic->W[14].put(tos+2);
    cpu_dsPic->W[15].put(tos+2+m_L);

    cpu_dsPic->pc->increment();

  }

  //--------------------------------------------------

  LSR::LSR (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("lsr");
  }

  void LSR::execute()
  {
  }

  //--------------------------------------------------

  MAC::MAC (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("mac");
  }

  void MAC::execute()
  {
  }

  //--------------------------------------------------

  MOV::MOV (Processor *new_cpu,
	    unsigned int new_opcode,
	    unsigned int addr,
	    eAddressingModes addrMode)
    : RegisterToRegisterInstruction(new_cpu, new_opcode, addr,"mov",addrMode)
  {
  }

  void MOV::execute()
  {
    RegisterValue baseRV(m_base ? m_base->get() : m_destination->get());
    RegisterValue srcRV (m_source->get());
    RegisterValue resRV (srcRV);

    resRV.data += baseRV.data;
    resRV.init |= baseRV.init;

    m_destination->put(resRV);

    unsigned flags =
      ((resRV.data & 0xffff  )  ? 0  : eZ) |
      ((resRV.data & 0x10000 )  ? eC : 0) |
      (((resRV.data ^ baseRV.data ^ srcRV.data)&0x10) ? eDC : 0) |
      ((((resRV.data & ~baseRV.data & ~srcRV.data) |
	 (~resRV.data & baseRV.data & srcRV.data)) & 0x8000) ? eOV : 0) |
      ((resRV.data & 0x8000 )  ? eN : 0);

    cpu_dsPic->m_status.putFlags(flags, eC|eZ|eOV|eN|eDC, 0);
    cpu_dsPic->pc->increment();
  }


  //--------------------------------------------------

  MOVSAC::MOVSAC (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("movsac");
  }

  void MOVSAC::execute()
  {
  }

  //--------------------------------------------------

  MPY::MPY (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("mpy");
  }

  void MPY::execute()
  {
  }

  //--------------------------------------------------

  MUL::MUL (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("mul");
  }

  void MUL::execute()
  {
  }

  //--------------------------------------------------

  NEG::NEG (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("neg");
  }

  void NEG::execute()
  {
  }

  //--------------------------------------------------

  NOP::NOP (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("nop");

    // For the 18cxxx family, this 'nop' may in fact be the
    // 2nd word in a 2-word opcode. So just to be safe, let's
    // initialize the cross references to the source file.
    // (Subsequent initialization code will overwrite this,
    // but there is a chance that this info will be accessed
    // before that occurs).
  }

  void NOP::execute()
  {
    //cpu_pic->pc->increment();
  }

  //--------------------------------------------------

  POP::POP (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("pop");
  }

  void POP::execute()
  {
  }

  //--------------------------------------------------

  PUSH::PUSH (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("push");
  }

  void PUSH::execute()
  {
  }

  //--------------------------------------------------

  PWRSAV::PWRSAV (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("pwrsav");
  }

  void PWRSAV::execute()
  {
  }

  //--------------------------------------------------

  RCALL::RCALL (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : LiteralBranch(new_cpu, new_opcode, addr,"rcall")
  {
  }

  void RCALL::execute()
  {
    cpu_dsPic->m_stack.push();
    cpu_dsPic->pc->jump(m_destination>>1);
  }

  //--------------------------------------------------

  REPEAT::REPEAT (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("repeat");
  }

  void REPEAT::execute()
  {
  }

  //--------------------------------------------------

  RESET::RESET (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("reset");
  }

  void RESET::execute()
  {
  }

  //--------------------------------------------------

  RET::RET (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("ret");
  }

  void RET::execute()
  {
  }

  //--------------------------------------------------

  ROT::ROT (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("rot");
  }

  void ROT::execute()
  {
  }

  //--------------------------------------------------

  SAC::SAC (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("sac");
  }

  void SAC::execute()
  {
  }

  //--------------------------------------------------

  SE::SE (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("se");
  }

  void SE::execute()
  {
  }

  //--------------------------------------------------

  SETM::SETM (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("setm");
  }

  void SETM::execute()
  {
  }

  //--------------------------------------------------

  SFTAC::SFTAC (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("sftac");
  }

  void SFTAC::execute()
  {
  }

  //--------------------------------------------------

  SL::SL (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("sl");
  }

  void SL::execute()
  {
  }

  //--------------------------------------------------

  SUB::SUB (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("sub");
  }

  void SUB::execute()
  {
  }

  //--------------------------------------------------

  SWAP::SWAP (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("swap");
  }

  void SWAP::execute()
  {
  }

  //--------------------------------------------------

  TBLRD::TBLRD (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("tblrd");
  }

  void TBLRD::execute()
  {
  }

  //--------------------------------------------------

  TBLWT::TBLWT (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("tblwt");
  }

  void TBLWT::execute()
  {
  }

  //--------------------------------------------------

  ULNK::ULNK (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("ulnk");
  }

  void ULNK::execute()
  {
  }

  //--------------------------------------------------

  XOR::XOR (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("xor");
  }

  void XOR::execute()
  {
  }

  //--------------------------------------------------

  ZE::ZE (Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    : instruction(new_cpu, new_opcode, addr)
  {

    decode(new_opcode);
    new_name("ZE");
  }

  void ZE::execute()
  {
  }


};
