/*
   Copyright (C) 1998-2003 T. Scott Dattalo

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

/*
  stuff that needs to be fixed:

  Register aliasing
  The "invalid instruction" in program memory.

*/

#ifdef _WIN32
#include "uxtime.h"
#endif

#include <string.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <memory>
#include <string>
#include <sstream>
#include <typeinfo>

#include "processor.h"

#include <config.h>

#include "14bit-registers.h"
#include "attributes.h"
#include "errors.h"
#include "gpsim_classes.h"
#include "gpsim_interface.h"
#include "gpsim_time.h"
#include "interface.h"
#include "modules.h"
#include "pic-processor.h"
#include "sim_context.h"
#include "stimuli.h"
#include "trace.h"
#include "ui.h"


#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

//#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("%s:%d-%s() ",__FILE__,__LINE__,__FUNCTION__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

//------------------------------------------------------------------------
// active_cpu  is a pointer to the pic processor that is currently 'active'.
// 'active' means that it's the one currently being simulated or the one
// currently being manipulated by the user (e.g. register dumps, break settings)

Processor *active_cpu;

// create instances of inline get_active_cpu() and set_active_cpu() methods
// by taking theirs address
Processor *(*dummy_get_active_cpu)(void) = get_active_cpu;
void (*dummy_set_active_cpu)(Processor *act_cpu) = set_active_cpu;

static char pkg_version[] = PACKAGE_VERSION;

class CPU_Freq : public Float {
public:
  CPU_Freq(Processor * _cpu, double freq);

  void set(double d) override;

  void set_rc_freq(double d)
  {
    RCfreq = d;
    if (use_rc_freq) Float::set(d);
  }

  void set_rc_active(bool _use_rc_freq)
  {
    use_rc_freq = _use_rc_freq;
    Float::set(use_rc_freq ? RCfreq : freq);
  }

private:
  Processor * cpu;
  double    freq;
  double 	RCfreq = 0;
  bool		use_rc_freq = false;
};


CPU_Freq::CPU_Freq(Processor * _cpu, double freq)
  : Float("frequency", freq, " oscillator frequency."),
    cpu(_cpu), freq(freq)
{
}


void CPU_Freq::set(double d)
{
  pic_processor *pCpu = dynamic_cast<pic_processor *>(cpu);
  freq = d;
  if (!use_rc_freq) Float::set(d);

  if (cpu) {
    cpu->update_cps();
  }

  if (pCpu) {
    pCpu->wdt->update();
  }
}


CPU_Vdd::CPU_Vdd(Processor * _cpu, double vdd)
  : Float("Vdd", vdd, "Processor supply voltage"),
    cpu(_cpu)
{
}


void CPU_Vdd::set(double d)
{
  Float::set(d);

  if (cpu) {
    cpu->update_vdd();
  }
}


//------------------------------------------------------------------------
//
// Processor - Constructor
//

Processor::Processor(const char *_name, const char *_desc)
  : Module(_name, _desc),
    pma(nullptr),
    rma(this),
    ema(this),
    pc(nullptr),
    bad_instruction(this, 0x3fff, 0),
    mFrequency(nullptr)
{
  registers = nullptr;
  m_pConstructorObject = nullptr;
  m_Capabilities = 0;
  m_ProgramMemoryAllocationSize = 0;

  if (verbose) {
    std::cout << "processor constructor\n";
  }

  addSymbol(mFrequency = new CPU_Freq(this, 20e6));
  set_ClockCycles_per_Instruction(4);
  update_cps();
  setWarnMode(true);
  setSafeMode(true);
  setUnknownMode(true);
  setBreakOnReset(true);
  // derived classes need to override these values
  m_uPageMask    = 0x00;
  m_uAddrMask    = 0xff;
  interface = new ProcessorInterface(this);
  // let the processor version number simply be gpsim's version number.
  version = &pkg_version[0];
  emplace_trace<trace::CycleCounterEntry>(get_cycles().get());
  addSymbol(m_pWarnMode = new WarnModeAttribute(this));
  addSymbol(m_pSafeMode = new SafeModeAttribute(this));
  addSymbol(m_pUnknownMode = new UnknownModeAttribute(this));
  addSymbol(m_pBreakOnReset = new BreakOnResetAttribute(this));
  m_vdd = new CPU_Vdd(this, 5.0);
  addSymbol(m_vdd);
  m_pbBreakOnInvalidRegisterRead = new Boolean("BreakOnInvalidRegisterRead",
      true, "Halt simulation when an invalid register is read from.");
  addSymbol(m_pbBreakOnInvalidRegisterRead);
  m_pbBreakOnInvalidRegisterWrite = new Boolean("BreakOnInvalidRegisterWrite",
      true, "Halt simulation when an invalid register is written to.");
  addSymbol(m_pbBreakOnInvalidRegisterWrite);
  set_Vdd(5.0);
}


