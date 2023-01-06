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


#ifndef SRC_PIC_INSTRUCTIONS_H_
#define SRC_PIC_INSTRUCTIONS_H_

#include <glib.h>
#include <string>

#include "gpsim_object.h"
#include "value.h"

class Expression;
class Processor;
class Register;

// FIXME get rid of AddressSymbol and LineNumberSymbol classes

class AddressSymbol : public Integer {
public:
  AddressSymbol(Processor *pCpu, const char *, unsigned int);
  std::string toString() override;
  Value* evaluate() override;
  int set_break(ObjectBreakTypes bt = eBreakAny,
                ObjectActionTypes at = eActionHalt,
                Expression *expr = nullptr) override;
};


class LineNumberSymbol : public AddressSymbol {
protected:
  int src_id = 0, src_line = 0, lst_id = 0, lst_line = 0, lst_page = 0;
public:

  LineNumberSymbol(Processor *pCpu, const char *, unsigned int);
  void put_address(int new_address)
  {
    set(new_address);
  }
  void put_src_line(int new_src_line)
  {
    src_line = new_src_line;
  }
  void put_lst_line(int new_lst_line)
  {
    lst_line = new_lst_line;
  }
  void put_lst_page(int new_lst_page)
  {
    lst_page = new_lst_page;
  }
};


/*
 *  base class for an instruction
 */

class instruction : public Value {
public:
  enum INSTRUCTION_TYPES {
    NORMAL_INSTRUCTION,
    INVALID_INSTRUCTION,
    BREAKPOINT_INSTRUCTION,
    NOTIFY_INSTRUCTION,
    PROFILE_START_INSTRUCTION,
    PROFILE_STOP_INSTRUCTION,
    MULTIWORD_INSTRUCTION,
    ASSERTION_INSTRUCTION
  };

  /*
   * Not all instructions derived from the instruction
   * class use these constants...
   */

  enum {
    REG_MASK_12BIT = 0x1f,
    REG_MASK_14BIT = 0x7f,
    REG_MASK_16BIT = 0xff,
    DESTINATION_MASK_12BIT = 0x20,
    DESTINATION_MASK_14BIT = 0x80,
    DESTINATION_MASK_16BIT = 0x200,
    ACCESS_MASK_16BIT = 0x100,
  };

  instruction(Processor *pProcessor, unsigned int uOpCode, unsigned int uAddrOfInstr);
  instruction(const instruction &) = delete;
  instruction& operator = (const instruction &) = delete;
  virtual ~instruction();

  virtual void execute() = 0;
  virtual void debug() {}
  virtual int instruction_size()
  {
    return 1;
  }
  virtual unsigned int get_opcode()
  {
    return opcode;
  }
  unsigned int get_value() override
  {
    return opcode;
  }
  virtual void put_value(unsigned int /* new_value */ ) {}
  virtual unsigned int getAddress()
  {
    return m_uAddrOfInstr;
  }
  virtual void setAddress(unsigned int addr)
  {
    m_uAddrOfInstr = addr;
  }
  virtual int get_src_line()
  {
    return src_line;
  }
  virtual int get_hll_src_line()
  {
    return hll_src_line;
  }
  virtual void set_hll_src_line(int line)
  {
    hll_src_line = line;
  }
  virtual int get_lst_line()
  {
    return lst_line;
  }
  virtual int get_file_id()
  {
    return file_id;
  }
  virtual int get_hll_file_id()
  {
    return hll_file_id;
  }
  virtual void set_hll_file_id(int file_id)
  {
    hll_file_id = file_id;
  }
  virtual enum INSTRUCTION_TYPES isa()
  {
    return NORMAL_INSTRUCTION;
  }
  virtual guint64 getCyclesUsed()
  {
    return cycle_count;
  }
  virtual bool isBase() = 0;
  void decode(Processor *new_cpu, unsigned int new_opcode);
  virtual void update_line_number(int file, int sline, int lline, int hllfile, int hllsline);

  virtual char *ReadSrcLine(char *buf, int nBytes);
  virtual char *ReadLstLine(char *buf, int nBytes);
  virtual char *ReadHLLLine(char *buf, int nBytes);

  int set_break(ObjectBreakTypes bt = eBreakAny,
                ObjectActionTypes at = eActionHalt,
                Expression *expr = nullptr) override;
  virtual void addLabel(std::string &rLabel);

  // Some instructions require special initialization after they've
  // been instantiated. For those that do, the instruction base class
  // provides a way to control the initialization state (see the 16-bit
  // PIC instructions).
  virtual void initialize(bool /* init_state */ ) {}

  bool bIsModified()
  {
    return m_bIsModified;
  }
  void setModified(bool b)
  {
    m_bIsModified = b;
  }
  gpsimObject *getLineSymbol()
  {
    return pLineSymbol;
  }
protected:
  bool m_bIsModified = false; // flag indicating if this instruction has
  // changed since start.
  guint64 cycle_count = 0; // Nr of cycles used up by this instruction

