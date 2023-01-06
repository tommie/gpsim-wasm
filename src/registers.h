/*
   Copyright (C) 1998-2003 Scott Dattalo

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

#ifndef SRC_REGISTERS_H_
#define SRC_REGISTERS_H_

#include <glib.h>

class Expression;
class Processor;
class Module;

#include "gpsim_classes.h"
#include "gpsim_object.h"
#include "value.h"
#include "ValueCollections.h"

#include <string>
#include <vector>

#define AN_INVALID_ADDRESS 0xffffffff

//---------------------------------------------------------
// RegisterValue class
//
// This class is used to represent the value of registers.
// It also defines which bits have been initialized and which
// are valid.
//

class RegisterValue {
public:
  unsigned int data = 0;  // The actual numeric value of the register.
  unsigned int init = 0xff;  // bit mask of initialized bits.
                             // assume 8-bit wide, uninitialized registers

  RegisterValue()
  {
  }

  RegisterValue(unsigned int d, unsigned int i)
    : data(d), init(i)
  {
  }

  RegisterValue(const RegisterValue &value)
    : data(value.data), init(value.init)
  {
  }

  inline bool initialized()
  {
    return init == 0;
  }

  inline unsigned int get()
  {
    return data;
  }

  inline void put(unsigned int d)
  {
    data = d;
  }

  inline void put(unsigned int d, unsigned int i)
  {
    data = d;
    init = i;
  }

  inline unsigned int geti()
  {
    return init;
  }

  inline void puti(unsigned int i)
  {
    init = i;
  }

  inline void operator = (RegisterValue rv)
  {
    data = rv.data;
    init = rv.init;
  }

  inline operator unsigned int ()
  {
    return data;
  }

  inline operator int ()
  {
    return (int)data;
  }

  bool operator == (const RegisterValue &rv) const
  {
    return data == rv.data && init == rv.init;
  }

  bool operator != (const RegisterValue &rv) const
  {
    return data != rv.data || init != rv.init;
  }

  void operator >>= (unsigned int val)
  {
    data >>= val;
    init >>= val;
  }
  char * toString(char *str, int len, int regsize = 2) const;
  char * toBitStr(char *s, int len, unsigned int BitPos,
                  const char *ByteSeparator = "_",
                  const char *HiBitNames = nullptr,
                  const char *LoBitNames = nullptr,
                  const char *UndefBitNames = nullptr) const;
};


//---------------------------------------------------------
/// Register - base class for gpsim registers.
/// The Register class is used by processors and modules to
/// to create memory maps and special function registers.
///

class Register : public Value {
public:
  enum REGISTER_TYPES {
    INVALID_REGISTER,
    GENERIC_REGISTER,
    FILE_REGISTER,
    SFR_REGISTER,
    BP_REGISTER
  };

  RegisterValue value;
  unsigned int address = AN_INVALID_ADDRESS;

  // If non-zero, the alias_mask describes all address at which
  // this file register appears. The assumption (that is true so
  // far for all pic architectures) is that the aliased register
  // locations differ by one bit. For example, the status register
  // appears at addresses 0x03 and 0x83 in the 14-bit core.
  // Consequently, alias_mask = 0x80 and address (above) is equal
  // to 0x03.

  unsigned int alias_mask = 0;
  RegisterValue por_value;  // power on reset value
  unsigned int mValidBits = 0xFF;  // = 255 for 8-bit registers, = 65535 for 16-bit registers.

  // The read_trace and write_trace variables are used while
  // tracing register reads and writes. Essentially, these are
  // the trace commands.

  RegisterValue write_trace;
  RegisterValue read_trace;

  // The trace_state is used to reconstruct the state of the
  // register while traversing a trace buffer.

  RegisterValue trace_state;

  guint64 read_access_count = 0;
  guint64 write_access_count = 0;


public:
  Register(Module *, const char *pName, const char *pDesc = nullptr);
  virtual ~Register();


  int set_break(ObjectBreakTypes bt = eBreakAny,
                ObjectActionTypes at = eActionHalt,
                Expression *expr = nullptr) override;
  int clear_break() override;

  /// get - method for accessing the register's contents.

  virtual unsigned int get();

  /// put - method for writing a new value to the register.

  virtual void put(unsigned int new_value);


  /// put_value - is the same as put(), but some extra stuff like
  /// interfacing to the gui is done. (It's more efficient than
  /// burdening the run time performance with (unnecessary) gui
  ///  calls.)


  virtual void put_value(unsigned int new_value);

  /// get_value - same as get(), but no trace is performed

  unsigned int get_value() override
  {
    return value.get();
  }

  /// getRV - get the whole register value - including the info
  /// of the three-state bits.

  virtual RegisterValue getRV()
  {
    value.data = get();
    return value;
  }

  /// putRV - write a new value to the register.
  /// \deprecated {use SimPutAsRegisterValue()}
  ///

  virtual void putRV(RegisterValue rv)
  {
    value.init = rv.init;
    put(rv.data);
  }

  /// getRV_notrace and putRV_notrace are analogous to getRV and putRV
  /// except that the action (in the derived classes) will not be
  /// traced. The primary reason for this is to allow the gui to
  /// refresh it's windows without having the side effect of filling
  /// up the trace buffer

  virtual RegisterValue getRV_notrace()
  {
    value.data = value.get();
    return value;
  }
  virtual void putRV_notrace(RegisterValue rv)
  {
    value.init = rv.init;
    put_value(rv.data);
  }

  virtual RegisterValue getRVN()
  {
    return getRVN_notrace();
  }
  virtual RegisterValue getRVN_notrace()
  {
    return getRV_notrace();
  }

  /// set --- cast another Value object type into a register type
  /// this is used primarily by expression and stimuli processing
  /// (the put() methods are used by the processors).
  /// FIXME -- consolidate the get, set, and put methods
  void set(Value *) override;

  /// copy --- This is used during expression parsing.
  Value *copy() override;

  /// get(gint64 &i) --- ugh.
  void get(gint64 &i) override;

  virtual void initialize()
  {
  }

  /// get3StateBit - returns the 3-state value of a bit
  /// if a bit is known then a '1' or '0' is returned else,
  /// a '?' is returned. No check is performed to ensure
  /// that only a single bit is checked, thus it's possible
  /// to get the state of a group of bits using this method.

  virtual char get3StateBit(unsigned int bitMask)
  {
    RegisterValue rv = getRV_notrace();
    return (rv.init & bitMask) ? '?' : ((rv.data & bitMask) ? '1' : '0');
  }
  /// In the Register class, the 'Register *get()' returns a
  /// pointer to itself. Derived classes may return something
  /// else (e.g. a break point may be pointing to the register
  /// it replaced and will return that instead).

  virtual Register *getReg()
  {
    return this;
  }

  virtual REGISTER_TYPES isa()
  {
    return GENERIC_REGISTER;
  }
  virtual void reset(RESET_TYPE )
  {
  }

  /// The setbit function is not really intended for general purpose
  /// registers. Instead, it is a place holder which is over-ridden
  /// by the IO ports.

  virtual void setbit(unsigned int bit_number, bool new_value);

  ///  like setbit, getbit is used mainly for breakpoints.

  virtual bool get_bit(unsigned int bit_number);
  virtual double get_bit_voltage(unsigned int bit_number);

  ///  Breakpoint objects will overload this function and return true.

  virtual bool hasBreak()
  {
    return false;
  }

  ///  register_size returns the number of bytes required to store the register
  ///  (this is used primarily by the gui to determine how wide to make text fields)

  virtual unsigned int register_size() const;

  /*
    When the register is accessed, this action is recorded in the trace buffer.
    Here we can specify the exact trace command to use.
   */
  virtual void set_write_trace(RegisterValue &rv);
  virtual void set_read_trace(RegisterValue &rv);
  virtual void put_trace_state(RegisterValue rv)
  {
    trace_state = rv;
  }

  virtual RegisterValue get_trace_state()
  {
    return trace_state;
  }

  /*
    convert value to a string:
   */
  char * toString(char *str, int len) override;
  char * toBitStr(char *s, int len) override;
  virtual std::string &baseName()
  {
    return name_str;
  }

  virtual unsigned int getAddress()
  {
    return address;
  }
  virtual void setAddress(unsigned int addr)
  {
    address = addr;
  }
  Register *getReplaced()
  {
    return m_replaced;
  }
  void setReplaced(Register *preg)
  {
    m_replaced = preg;
  }

  void new_name(std::string &) override;
  void new_name(const char *) override;