//-------------------------------------------------------------------
Processor::~Processor()
{
  deleteSymbol(m_pbBreakOnInvalidRegisterRead);
  deleteSymbol(m_pbBreakOnInvalidRegisterWrite);
  deleteSymbol(m_pWarnMode);
  deleteSymbol(m_pSafeMode);
  deleteSymbol(m_pUnknownMode);
  deleteSymbol(m_pBreakOnReset);
  deleteSymbol(mFrequency);
  deleteSymbol(m_vdd);
  delete interface;
  delete_invalid_registers();
  delete []registers;
  destroyProgramMemoryAccess(pma);

  for (unsigned int i = 0; i < m_ProgramMemoryAllocationSize; i++) {
    if (program_memory[i] != &bad_instruction) {
      delete program_memory[i];
    }
  }

  delete []program_memory;
}


unsigned long Processor::GetCapabilities()
{
  return m_Capabilities;
}


//-------------------------------------------------------------------
// Simulation modes:
void Processor::setWarnMode(bool newWarnMode)
{
  bWarnMode = newWarnMode;
}


void Processor::setSafeMode(bool newSafeMode)
{
  bSafeMode = newSafeMode;
}


void Processor::setUnknownMode(bool newUnknownMode)
{
  bUnknownMode = newUnknownMode;
}


void Processor::setBreakOnReset(bool newBreakOnReset)
{
  bBreakOnReset = newBreakOnReset;
}


//------------------------------------------------------------------------
// Attributes


void Processor::set_RCfreq_active(bool state)
{
  if (mFrequency) {
    mFrequency->set_rc_active(state);
  }

  update_cps();
}


void Processor::set_frequency(double f)
{
  if (mFrequency) {
    mFrequency->set(f);
  }

  update_cps();
}


void Processor::set_frequency_rc(double f)
{
  if (mFrequency) {
    mFrequency->set_rc_freq(f);
  }

  update_cps();
}


double Processor::get_frequency()
{
  return mFrequency ? mFrequency->get() : 0;
}


void Processor::update_cps()
{
  get_cycles().set_instruction_cps((uint64_t)(get_frequency() / clocks_per_inst));
}


double  Processor::get_OSCperiod()
{
  double f = get_frequency();

  if (f > 0.0) {
    return 1.0 / f;

  } else {
    return 0.0;
  }
}


//-------------------------------------------------------------------
//
// init_register_memory (unsigned int memory_size)
//
// Allocate an array for holding register objects.
//

void Processor::init_register_memory(unsigned int memory_size)
{
  if (verbose) {
    std::cout << __FUNCTION__ << " memory size: " << memory_size << '\n';
  }

  registers = new Register *[memory_size];
  // For processors with banked memory, the register_bank corresponds to the
  // active bank. Let this point to the beginning of the register array for
  // now.
  register_bank = registers;
  rma.set_Registers(registers, memory_size);
  // Make all of the file registers 'undefined' (each processor derived from this base
  // class defines its own register mapping).
  std::fill_n(registers, memory_size, nullptr);
}


//-------------------------------------------------------------------
//
//
// create_invalid_registers
//
//   The purpose of this function is to complete the initialization
// of the file register memory by placing an instance of an 'invalid
// file register' at each 'invalid' memory location. Most of PIC's
// do not use the entire address space available, so this routine
// fills the voids.
//

void Processor::create_invalid_registers()
{
  unsigned int addr;

  if (verbose) {
    std::cout << "Creating invalid registers " << register_memory_size() << '\n';
  }

  // Now, initialize any undefined register as an 'invalid register'
  // Note, each invalid register is given its own object. This enables
  // the simulation code to efficiently capture any invalid register
  // access. Furthermore, it's possible to set break points on
  // individual invalid file registers. By default, gpsim halts whenever
  // there is an invalid file register access.

  for (addr = 0; addr < register_memory_size(); addr += map_rm_index2address(1)) {
    unsigned int index = map_rm_address2index(addr);

    if (!registers[index]) {
      char nameBuff[100];
      snprintf(nameBuff, sizeof(nameBuff), "INVREG_%X", addr);
      registers[index] = new InvalidRegister(this, nameBuff, nullptr, addr);
    }
  }
}