  unsigned int opcode;
  unsigned int m_uAddrOfInstr;
  gpsimObject *pLineSymbol = nullptr;
  int file_id = -1;            // The source file that declared this instruction
                          // (The file_id is an index into an array of files)
  int hll_file_id = -1;        // The hll source file that declared this instruction
  int src_line = -1;           // The line number within the source file
  int lst_line = -1;           // The line number within the list file
  int hll_src_line = -1;       // The line number within the HLL source file
};


//---------------------------------------------------------
// An AliasedInstruction is a class that is designed to replace an
// instruction in program memory. (E.g. breakpoint instructions are an
// example).
class AliasedInstruction : public instruction {
public:
  explicit AliasedInstruction(instruction *);
  AliasedInstruction();
  AliasedInstruction(Processor *pProcessor,
                     unsigned int uOpCode,
                     unsigned int uAddrOfInstr);
  ~AliasedInstruction();

  void setReplaced(instruction *);
  virtual instruction *getReplaced();

  void execute() override;
  void debug() override;
  int instruction_size() override;
  unsigned int get_opcode() override;
  unsigned int get_value() override;
  void put_value(unsigned int new_value) override;
  int get_src_line() override;
  int get_hll_src_line() override;
  int get_lst_line() override;
  int get_file_id() override;
  int get_hll_file_id() override;
  void update_line_number(int file, int sline, int lline, int hllfile, int hllsline) override;
  enum INSTRUCTION_TYPES isa() override;
  void initialize(bool init_state) override;
  char *name(char *, int len) override;
  bool isBase() override;

  void update() override;
  void add_xref(void *xref) override;
  void remove_xref(void *xref) override;

protected:
  instruction *m_replaced;
};


//---------------------------------------------------------
class invalid_instruction : public instruction {
public:
  invalid_instruction(Processor *new_cpu,
                      unsigned int new_opcode,
                      unsigned int address);

  void execute() override;

  void debug() override
  {
    //cout << "*** INVALID INSTRUCTION ***\n";
  }

  enum INSTRUCTION_TYPES isa() override
  {
    return INVALID_INSTRUCTION;
  }
  //virtual char *name(char *str){return("INVALID");};
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {
    return new invalid_instruction(new_cpu, new_opcode, address);
  }
  void addLabel(std::string &rLabel) override;
  bool isBase() override
  {
    return true;
  }
};


//---------------------------------------------------------
class Literal_op : public instruction {
public:
  Literal_op(Processor *pProcessor, unsigned int uOpCode, unsigned int uAddrOfInstr);

  unsigned int L;

  void debug() override {}
  char *name(char *, int) override;
  bool isBase() override
  {
    return true;
  }

  void decode(Processor *new_cpu, unsigned int new_opcode);
};


//---------------------------------------------------------
class Bit_op : public instruction {
public:
  Bit_op(Processor *pProcessor, unsigned int uOpCode, unsigned int uAddrOfInstr);

  unsigned int mask = 0, register_address = 0;
  bool access = false;
  Register *reg = nullptr;

  void debug() override {}
  char *name(char *, int) override;
  bool isBase() override
  {
    return true;
  }

  void decode(Processor *new_cpu, unsigned int new_opcode);
};


//---------------------------------------------------------
class Register_op : public instruction {
public:
  Register_op(Processor *pProcessor, unsigned int uOpCode, unsigned int uAddrOfInstr);

  static Register *source;
  unsigned int register_address = 0;
  bool destination = false;
  bool access = false;

  /*  Register *destination;*/

  void debug() override {}
  char *name(char *, int) override;
  bool isBase() override
  {
    return true;
  }

  void decode(Processor *new_cpu, unsigned int new_opcode);
};


//-----------------------------------------------------------------
//
// instruction_constructor - a class used to create the PIC instructions
//
// The way it works is the 'instruction_constructor' structure
// contains three pieces of info for each instruction:
//   inst_mask - a bit mask indicating which bits uniquely
//               identify an instruction
//   opcode    - What those unique bits should be
//   inst_constructor - A pointer to the static member function
//                      'construct' in the instruction class.
//
// An instruction is decoded by finding a matching entry in
// the instruction_constructor array. A match is defined to
// be:
//    inst_mask & passed_opcode == opcode
// which means that the opcode that is passed to the decoder
// is ANDed with the instruction mask bits and compared to
// the base bits of the opcode. If this test passes, then the
// 'inst_constructor' will be called.

struct instruction_constructor {
  unsigned int inst_mask;
  unsigned int opcode;
  instruction * (*inst_constructor)(Processor *cpu, unsigned int inst, unsigned int address);
};


#endif  //  SRC_PIC_INSTRUCTIONS_H_