protected:
  // A pointer to the register that this register replaces.
  // This is used primarily by the breakpoint code.
  Register *m_replaced = nullptr;
};


//---------------------------------------------------------
// define a special 'invalid' register class. Accessess to
// to this class' value get 0

class InvalidRegister : public Register {
public:
  InvalidRegister(Processor *, const char *pName, const char *pDesc = nullptr);

  void put(unsigned int new_value) override;
  unsigned int get() override;
  REGISTER_TYPES isa() override
  {
    return INVALID_REGISTER;
  }
  Register *getReg() override
  {
    return nullptr;
  }
};


//---------------------------------------------------------
// Base class for a special function register.
class BitSink;


class sfr_register : public Register {
public:
  sfr_register(Module *, const char *pName, const char *pDesc = nullptr);

  RegisterValue wdtr_value; // wdt or mclr reset value

  REGISTER_TYPES isa() override
  {
    return SFR_REGISTER;
  }
  void initialize() override {}

  void reset(RESET_TYPE r) override;

  // The assign and release BitSink methods don't do anything
  // unless derived classes redefine them. Their intent is to
  // provide an interface to the BitSink design - a design that
  // allows clients to be notified when bits change states.

  virtual bool assignBitSink(unsigned int /* bitPosition */ , BitSink *)
  {
    return false;
  }
  virtual bool releaseBitSink(unsigned int /* bitPosition */ , BitSink *)
  {
    return false;
  }
};