//-------------------------------------------------------------------
//
// Delete invalid registers
//
void Processor::delete_invalid_registers()
{
  unsigned int i = 0;

  for (i = 0; i < rma.get_size(); i++) {
    // cout << __FUNCTION__ << "  reg: 0x"<<hex << i << " ptr:" << registers[i] << endl;
    InvalidRegister *pReg = dynamic_cast<InvalidRegister *>(registers[i]);

    if (pReg) {
      delete registers[i];
      registers[i] = nullptr;

    } else if (registers[i]) {
      std::cout << __FUNCTION__ << "  reg: 0x" << std::hex << i << " ptr:" << registers[i];
      std::cout << ' ' << registers[i]->name().substr(0, 10) << '\n';
    }
  }
}


//-------------------------------------------------------------------
//    add_file_registers
//
//  The purpose of this member function is to allocate memory for the
// general purpose registers.
//

void Processor::add_file_registers(unsigned int start_address, unsigned int end_address, unsigned int alias_offset)
{
  unsigned int j;
  // Initialize the General Purpose Registers:
  Dprintf((" from 0x%x to 0x%x alias 0x%x\n", start_address, end_address, alias_offset));
  char str[100];

  for (j = start_address; j <= end_address; j++) {
#ifdef DEBUG

    if (j == 0x11) {
      printf("Processor::add_file_registers j 0x%x\n", j);
    }

#endif

    if (registers[j] && (registers[j]->isa() == Register::INVALID_REGISTER)) {
      delete registers[j];

    } else if (registers[j])
      std::cout << __FUNCTION__ << " Already register " << registers[j]->name()
                << " at 0x" << std::hex << j << '\n';

    //The default register name is simply its address
    snprintf(str, sizeof(str), "REG%03X", j);
    registers[j] = new Register(this, str, nullptr, j);

    if (alias_offset) {
      registers[j + alias_offset] = registers[j];
      registers[j]->alias_mask = alias_offset;

    } else {
      registers[j]->alias_mask = 0;
    }
  }
}


//-------------------------------------------------------------------
//    delete_file_registers
//
//  The purpose of this member function is to delete file registers
//

void Processor::delete_file_registers(unsigned int start_address,
                                      unsigned int end_address,
                                      bool bRemoveWithoutDelete)
{
#define DFR_DEBUG 0

  if (DFR_DEBUG)
    std::cout << __FUNCTION__
              << "  start:" << std::hex << start_address
              << "  end:" << std::hex << end_address
              << '\n';

  //  FIXME - this function is bogus.
  // The aliased registers do not need to be searched for - the alias mask
  // can tell at what addresses a register is aliased.
#define SMALLEST_ALIAS_DISTANCE  32
#define ALIAS_MASK (SMALLEST_ALIAS_DISTANCE-1)
  unsigned int i, j;

  if (start_address != end_address) {
    Dprintf(("from 0x%x to 0x%x\n", start_address, end_address));
  }

  for (j = start_address; j <= end_address; j++) {
    if (registers[j]) {
      Register *thisReg = registers[j];
      Register *replaced = thisReg->getReplaced();

      if (thisReg->alias_mask) {
        // This register appears in more than one place. Let's find all
        // of its aliases.
        for (i = j & ALIAS_MASK; i < rma.get_size(); i += SMALLEST_ALIAS_DISTANCE)
          if (thisReg == registers[i]) {
            if (DFR_DEBUG) {
              std::cout << "   removing at address:" << std::hex << i << '\n';
            }

            registers[i] = nullptr;
          }
      }

      if (DFR_DEBUG) {
        std::cout << " deleting: " << std::hex << j << '\n';
      }

      registers[j] = nullptr;

      if (!bRemoveWithoutDelete) {
        delete replaced;
        delete thisReg;
      }

    } else {
      printf("%s register 0x%x already deleted\n", __FUNCTION__, j);
    }
  }
}


//-------------------------------------------------------------------
//
//
//    alias_file_registers
//
//  The purpose of this member function is to alias the
// general purpose registers.
//

void Processor::alias_file_registers(unsigned int start_address, unsigned int end_address, unsigned int alias_offset)
{
  unsigned int j;
  // FIXME -- it'd probably make better sense to keep a list of register addresses at
  // which a particular register appears.
#ifdef DEBUG

  if (start_address == 0x20) {
    printf("DEBUG trace %x\n", start_address);
  }

  if (start_address != end_address) {
    Dprintf((" from 0x%x to 0x%x alias_offset 0x%x\n", start_address, end_address, alias_offset));
  }

#endif

  for (j = start_address; j <= end_address; j++) {
    if (alias_offset && (j + alias_offset < rma.get_size())) {
      if (registers[j + alias_offset]) {
        if (registers[j + alias_offset] == registers[j]) {
          printf("alias_file_register Duplicate alias %s from 0x%x to 0x%x \n", registers[j + alias_offset]->name().c_str(), j, j + alias_offset);

        } else {
          delete registers[j + alias_offset];
        }
      }

      registers[j + alias_offset] = registers[j];

      if (registers[j]) {
        registers[j]->alias_mask = alias_offset;
      }
    }
  }
}


