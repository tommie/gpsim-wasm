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

#ifndef SRC_DSPIC_DSPIC_INSTRUCTIONS_H_
#define SRC_DSPIC_DSPIC_INSTRUCTIONS_H_

#include "../registers.h"
#include "../pic-instructions.h"

class Processor;
namespace dspic
{
  class dsPicProcessor;
};

namespace dspic_instructions
{

  // Fix me - duplicate of the status register bit definitions.
  enum {
    eC    = 1<<0,
    eZ    = 1<<1,
    eOV   = 1<<2,
    eN    = 1<<3,
    eRA   = 1<<4,
    eIPLD = 1<<5,
    eIPL1 = 1<<6,
    eIPL2 = 1<<7,
    eDC   = 1<<8,
    eDA   = 1<<9,
    eSAB  = 1<<10,
    eOAB  = 1<<11,
    eSB   = 1<<12,
    eSA   = 1<<13,
    eOB   = 1<<14,
    eOA   = 1<<15
  };

  //---------------------------------------------------------
  class MultiWordInstruction : public instruction
  {
  public:
    MultiWordInstruction(Processor *new_cpu, 
                         unsigned int new_opcode,
                         unsigned int addr);
    virtual int instruction_size(void) { return 2;}
    virtual enum INSTRUCTION_TYPES isa(void) {return MULTIWORD_INSTRUCTION;};
    virtual bool isBase() { return true;}
    virtual void initialize(bool init_state) { initialized = init_state; }
  protected:
    unsigned int word2_opcode;
    unsigned int PMaddress;
    unsigned int PMindex;
    bool initialized;
  };

  //---------------------------------------------------------
  class MultiWordBranch : public MultiWordInstruction
  {
  public:
    MultiWordBranch(Processor *new_cpu, 
                    unsigned int new_opcode,
                    unsigned int addr);

    unsigned int destination_index;

    void runtime_initialize(void);
    virtual void execute(void){};
    virtual char *name(char *,int);

  };

  //---------------------------------------------------------
  class LiteralBranch : public instruction
  {
  public:
    LiteralBranch(Processor *new_cpu, 
                  unsigned int new_opcode,
                  unsigned int addr,
                  const char *_name);

    virtual bool isBase() { return true;}
    virtual char *name(char *,int);
  protected:
    unsigned int m_destination;
    const char *mcP_conditionName;
  };

  //----------------------------------------------------------------------
  class ImmediateInstruction : public instruction
  {
  public:
    ImmediateInstruction(Processor *new_cpu, 
                         unsigned int new_opcode,
                         unsigned int addr);
    virtual char *name(char *,int len);
    virtual bool isBase() { return true;}
  protected:
    unsigned int m_L;
  };

  //----------------------------------------------------------------------
  class AddressingMode 
  {
  public:
    AddressingMode(dspic::dsPicProcessor *cpu, 
                   unsigned int addr);

    virtual ~AddressingMode()
    {
    }

    virtual RegisterValue get()=0;
    virtual void put(RegisterValue &)=0;
    virtual char *name(char *buff, int len)=0;
    /**/
    static AddressingMode *construct(dspic::dsPicProcessor *new_cpu, 
                                     unsigned int new_mode, unsigned int addr);
    /**/

    enum {
      eDirect = 0,
      eIndirect,
      eIndirectPostDec,
      eIndirectPostInc,
      eIndirectPreDec,
      eIndirectPreInc,
      eIndirectRegOffset,
      eIndirectRegOffset_,
      eLiteral=6,
      eLiteral_=7,
    };

  protected:
    dspic::dsPicProcessor *m_cpu;
    unsigned int m_mode;
    unsigned int m_addr;
    static const RegisterValue m_unknown;
  };
  //----------------------------------------------------------------------
  class LiteralAddressingMode : public AddressingMode
  {
  public:
    LiteralAddressingMode(dspic::dsPicProcessor *cpu, 
                          unsigned int addr);

    virtual ~LiteralAddressingMode()
    {
    }

    RegisterValue get() override {return m_rv;}
    void put(RegisterValue &) override {} // maybe we should throw an error?
    char *name(char *buff, int len) override;
  private:
    RegisterValue m_rv;
  };
  //----------------------------------------------------------------------
  class RegisterAddressingMode : public AddressingMode
  {
  public:
    RegisterAddressingMode(dspic::dsPicProcessor *cpu, 
                      unsigned int addr, 
                      const char *format);

    virtual ~RegisterAddressingMode()
    {
    }

    char *name(char *buff, int len) override;
  protected:
    const char *m_cPformat;
  };
  //----------------------------------------------------------------------
  class RegDirectAddrMode : public RegisterAddressingMode
  {
  public:
    RegDirectAddrMode(dspic::dsPicProcessor *cpu, unsigned int addr);

    virtual ~RegDirectAddrMode()
    {
    }