//---------------------------------------------------------
// Program Counter
//
class PCTraceType;
class Program_Counter : public Value {
public:
  unsigned int value;              // pc's current value
  unsigned int memory_size;
  unsigned int pclath_mask;        // pclath confines PC to banks
  unsigned int instruction_phase;
  unsigned int trace_state;        // used while reconstructing the trace history

  // Trace commands
  unsigned int trace_increment;
  unsigned int trace_branch;
  unsigned int trace_skip;
  unsigned int trace_other;

  Program_Counter(const char *name, const char *desc, Module *pM);
  ~Program_Counter();

  // A simple helper for producing a warning/error message
  virtual void bounds_error ( const char * func, const char * test, unsigned int val );

  virtual void increment();
  virtual void start_skip();
  virtual void skip();
  virtual void jump(unsigned int new_value);
  virtual void interrupt(unsigned int new_value);
  virtual void computed_goto(unsigned int new_value);
  virtual void new_address(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
  virtual void update_pcl();
  void get(char *buffer, int buf_size) override;
  unsigned int get_value() override
  {
    return value;
  }
  virtual unsigned int pcl_read()
  {
    return value;
  }
  virtual unsigned int get_PC()
  {
    return value;
  }

  virtual void set_PC(unsigned int new_value)
  {
    value = new_value;
    this->update();
  }

  /// set --- cast another Value object type into a program counter register type
  /// this is used primarily by expression and stimuli processing
  /// (the put() methods are used by the processors).
  /// FIXME -- consolidate the get, set, and put methods
  void set(Value *) override;

  // initialize the dynamically allocated trace type
  virtual void set_trace_command();

  /// get_raw_value -- on the 16-bit cores, get_value is multiplied by 2
  /// whereas get_raw_value isn't. The raw value of the program counter
  /// is used as an index into the program memory.
  virtual unsigned int get_raw_value()
  {
    return value;
  }

  virtual void set_phase(int phase)
  {
    instruction_phase = phase;
  }
  virtual int get_phase()
  {
    return instruction_phase;
  }

  void set_reset_address(unsigned int _reset_address)
  {
    reset_address = _reset_address;
  }
  unsigned int get_reset_address()
  {
    return reset_address;
  }

  void reset();

  virtual unsigned int get_next();

  virtual void put_trace_state(unsigned int ts)
  {
    trace_state = ts;
  }

protected:
  unsigned int reset_address;      /* Value pc gets at reset */
  PCTraceType *m_pPCTraceType;
};


// Used in the command prompt interface
class RegisterCollection : public IIndexedCollection {
public:
  RegisterCollection(Processor *pProcessor,
                     const char *collection_name,
                     Register   **ppRegisters,
                     unsigned int uiSize);
  ~RegisterCollection();

  void get(char *return_str, int len) override;
  unsigned int GetSize() override;
  Value &GetAt(unsigned int uIndex, Value *pValue = nullptr) override;
  void SetAt(unsigned int uIndex, Value *pValue) override;
  void ConsolidateValues(int &iColumnWidth,
                         std::vector<std::string> &aList,
                         std::vector<std::string> &aValue) override;
  //  virtual void SetAt(ExprList_t* pIndexers, Expression *pExpr);
  unsigned int GetLowerBound() override;
  unsigned int GetUpperBound() override;

private:
  Processor *   m_pProcessor;
  Register **   m_ppRegisters;
  unsigned int  m_uSize;
  Integer       m_ReturnValue;
};


//------------------------------------------------------------------------
// BitSink
//
// A BitSink is an object that can direct bit changes in an SFR to some
// place where they're needed. The purpose is to abstract the interface
// between special bits and the various peripherals.
//
// A client wishing to be notified whenever an SFR bit changes states
// will create a BitSink object and pass its pointer to the SFR. The
// client will also tell the SFR which bit this applies to. Now, when
// the bit changes states in the SFR, the SFR will call the setSink()
// method.

class BitSink {
public:
  virtual ~BitSink()
  {
  }

  virtual void setSink(bool) = 0;
};


#endif // SRC_REGISTERS_H_