//-------------------------------------------------------------------
//
// init_program_memory(unsigned int memory_size)
//
// The purpose of this member function is to allocate memory for the
// pic's code space. The 'memory_size' parameter tells how much memory
// is to be allocated
//
//  The following is not correct for 18f2455 and 18f4455 processors
//  so test has been disabled (RRR)
//
//  AND it should be an integer of the form of 2^n.
// If the memory size is not of the form of 2^n, then this routine will
// round up to the next integer that is of the form 2^n.
//
//   Once the memory has been allocated, this routine will initialize
// it with the 'bad_instruction'. The bad_instruction is an instantiation
// of the instruction class that chokes gpsim if it is executed. Note that
// each processor owns its own 'bad_instruction' object.

void Processor::init_program_memory(unsigned int memory_size)
{
  if (verbose) {
    std::cout << "Initializing program memory: 0x" << memory_size << " words\n";
  }

#ifdef RRR

  if ((memory_size - 1) & memory_size) {
    std:: cout << "*** WARNING *** memory_size should be of the form 2^N\n";
    memory_size = (memory_size + ~memory_size) & MAX_PROGRAM_MEMORY;
    std::cout << "gpsim is rounding up to memory_size = " << memory_size << '\n';
  }

#endif
  // Initialize 'program_memory'. 'program_memory' is a pointer to an array of
  // pointers of type 'instruction'. This is where the simulated instructions
  // are stored.
  program_memory = new instruction *[memory_size];
  m_ProgramMemoryAllocationSize = memory_size;
  std::fill_n(program_memory, memory_size, &bad_instruction);
  pma = createProgramMemoryAccess(this);
  pma->name();
}


ProgramMemoryAccess * Processor::createProgramMemoryAccess(Processor *processor)
{
  return new ProgramMemoryAccess(processor);
}


void Processor::destroyProgramMemoryAccess(ProgramMemoryAccess *pma)
{
  delete pma;
}


//-------------------------------------------------------------------
// init_program_memory(int address, int value)
//
// The purpose of this member fucntion is to instantiate an Instruction
// object in the program memory. If the opcode is invalid, then a 'bad_instruction'
// is inserted into the program memory instead. If the address is beyond
// the program memory address space, then it may be that the 'opcode' is
// is in fact a configuration word.
//

void Processor::init_program_memory(unsigned int address, unsigned int value)
{
  unsigned int uIndex = map_pm_address2index(address);

  if (!program_memory) {
    const char *buf = "ERROR: internal bug " __FILE__ ":" STR(__LINE__);
    throw FatalError(buf);
  }

  if (uIndex < program_memory_size()) {
    if (program_memory[uIndex] != 0 && program_memory[uIndex]->isa() != instruction::INVALID_INSTRUCTION) {
      // this should not happen
      delete program_memory[uIndex];
    }

    program_memory[uIndex] = disasm(address, value);

    if (program_memory[uIndex] == 0) {
      program_memory[uIndex] = &bad_instruction;
    }

    //program_memory[uIndex]->add_line_number_symbol();

  } else if (set_config_word(address, value)) {
  } else {
    set_out_of_range_pm(address, value);  // could be e2prom
  }
}


//-------------------------------------------------------------------
//erase_program_memory(unsigned int address)
//
//	Checks if a program memory location contains an instruction
//	and deletes it if it does.
//
void Processor::erase_program_memory(unsigned int address)
{
  unsigned int uIndex = map_pm_address2index(address);

  if (!program_memory) {
    const char *buf = "ERROR: internal bug " __FILE__ ":" STR(__LINE__);
    throw FatalError(buf);
  }

  if (uIndex < program_memory_size()) {
    if (program_memory[uIndex] != 0 && program_memory[uIndex]->isa() != instruction::INVALID_INSTRUCTION) {
      delete program_memory[uIndex];
      program_memory[uIndex] = &bad_instruction;
    }

  } else {
    std::cout << "Erase Program memory\n";
    std::cout << "Warning::Out of range address " << std::hex << address << '\n';
    std::cout << "Max allowed address is 0x" << std::hex << (program_address_limit() - 1) << '\n';
  }
}


void Processor::init_program_memory_at_index(unsigned int uIndex, unsigned int value)
{
  init_program_memory(map_pm_index2address(uIndex), value);
}


void Processor::init_program_memory_at_index(unsigned int uIndex,
    const unsigned char *bytes, int nBytes)
{
  for (int i = 0; i < nBytes / 2; i++) {
    init_program_memory_at_index(uIndex + i, (((unsigned int)bytes[2 * i + 1]) << 8)  | bytes[2 * i]);
  }
}


