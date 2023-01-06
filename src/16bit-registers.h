/*
   Copyright (C) 1998 T. Scott Dattalo

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

#ifndef SRC_16_BIT_REGISTERS_H_
#define SRC_16_BIT_REGISTERS_H_

#include <glib.h>
#include <string>

#include "14bit-tmrs.h"
#include "eeprom.h"
#include "gpsim_classes.h"
#include "pic-registers.h"
#include "registers.h"
#include "stimuli.h"
#include "tmr0.h"
#include "trigger.h"
#include "14bit-registers.h"

#define _16BIT_REGISTER_MASK   0xfff

class HLVDCON;
class INTCON;
class Indirect_Addressing;
class InterruptSource;
class PIR_SET;
class PinModule;
class Processor;
class Stack16;
class _16bit_processor;
class pic_processor;


//---------------------------------------------------------
//
// Indirect_Addressing
//
// This class coordinates the indirect addressing on the 18cxxx
// parts. Each of the registers comprising the indirect addressing
// subsystem: FSRnL,FSRnH, INDFn, POSTINCn, POSTDECn, PREINCn, and
// PLUSWn are each individually defined as sfr_registers AND included
// in the Indirect_Addressing class. So accessing these registers
// is the same as accessing any register: through the core cpu's
// register memory. The only difference for these registers is that
// the



//---------------------------------------------------------
// FSR registers

class FSRL : public sfr_register {
public:
  FSRL(Processor *, const char *pName, const char *pDesc, Indirect_Addressing *pIAM);

  void put(unsigned int new_value) override;
  void put_value(unsigned int new_value) override;

protected:
  Indirect_Addressing  *iam;
};


class FSRH : public sfr_register {
public:
  FSRH(Processor *, const char *pName, const char *pDesc, Indirect_Addressing *pIAM);

  void put(unsigned int new_value) override;
  void put_value(unsigned int new_value) override;

protected:
  Indirect_Addressing  *iam;
};


class INDF16 : public sfr_register {
public:
  INDF16(Processor *, const char *pName, const char *pDesc, Indirect_Addressing *pIAM);

  void put(unsigned int new_value) override;
  void put_value(unsigned int new_value) override;
  unsigned int get() override;
  unsigned int get_value() override;

protected:
  Indirect_Addressing  *iam;
};


class PREINC : public sfr_register {
public:
  PREINC(Processor *, const char *pName, const char *pDesc, Indirect_Addressing *pIAM);

  void put(unsigned int new_value) override;
  void put_value(unsigned int new_value) override;
  unsigned int get() override;
  unsigned int get_value() override;

protected:
  Indirect_Addressing  *iam;
};


class POSTINC : public sfr_register {
public:
  POSTINC(Processor *, const char *pName, const char *pDesc, Indirect_Addressing *pIAM);

  void put(unsigned int new_value) override;
  void put_value(unsigned int new_value) override;
  unsigned int get() override;
  unsigned int get_value() override;

protected:
  Indirect_Addressing  *iam;
};


class POSTDEC : public sfr_register {
public:
  POSTDEC(Processor *, const char *pName, const char *pDesc, Indirect_Addressing *pIAM);

  void put(unsigned int new_value) override;
  void put_value(unsigned int new_value) override;
  unsigned int get() override;
  unsigned int get_value() override;

protected:
  Indirect_Addressing  *iam;
};


class PLUSW : public sfr_register {
public:
  PLUSW(Processor *, const char *pName, const char *pDesc, Indirect_Addressing *pIAM);

  void put(unsigned int new_value) override;
  void put_value(unsigned int new_value) override;
  unsigned int get() override;
  unsigned int get_value() override;

protected:
  Indirect_Addressing  *iam;
};


class Indirect_Addressing {
public:
  Indirect_Addressing(pic_processor *cpu, const std::string &n);

  pic_processor *cpu;
  //RRR  _16bit_processor *cpu;

  unsigned int fsr_value;     // 16bit concatenation of fsrl and fsrh
  unsigned int fsr_state;     /* used in conjunction with the pre/post incr
			       * and decrement. This is mainly needed for
			       * those instructions that perform read-modify-
			       * write operations on the indirect registers
			       * eg. btg POSTINC1,4 . The post increment must
			       * occur after the bit is toggled and not during
			       * the read operation that's determining the
			       * current state.
			       */
  int     fsr_delta;          /* If there's a pending update to the fsr register
			       * pair, then the magnitude of that update is
			       * stored here.
			       */
  guint64 current_cycle;      /* Stores the cpu cycle when the fsr was last
			       * changed.
			       */
  FSRL    fsrl;
  FSRH    fsrh;
  INDF16  indf;
  PREINC  preinc;
  POSTINC postinc;
  POSTDEC postdec;
  PLUSW   plusw;

  //void init(_16bit_processor *new_cpu);
  void put(unsigned int new_value);
  unsigned int get();
  unsigned int get_value();
  void put_fsr(unsigned int new_fsr);
  unsigned int get_fsr_value()
  {
    return fsr_value & 0xfff;
  }
  void update_fsr_value();
  void preinc_fsr_value();
  void postinc_fsr_value();
  void postdec_fsr_value();
  int  plusw_fsr_value();
  int  plusk_fsr_value(int k);

  /* bool is_indirect_register(unsigned int reg_address)
   *
   * The purpose of this routine is to determine whether or not the
   * 'reg_address' is the address of an indirect register. This is
   * used by the 'put' and 'get' functions of the indirect registers.
   * Indirect registers are forbidden access to other indirect registers.
   * (Although double indirection in a single instruction cycle would
   * be powerful!).
   *
   * The indirect registers reside at the following addresses
   * 0xfeb - 0xfef, 0xfe3 - 0xfe7, 0xfdb- 0xfdf
   * If you look at the binary representation of these ranges:
   * 1111 1110 1011, 1111 1110 1100 - 1111 1110 1111    (0xfeb,0xfec - 0xfef)
   * 1111 1110 0011, 1111 1110 0100 - 1111 1110 0111    (0xfe3,0xfe4 - 0xfe7)
   * 1111 1101 1011, 1111 1101 1100 - 1111 1101 1111    (0xfdb,0xfdc - 0xfdf)
   * ------------------------------------------------------------------------
   * 1111 11xx x011, 1111 11vv v1yy - 1111 11vv v1yy
   *
   * Then you'll notice that indirect register addresses share
   * the common bit pattern 1111 11xx x011 for the left column.
   * Furthermore, the middle 3-bits, xxx, can only be 3,4, 5.
   * The ranges in the last two columns share the bit pattern
   * 1111 11vv v1yy. The middle 3-bits, vvv, again can only be
   * 3,4, or 5. The least two lsbs, yy, are don't cares.
   */

  inline bool is_indirect_register(unsigned int reg_address)
  {
    if (((reg_address & 0xfc7) == 0xfc3) || ((reg_address & 0xfc4) == 0xfc4)) {
      unsigned midbits = (reg_address >> 3) & 0x7;

      if (midbits >= 3 && midbits <= 5) {
        return true;
      }
    }
    return false;
  }
};