    RegisterValue get() override;
    void put(RegisterValue &) override;
  };
  //----------------------------------------------------------------------
  class RegIndirectAddrMode : public RegisterAddressingMode
  {
  public:
    RegIndirectAddrMode(dspic::dsPicProcessor *cpu, unsigned int addr);

    virtual ~RegIndirectAddrMode()
    {
    }

    RegisterValue get() override;
    void put(RegisterValue &) override;
  };

  //----------------------------------------------------------------------
  class RegIndirectPostDecAddrMode : public RegisterAddressingMode
  {
  public:
    RegIndirectPostDecAddrMode(dspic::dsPicProcessor *cpu, unsigned int addr);

    virtual ~RegIndirectPostDecAddrMode()
    {
    }

    RegisterValue get() override;
    void put(RegisterValue &) override;
  };
  //----------------------------------------------------------------------
  class RegIndirectPostIncAddrMode : public RegisterAddressingMode
  {
  public:
    RegIndirectPostIncAddrMode(dspic::dsPicProcessor *cpu, unsigned int addr);

    virtual ~RegIndirectPostIncAddrMode()
    {
    }

    RegisterValue get() override;
    void put(RegisterValue &) override;
  };
  //----------------------------------------------------------------------
  class RegIndirectPreDecAddrMode : public RegisterAddressingMode
  {
  public:
    RegIndirectPreDecAddrMode(dspic::dsPicProcessor *cpu, unsigned int addr);

    virtual ~RegIndirectPreDecAddrMode()
    {
    }

    RegisterValue get() override;
    void put(RegisterValue &) override;
  };

  //----------------------------------------------------------------------
  class RegIndirectPreIncAddrMode : public RegisterAddressingMode
  {
  public:
    RegIndirectPreIncAddrMode(dspic::dsPicProcessor *cpu, unsigned int addr);

    virtual ~RegIndirectPreIncAddrMode()
    {
    }

    RegisterValue get() override;
    void put(RegisterValue &) override;
  };

  //----------------------------------------------------------------------
  class RegisterInstruction : public instruction
  {
  public:
    RegisterInstruction(Processor *new_cpu, 
                        unsigned int new_opcode,
                        unsigned int addr,
                        const char *_name);
    virtual bool isBase() { return true;}
  protected:
    bool m_bByteOperation;
    AddressingMode *m_base;
    AddressingMode *m_source;
    AddressingMode *m_destination;
  };
  //++++++++++++++++++++++++++++++++++++++++
  enum eAddressingModes {
    eRegisterDirect,
    eRegisterIndirect
  };
  //----------------------------------------------------------------------
  class RegisterToRegisterInstruction : public RegisterInstruction
  {
  public:
    RegisterToRegisterInstruction(Processor *new_cpu, 
                                  unsigned int new_opcode,
                                  unsigned int addr,
                                  const char *new_name,
                                  eAddressingModes addrMode);
    virtual char *name(char *,int len);
  protected:
    eAddressingModes m_addrMode;
    //bool m_bHasBase; // true for instructions with 3 operands.
  };
 
  //----------------------------------------------------------------------
  class ADDR : public RegisterToRegisterInstruction
  {
  public:
    ADDR(Processor *new_cpu, unsigned int new_opcode, 
         unsigned int addr, eAddressingModes addrMode);
    virtual void execute();
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {
      if ((new_opcode & 0xf00000) == 0x400000)
        return new ADDR(new_cpu,new_opcode,addr,eRegisterIndirect);
      return new ADDR(new_cpu,new_opcode,addr,eRegisterDirect);
    }
  };

  //----------------------------------------------------------------------
  class ADD : public instruction
  {
  public:
    ADD(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new ADD(new_cpu,new_opcode,addr);}
  };