//------------------------------------------------------------------
// Fetch the rom contents at a particular address.
unsigned int Processor::get_program_memory_at_address(unsigned int address)
{
  unsigned int uIndex = map_pm_address2index(address);
  return (uIndex < program_memory_size() && program_memory[uIndex])
         ? program_memory[uIndex]->get_opcode()
         : 0xffffffff;
}


//-------------------------------------------------------------------
// build_program_memory - given an array of opcodes this function
// will convert them into instructions and insert them into the
// simulated program memory.
//

void Processor::build_program_memory(unsigned int *memory,
                                     unsigned int minaddr,
                                     unsigned int maxaddr)
{
  for (unsigned int i = minaddr; i <= maxaddr; i++)
    if (memory[i] != 0xffffffff) {
      init_program_memory(i, memory[i]);
    }
}


//-------------------------------------------------------------------
/** @brief Write a word of data into memory outside flash
 *
 *  This method is called when loading data from the COD or HEX file
 *  and the address is not in the program ROM or normal config space.
 *  In this base class, there is no such memory. Real processors,
 *  particularly those with EEPROM, will need to override this method.
 *
 *  @param  address Memory address to set. Byte address on 18F
 *  @param  value   Word data to write in.
 */
void Processor::set_out_of_range_pm(unsigned int address, unsigned int value)
{
  std::cout << "Warning::Out of range address " << address << " value " << value << '\n';
  std::cout << "Max allowed address is 0x" << std::hex << (program_address_limit() - 1) << '\n';
}


//-------------------------------------------------------------------
//
// disassemble - Disassemble the contents of program memory from
// 'start_address' to 'end_address'. The instruction at the current
// PC is marked with an arrow '==>'. If an instruction has a break
// point set on it then it will be marked with a 'B'. The instruction
// mnemonics come from the class declarations for each instruction.
// However, it is possible to modify this on a per instruction basis.
// In other words, each instruction in the program memory has it's
// own instantiation. So a MOVWF at address 0x20 is different than
// one at address 0x21. It is possible to change the mnemonic of
// one without affecting the other. As of version 0.0.7 though, this
// is not implemented.
//

void Processor::disassemble(signed int s, signed int e)
{
  instruction *inst;

  if (s > e) {
    return;
  }

  unsigned int start_PMindex = map_pm_address2index(s);
  unsigned int end_PMindex   = map_pm_address2index(e);

  if (start_PMindex >= program_memory_size()) {
    if (s < 0) {
      start_PMindex = 0;

    } else {
      return;
    }
  }

  if (end_PMindex  >= program_memory_size()) {
    if (e < 0) {
      return;

    } else {
      end_PMindex = program_memory_size() - 1;
    }
  }

  const int iConsoleWidth = 80;
  char str[iConsoleWidth];

  if (!pc) {
    const char *buf = "ERROR: internal bug " __FILE__ ":" STR(__LINE__);
    throw FatalError(buf);
  }

  unsigned uPCAddress = pc->get_value();
  ISimConsole &Console = GetUserInterface().GetConsole();

  for (unsigned int PMindex = start_PMindex; PMindex <= end_PMindex; PMindex++) {
    unsigned int uAddress = map_pm_index2address(PMindex);
    str[0] = 0;
    const char *pszPC = (uPCAddress == uAddress) ? "==>" : "   ";
    inst = program_memory[PMindex];
    // If this is not a "base" instruction then it has been replaced
    // with something like a break point.
    char cBreak = ' ';

    if (!inst->isBase()) {
      cBreak = 'B';
      inst = pma->getFromIndex(PMindex);
    }

    inst->name(str, sizeof(str));
    char *pAfterNumonic = strchr(str, '\t');
    int iNumonicWidth = pAfterNumonic ? pAfterNumonic - str : 5;
    int iOperandsWidth = 14;
    int iSrc = iOperandsWidth - (strlen(str) - iNumonicWidth - 1);
    const char *pFormat = (opcode_size() <= 2) ?  "% 3s%c%04x  %04x  %s %*s%s\n" :  "% 3s%c%04x  %06x  %s %*s\n";
    Console.Printf(pFormat,
                   pszPC, cBreak, uAddress, inst->get_opcode(),
                   str, iSrc, "");
  }
}


/* If Vdd is changed, fix up the digital high low thresholds */
void Processor::update_vdd()
{
  for (int i = 1; i <= get_pin_count(); i++) {
    IOPIN *pin = get_pin(i);

    if (pin) {
      pin->set_digital_threshold(get_Vdd());
    }
  }
}


//-------------------------------------------------------------------
//
Processor * Processor::construct()
{
  std::cout << " Can't create a generic processor\n";
  return nullptr;
}