//---------------------------------------------------------
class Fast_Stack {
public:
  void init(_16bit_processor *new_cpu);
  void push();
  void pop();

  unsigned int w = 0;
  unsigned int status = 0;
  unsigned int bsr = 0;
  _16bit_processor *cpu = nullptr;
};


//---------------------------------------------------------
class PCL16 : public PCL {
public:
  PCL16(Processor *, const char *pName, const char *pDesc = nullptr);

  unsigned int get() override;
  unsigned int get_value() override;
};


//---------------------------------------------------------
// Program Counter
//

class Program_Counter16 : public Program_Counter {
public:
  explicit Program_Counter16(Processor *pCpu);

  virtual void bounds_error ( const char * func, const char * test, unsigned int val );
  virtual void increment() override;
  //virtual void skip();
  //virtual void jump(unsigned int new_value);
  //virtual void interrupt(unsigned int new_value);
  void computed_goto(unsigned int new_value) override;
  //virtual void new_address(unsigned int new_value);
  void put_value(unsigned int new_value) override;
  void update_pcl() override;
  unsigned int get_value() override;
  virtual unsigned int pcl_read() override;
  //virtual unsigned int get_next();

  bool update_latch;
};


//---------------------------------------------------------
// Stack
//

class STKPTR16 : public sfr_register {
public:
  enum {
    STKUNF = 1 << 6,
    STKOVF = 1 << 7
  };
  STKPTR16(Processor *, const char *pName, const char *pDesc = nullptr);