  //----------------------------------------------------------------------
  class ADDC : public instruction
  {
  public:
    ADDC(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new ADDC(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class AND : public instruction
  {
  public:
    AND(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new AND(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class ASR : public instruction
  {
  public:
    ASR(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new ASR(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class BCLR : public instruction
  {
  public:
    BCLR(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new BCLR(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class BRA : public LiteralBranch
  {
  public:
    BRA(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new BRA(new_cpu,new_opcode, addr);}
  protected:
    unsigned int m_condition;
    enum eConditions {
      OV=0,  // Overflow
      C,     // Carry and GEU
      Z,     // Zero
      N,     // Negative
      LE,    // Less than or equal to
      LT,    // Less than
      LEU,   // Less than or equal unsigned
      UN,    // Unconditionally
      NOV,   // No overflow
      NC,    // No Carry and LTU (less than unsigned)
      NZ,    // Not zero
      NN,    // Not Negative
      GT,    // Greater than
      GE,    // Greater than or equal
      GTU,   // Greater than unsigned
      NU     // not used...
    };
  };

  //----------------------------------------------------------------------
  class BSET : public instruction
  {
  public:
    BSET(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new BSET(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class BSW : public instruction
  {
  public:
    BSW(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new BSW(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class BTG : public instruction
  {
  public:
    BTG(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new BTG(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class BTS : public instruction
  {
  public:
    BTS(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new BTS(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class BTST : public instruction
  {
  public:
    BTST(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new BTST(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class BTSTS : public instruction
  {
  public:
    BTSTS(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new BTSTS(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class CALL : public instruction
  {
  public:
    CALL(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new CALL(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class CLR : public instruction
  {
  public:
    CLR(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new CLR(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class CLRWDT : public instruction
  {
  public:
    CLRWDT(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new CLRWDT(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class COM : public instruction
  {
  public:
    COM(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new COM(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class CP : public instruction
  {
  public:
    CP(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new CP(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class CP0 : public instruction
  {
  public:
    CP0(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new CP0(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class CPB : public instruction
  {
  public:
    CPB(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new CPB(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class CPS : public instruction
  {
  public:
    CPS(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new CPS(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class DAW : public instruction
  {
  public:
    DAW(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new DAW(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class DEC : public instruction
  {
  public:
    DEC(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new DEC(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class DISI : public instruction
  {
  public:
    DISI(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new DISI(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class DIV : public instruction
  {
  public:
    DIV(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new DIV(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class DO : public instruction
  {
  public:
    DO(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new DO(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class ED : public instruction
  {
  public:
    ED(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new ED(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class EXCH : public instruction
  {
  public:
    EXCH(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new EXCH(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class FB : public instruction
  {
  public:
    FB(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new FB(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class GOTO : public MultiWordBranch
  {
  public:
    GOTO(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    void execute() override;
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new GOTO(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class INC : public instruction
  {
  public:
    INC(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new INC(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class IOR : public instruction
  {
  public:
    IOR(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new IOR(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class LAC : public instruction
  {
  public:
    LAC(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new LAC(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class LNK : public ImmediateInstruction
  {
  public:
    LNK(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new LNK(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class LSR : public instruction
  {
  public:
    LSR(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new LSR(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class MAC : public instruction
  {
  public:
    MAC(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new MAC(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  //----------------------------------------------------------------------
  class MOV : public RegisterToRegisterInstruction
  {
  public:
    MOV(Processor *new_cpu, unsigned int new_opcode, 
         unsigned int addr, eAddressingModes addrMode);
    virtual void execute();
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {
      if ((new_opcode & 0xf78000) == 0xb78000)
        return new MOV(new_cpu,new_opcode,addr,eRegisterDirect);
      return new MOV(new_cpu,new_opcode,addr,eRegisterIndirect);
    }
  };

  //----------------------------------------------------------------------
  class MOVSAC : public instruction
  {
  public:
    MOVSAC(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new MOVSAC(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class MPY : public instruction
  {
  public:
    MPY(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new MPY(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class MUL : public instruction
  {
  public:
    MUL(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new MUL(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class NEG : public instruction
  {
  public:
    NEG(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new NEG(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class NOP : public instruction
  {
  public:
    NOP(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new NOP(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class POP : public instruction
  {
  public:
    POP(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new POP(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class PUSH : public instruction
  {
  public:
    PUSH(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new PUSH(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class PWRSAV : public instruction
  {
  public:
    PWRSAV(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new PWRSAV(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class RCALL : public LiteralBranch
  {
  public:
    RCALL(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new RCALL(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class REPEAT : public instruction
  {
  public:
    REPEAT(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new REPEAT(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class RESET : public instruction
  {
  public:
    RESET(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new RESET(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class RET : public instruction
  {
  public:
    RET(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new RET(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class ROT : public instruction
  {
  public:
    ROT(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new ROT(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class SAC : public instruction
  {
  public:
    SAC(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new SAC(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class SE : public instruction
  {
  public:
    SE(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new SE(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class SETM : public instruction
  {
  public:
    SETM(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new SETM(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class SFTAC : public instruction
  {
  public:
    SFTAC(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new SFTAC(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class SL : public instruction
  {
  public:
    SL(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new SL(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class SUB : public instruction
  {
  public:
    SUB(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new SUB(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class SWAP : public instruction
  {
  public:
    SWAP(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new SWAP(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class TBLRD : public instruction
  {
  public:
    TBLRD(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new TBLRD(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class TBLWT : public instruction
  {
  public:
    TBLWT(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new TBLWT(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class ULNK : public instruction
  {
  public:
    ULNK(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new ULNK(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class XOR : public instruction
  {
  public:
    XOR(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new XOR(new_cpu,new_opcode, addr);}
  };

  //----------------------------------------------------------------------
  class ZE : public instruction
  {
  public:
    ZE(Processor *new_cpu, unsigned int new_opcode, unsigned int addr);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int addr)
    {return new ZE(new_cpu,new_opcode, addr);}
  };

};

#endif // SRC_DSPIC_DSPIC_INSTRUCTIONS_H_