//-------------------------------------------------------------------
//
// step_over - In most cases, step_over will simulate just one instruction.
// However, if the next instruction is a branching one (e.g. goto, call,
// return, etc.) then a break point will be set after it and gpsim will
// begin 'running'. This is useful for stepping over time-consuming calls.
//

void Processor::step_over()
{
  step([](unsigned int step) { return false; }); // Try one step
}


//-------------------------------------------------------------------
//
//
//    create
//
//  The purpose of this member function is to 'create' a pic processor.
// Since this is a base class member function, only those things that
// are common to all pics are created.

void Processor::create()
{
  const char *buf = " a generic processor cannot be created " __FILE__ ":" STR(__LINE__);
  throw FatalError(buf);
}


//-------------------------------------------------------------------
void Processor::dump_registers()
{
  //  parse_string("dump");
}


//-------------------------------------------------------------------
void Processor::Debug()
{
  std::cout << " === Debug === \n";

  if (pc) {
    std::cout << "PC=0x" << std::hex << pc->value << '\n';
  }
}


//-------------------------------------------------------------------
uint64_t Processor::cycles_used(unsigned int address)
{
  return program_memory[address]->getCyclesUsed();
}


//-------------------------------------------------------------------
MemoryAccess::MemoryAccess(Processor *new_cpu)
  : cpu(new_cpu)
{
}


MemoryAccess::~MemoryAccess()
{
}


Processor *MemoryAccess::get_cpu()
{
  return cpu;
}


//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
// ProgramMemoryAccess
//
// The ProgramMemoryAccess class provides an interface to the processor's
// program memory. On Pic processors, this is the memory where instructions
// are stored.
//

ProgramMemoryAccess::ProgramMemoryAccess(Processor *new_cpu)
  : MemoryAccess(new_cpu)
{
  init(new_cpu);
}


void ProgramMemoryAccess::init(Processor * /* new_cpu */ )
{
  _address = _opcode = _state = 0;

  // add the 'main' pma to the list pma context's. Processors may
  // choose to add multiple pma's to the context list. The gui
  // will build a source browser for each one of these. The purpose
  // is to provide more than one way of debugging the code. (e.g.
  // this is useful for debugging interrupt versus non-interrupt code).

  if (cpu) {
    cpu->pma_context.push_back(this);
  }
}


/*
void ProgramMemoryAccess::name(string & new_name)
{
  name_str = new_name;
}
*/
void ProgramMemoryAccess::putToAddress(unsigned int address, instruction *new_instruction)
{
  putToIndex(cpu->map_pm_address2index(address), new_instruction);
}


void ProgramMemoryAccess::putToIndex(unsigned int uIndex, instruction *new_instruction)
{
  if (!new_instruction) {
    return;
  }

  cpu->program_memory[uIndex] = new_instruction;
}


instruction *ProgramMemoryAccess::getFromAddress(unsigned int address)
{
  if (!cpu || !cpu->IsAddressInRange(address)) {
    return &cpu->bad_instruction;
  }

  unsigned int uIndex = cpu->map_pm_address2index(address);
  return getFromIndex(uIndex);
}


instruction *ProgramMemoryAccess::getFromIndex(unsigned int uIndex)
{
  if (uIndex < cpu->program_memory_size()) {
    return cpu->program_memory[uIndex];

  } else {
    return nullptr;
  }
}


//----------------------------------------
// get_rom - return the rom contents from program memory
//           If the address is normal program memory, then the opcode
//           of the instruction at that address is returned.
//           If the address is some other special memory (like configuration
//           memory in a PIC) then that data is returned instead.

unsigned int ProgramMemoryAccess::get_rom(unsigned int addr)
{
  return cpu->get_program_memory_at_address(addr);
}


//----------------------------------------
// put_rom - write new data to the program memory.
//           If the address is in normal program memory, then a new instruction
//           will be generated (if possible). If the address is some other
//           special memory (like configuration memory), then that area will
//           be updated.
//
void ProgramMemoryAccess::put_rom(unsigned int addr, unsigned int value)
{
  return cpu->init_program_memory(addr, value);
}


//----------------------------------------
// get_opcode - return an opcode from program memory.
//              If the address is out of range return 0.

unsigned int ProgramMemoryAccess::get_opcode(unsigned int addr)
{
  instruction * pInstr = getFromAddress(addr);

  if (pInstr) {
    return pInstr->get_opcode();

  } else {
    return 0;
  }
}


//----------------------------------------
// get_opcode_name - return an opcode name from program memory.
//                   If the address is out of range return 0;

char *ProgramMemoryAccess::get_opcode_name(unsigned int addr, char *buffer, unsigned int size)
{
  unsigned int uIndex = cpu->map_pm_address2index(addr);

  if (uIndex < cpu->program_memory_size()) {
    return cpu->program_memory[uIndex]->name(buffer, size);
  }

  *buffer = 0;
  return nullptr;
}