  void put_value(unsigned int new_value) override;
  void put(unsigned int new_value) override;

  Stack16 *stack = nullptr;
};


class TOSU : public sfr_register {
public:
  TOSU(Processor *, const char *pName, const char *pDesc = nullptr);

  void put(unsigned int new_value) override;
  void put_value(unsigned int new_value) override;
  unsigned int get() override;
  unsigned int get_value() override;

  Stack16 *stack = nullptr;
};


class Stack16 : public Stack {
public:
  explicit Stack16(Processor *);
  ~Stack16();

  bool push(unsigned int) override;
  unsigned int pop() override;
  void reset(RESET_TYPE) override;
  bool stack_overflow() override;
  bool stack_underflow() override;

  STKPTR16 stkptr;
  TOSL   tosl;
  TOSH   tosh;
  TOSU   tosu;
};


//---------------------------------------------------------
class CPUSTA :  public sfr_register {
public:

  enum {
    BOR      = 1 << 0,
    POR      = 1 << 1,
    PD       = 1 << 2,
    TO       = 1 << 3,
    GLINTD   = 1 << 4,
    STKAV    = 1 << 5,
  };
  CPUSTA(Processor *, const char *pName, const char *pDesc = nullptr);
};


//---------------------------------------------------------
// T0CON - Timer 0 control register
//RRRclass T0CON : public OPTION_REG {
class T0CON : public sfr_register
{
public:

  enum {
    TMR0ON = 1 << 7,
    T08BIT = 1 << 6,
    T0CS   = 1 << 5,
    T0SE   = 1 << 4,
    PSA    = 1 << 3,
    PS2    = 1 << 2,
    PS1    = 1 << 1,
    PS0    = 1 << 0,

  };

  T0CON(Processor *, const char *pName, const char *pDesc = nullptr);

  void put(unsigned int new_value) override;
  void put_value(unsigned int new_value) override;
  void initialize() override;
  void reset(RESET_TYPE r) override;
};


//---------------------------------------------------------
// TMR0 - Timer for the 16bit core.
//
// The 18cxxx extends TMR0 to a 16-bit timer. However, it maintains
// an 8-bit mode that is compatible with the 8-bit TMR0's in the
// 14 and 12-bit cores. The 18cxxx TMR0 reuses this code by deriving
// from the TMR0 class and providing definitions for many of the
// virtual functions.

class TMR0H : public sfr_register {
public:
  TMR0H(Processor *, const char *pName, const char *pDesc = nullptr);

  void put(unsigned int new_value) override;
  void put_value(unsigned int new_value) override;
  unsigned int get() override;
  unsigned int get_value() override;
};


class TMR0_16 : public TMR0 {
public:
  TMR0_16(Processor *, const char *pName, const char *pDesc = nullptr);

  void callback() override;
  void callback_print() override;

  void increment() override;
  unsigned int get() override;
  unsigned int get_value() override;
  void put_value(unsigned int new_value) override;
  unsigned int get_prescale() override;
  unsigned int max_counts() override;
  void set_t0if() override;
  bool get_t0cs() override;
  bool get_t0se() override;
  unsigned int get_option_reg() override {return t0con?t0con->get_value():0;}

  void initialize() override;
  void start(int new_value, int sync = 0) override;
  void sleep() override;
  void wake() override;

  T0CON  *t0con = nullptr;
  INTCON *intcon = nullptr;
  TMR0H  *tmr0h = nullptr;
  unsigned int value16 = 0;
};