//----------------------------------------
// Get the current value of the program counter.
unsigned int ProgramMemoryAccess::get_PC()
{
  if (cpu && cpu->pc) {
    return cpu->pc->get_value();
  }

  return 0;
}


//----------------------------------------
// Get the current value of the program counter.
void ProgramMemoryAccess::set_PC(unsigned int new_pc)
{
  if (cpu && cpu->pc) {
    return cpu->pc->put_value(new_pc);
  }
}


Program_Counter *ProgramMemoryAccess::GetProgramCounter()
{
  if (cpu) {
    return cpu->pc;
  }

  return nullptr;
}


void ProgramMemoryAccess::put_opcode_start(unsigned int addr, unsigned int new_opcode)
{
  unsigned int uIndex = cpu->map_pm_address2index(addr);

  if ((uIndex < cpu->program_memory_size()) && (_state == 0)) {
    _state = 1;
    _address = addr;
    _opcode = new_opcode;
    get_cycles().set_break_delta(40000, this);
  }
}


void ProgramMemoryAccess::put_opcode(unsigned int addr, unsigned int new_opcode)
{
  unsigned int uIndex = cpu->map_pm_address2index(addr);

  if (uIndex >= cpu->program_memory_size()) {
    return;
  }

  instruction *old_inst = getFromIndex(uIndex);
  instruction *new_inst = cpu->disasm(addr, new_opcode);

  if (new_inst == nullptr) {
    puts("FIXME, in ProgramMemoryAccess::put_opcode");
    return;
  }

  if (!old_inst) {
    putToIndex(uIndex, new_inst);
    return;
  }

  if (old_inst->isa() == instruction::INVALID_INSTRUCTION) {
    putToIndex(uIndex, new_inst);
    return;
  }

  // Now we need to make sure that the instruction we are replacing is
  // not a multi-word instruction. The 12 and 14 bit cores don't have
  // multi-word instructions, but the 16 bit cores do. If we are replacing
  // the second word of a multiword instruction, then we only need to
  // 'uninitialize' it.
  // if there was a breakpoint set at addr, save a pointer to the breakpoint.
  instruction *prev = getFromIndex(cpu->map_pm_address2index(addr - 1));

  if (prev) {
    prev->initialize(false);
  }

  cpu->program_memory[uIndex] = new_inst;
  cpu->program_memory[uIndex]->setModified(true);
  delete old_inst;
}


//--------------------------------------------------------------------------

bool ProgramMemoryAccess::hasValid_opcode_at_address(unsigned int address)
{
  if (getFromAddress(address)->isa() != instruction::INVALID_INSTRUCTION) {
    return true;
  }

  return false;
}


bool ProgramMemoryAccess::hasValid_opcode_at_index(unsigned int uIndex)
{
  if ((getFromIndex(uIndex))->isa() != instruction::INVALID_INSTRUCTION) {
    return true;
  }

  return false;
}


//--------------------------------------------------------------------------

bool ProgramMemoryAccess::isModified(unsigned int address)     // ***FIXME*** - address or index?
{
  unsigned int uIndex = cpu->map_pm_address2index(address);

  if ((uIndex < cpu->program_memory_size()) &&
      cpu->program_memory[uIndex]->bIsModified()) {
    return true;
  }

  return false;
}


//========================================================================
// Register Memory Access

RegisterMemoryAccess::RegisterMemoryAccess(Processor *new_cpu) :
  MemoryAccess(new_cpu)
{
  registers = nullptr;
  nRegisters = 0;
}


RegisterMemoryAccess::~RegisterMemoryAccess()
{
}


//--------------------------------------------------------------------------

Register *RegisterMemoryAccess::get_register(unsigned int address)
{
  if (!cpu || !registers || nRegisters <= address) {
    return nullptr;
  }

  Register *reg = registers[address];

  return !reg || reg->isa() == Register::INVALID_REGISTER ? nullptr : reg;
}


//--------------------------------------------------------------------------
void RegisterMemoryAccess::set_Registers(Register **_registers, int _nRegisters)
{
  nRegisters = _nRegisters;
  registers = _registers;
}


//------------------------------------------------------------------------
// insertRegister - Each register address may contain a linked list of registers.
// The top most register is the one that is referenced whenever a processor
// accesses the register memory. The primary purpose of the linked list is to
// support register breakpoints. For example, a write breakpoint is implemented
// with a breakpoint class derived from the register class. Setting a write
// breakpoint involves creating the write breakpoint object and placing it
// at the top of the register linked list. Then, when a processor attempts
// to write to this register, the breakpoint object will capture this and
// halt the simulation.