//---------------------------------------------------------
/*
class TMR3H : public TMRH
{
public:

};

class TMR3L : public TMRL
{
public:

};
*/


class T3CON : public T1CON {
public:
  enum { T3CCP1 = 1 << 3, T3CCP2 = 1 << 6,
       };

  T3CON(Processor *pCpu, const char *pName, const char *pDesc = nullptr);

  void put(unsigned int new_value) override;
  bool get_t1oscen() override
  {
    if (t1con) {
      return t1con->get_t1oscen();
    }

    return false;
  }

  CCPRL *ccpr1l = nullptr;
  CCPRL *ccpr2l = nullptr;
  TMRL  *tmr1l = nullptr;
  T1CON *t1con = nullptr;
};


//---------------------------------------------------------
//
// TMR3_MODULE
//
//

class TMR3_MODULE {
public:
  TMR3_MODULE();

  void initialize(T3CON *t1con, PIR_SET *pir_set);

  _16bit_processor *cpu = nullptr;
  char *name_str = nullptr;
  T3CON *t3con = nullptr;
  PIR_SET *pir_set = nullptr;
};


//-------------------------------------------------------------------

class TBL_MODULE : public EEPROM_EXTND {
public:
  explicit TBL_MODULE(_16bit_processor *pCpu);

  void increment();
  void decrement();
  void read();
  void write();
  void start_write() override;
  //void initialize(_16bit_processor *);

  unsigned int state = 0;
  unsigned int internal_latch = 0;

  _16bit_processor *cpu;

  sfr_register   tablat,
                 tblptrl,
                 tblptrh,
                 tblptru;
};


//////////////////////////////////////////
//////////////////////////////////////////
//   vapid Place holders
//////////////////////////////////////////
//////////////////////////////////////////


class LVDCON : public  sfr_register {
public:
  unsigned int valid_bits;

  enum {
    LVDL0 = 1 << 0,
    LVDL1 = 1 << 1,
    LVDL2 = 1 << 2,
    LVDL3 = 1 << 3,
    LVDEN = 1 << 4,
    IRVST = 1 << 5,
  };

  LVDCON(Processor *, const char *pName, const char *pDesc = nullptr);
};


/*
   High/Low-Voltage Detect Module
*/

class HLVD_stimulus : public stimulus {
public:
  explicit HLVD_stimulus(HLVDCON *_hlvd, const char *n = nullptr);
  ~HLVD_stimulus();

  void set_nodeVoltage(double v) override;

private:
  HLVDCON *hlvd;
};


class HLVDCON : public  sfr_register, public TriggerObject {
public:
  enum {
    VDIRMAG = 1 << 7, // Voltage Direction Magnitude Select bit
    BGVST   = 1 << 6, // Band Gap Reference Voltages Stable Status Flag bit
    IRVST   = 1 << 5, // Internal Reference Voltage Stable Flag bit
    HLVDEN  = 1 << 4, // High/Low-Voltage Detect Power Enable bit
    HLVDL3  = 1 << 3, // Voltage Detection Level bits
    HLVDL2  = 1 << 2, // Voltage Detection Level bits
    HLVDL1  = 1 << 1, // Voltage Detection Level bits
    HLVDL0  = 1 << 0, // Voltage Detection Level bits
    HLVDL_MASK = 0xf
  };
  HLVDCON(Processor *pCpu, const char *pName, const char *pDesc);
  ~HLVDCON();

  void put(unsigned int new_value) override;
  void callback_print() override;
  void callback() override;
  void set_hlvdin(PinModule *_hlvdin)
  {
    hlvdin = _hlvdin;
  }
  void check_hlvd();
  virtual void setIntSrc(InterruptSource *_IntSrc)
  {
    IntSrc = _IntSrc;
  }

private:
  PinModule	   *hlvdin = nullptr;
  HLVD_stimulus    *hlvdin_stimulus = nullptr;
  bool		   stimulus_active = false;
  unsigned int     write_mask;
  InterruptSource *IntSrc = nullptr;
};


#endif // SRC_16_BIT_REGISTERS_H_