bool RegisterMemoryAccess::insertRegister(unsigned int address, Register *pReg)
{
  if (!cpu || !registers || nRegisters <= address || !pReg) {
    return false;
  }

  Register *ptop = registers[address];
  pReg->setReplaced(ptop);
  registers[address] = pReg;
  return true;
}


//------------------------------------------------------------------------
// removeRegister - see comment on insertRegister. This method removes
// a register object from the breakpoint linked list.

bool RegisterMemoryAccess::removeRegister(unsigned int address, Register *pReg)
{
  if (!cpu || !registers || nRegisters <= address || !pReg) {
    return false;
  }

  Register *ptop = registers[address];

  if (ptop == pReg  &&  pReg->getReplaced()) {
    registers[address] = pReg->getReplaced();

  } else
    while (ptop) {
      Register *pNext = ptop->getReplaced();

      if (pNext == pReg) {
        ptop->setReplaced(pNext->getReplaced());
        return true;
      }

      ptop = pNext;
    }

  return false;
}


//-------------------------------------------------------------------
bool RegisterMemoryAccess::hasBreak(unsigned int address)
{
  if (!cpu || !registers || nRegisters <= address) {
    return false;
  }

  return registers[address]->isa() == Register::BP_REGISTER;
}


static InvalidRegister AnInvalidRegister(0, "AnInvalidRegister");

//-------------------------------------------------------------------
Register &RegisterMemoryAccess::operator [](unsigned int address)
{
  if (!registers || get_size() <= address) {
    return AnInvalidRegister;
  }

  return *registers[address];
}


void RegisterMemoryAccess::reset(RESET_TYPE r)
{
  for (unsigned int i = 0; i < nRegisters; i++) {
    // Do not reset aliased registers
    if (!(operator[](i).alias_mask && (operator[](i).alias_mask & i))) {
      operator[](i).reset(r);
    }
  }
}


//========================================================================
// Processor Constructor

ProcessorConstructor::ProcessorConstructor(tCpuContructor _cpu_constructor,
    const char *name1,
    const char *name2,
    const char *name3,
    const char *name4)
{
  cpu_constructor = _cpu_constructor;  // Pointer to the processor constructor
  names[0] = name1;                    // First name
  names[1] = name2;                    //  and three aliases...
  names[2] = name3;
  names[3] = name4;
  // Add the processor to the list of supported processors:
  GetList()->push_back(this);
}


//------------------------------------------------------------
Processor * ProcessorConstructor::ConstructProcessor(const char *opt_name)
{
  // Instantiate a specific processor. If a name is provided, then that
  // will be used. Otherwise, the third name in the list of aliases for
  // this processor will be used instead. (Why 3rd?... Before optional
  // processor names were allowed, the default name matched what is now
  // the third alias; this maintains a backward compatibility).
  if (opt_name && *opt_name != '\0') {
    return cpu_constructor(opt_name);
  }

  return cpu_constructor(names[2]);
}


ProcessorConstructorList * ProcessorConstructor::processor_list;

ProcessorConstructorList * ProcessorConstructor::GetList()
{
  if (processor_list == nullptr) {
    processor_list = new ProcessorConstructorList();
  }

  return processor_list;
}


//------------------------------------------------------------
// findByType -- search through the list of supported processors for
//               the one matching 'name'.


ProcessorConstructor *ProcessorConstructor::findByType(const char *name)
{
    for (const auto &p : * GetList())
    {
        for (int j = 0; j < nProcessorNames; j++)
        {
            if (p->names[j] && strcmp(name, p->names[j]) == 0)
            {
                return p;
            }
        }
    }

    return nullptr;
}


//------------------------------------------------------------
// dump() --  Print out a list of all of the processors
//

std::string ProcessorConstructor::listDisplayString()
{
    std::ostringstream stream;
    const int nPerRow = 4;   // Number of names to print per row.

    ProcessorConstructorList *pl = GetList();

    // loop through all of the processors and find the
    // one with the longest name
    size_t longest = 0;

    for (const auto &p : *pl)
    {
        longest = std::max(longest, strlen(p->names[1]));
    }

    // Print the name of each processor.

    for (auto processor_iterator = pl->begin(); processor_iterator != pl->end(); )
    {
        for (int i = 0; i < nPerRow && processor_iterator != pl->end(); i++)
        {
            auto p = *processor_iterator++;
            stream << p->names[1];

            if (i < nPerRow - 1)
            {
                // if this is not the last processor in the column, then
                // pad a few spaces to align the columns.
                int k = longest + 2 - strlen(p->names[1]);

                for (int j = 0; j < k; j++)
                {
                    stream << ' ';
                }
            }
        }

        stream << '\n';
    }

    return stream.str();
}
