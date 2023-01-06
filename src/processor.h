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

#ifndef SRC_PROCESSOR_H_
#define SRC_PROCESSOR_H_
#include <glib.h>

#include <algorithm>
#include <cstdio>
#include <vector>
#include <list>
#include <map>
#include <string>

#include "breakpoints.h"
#include "gpsim_classes.h"
#include "gpsim_object.h"
#include "modules.h"
#include "pic-instructions.h"
#include "registers.h"
#include "gpsim_time.h"
#include "gpsim_interface.h"
#include "trigger.h"
#include "value.h"

class CPU_Freq;
class ClockPhase;
class FileContext;
class Processor;
class ProcessorConstructor;
class ProgramMemoryCollection;
class Stimulus_Node;
class TraceType;
class XrefObject;
class phaseCaptureInterrupt;
class phaseExecute1Cycle;
class phaseExecute2ndHalf;
class phaseIdle;
class phaseSkip;

//---------------------------------------------------------
/// MemoryAccess - A base class designed to support
/// access to memory. For the PIC, this class is extended by
/// the ProgramMemoryAccess and RegisterMemoryAccess classes.

class MemoryAccess :  public TriggerObject, public gpsimObject
{
public:
    explicit MemoryAccess(Processor *new_cpu);
    ~MemoryAccess();

    virtual Processor *get_cpu();

    std::list<Register *> SpecialRegisters;

protected:
    Processor *cpu;             /// The processor to which this object belongs
};


//---------------------------------------------------------
/// The ProgramMemoryAccess class is the interface used
/// by objects other than the simulator to manipulate the
/// pic's program memory. For example, the breakpoint class
/// modifies program memory when break points are set or
/// cleared. The modification goes through here.

class ProgramMemoryAccess :  public MemoryAccess
{
public:
    /// Symbolic debugging
    enum HLL_MODES
    {
        ASM_MODE,      // Source came from plain old .asm files
        HLL_MODE       // Source came from a high level language like C or JAL.
    };

    explicit ProgramMemoryAccess(Processor *new_cpu);
    ProgramMemoryAccess(const ProgramMemoryAccess &) = delete;
    ProgramMemoryAccess& operator = (const ProgramMemoryAccess &) = delete;
    ~ProgramMemoryAccess();

    virtual void putToAddress(unsigned int addr, instruction *new_instruction);
    virtual void putToIndex(unsigned int uIndex, instruction *new_instruction);
    instruction *getFromAddress(unsigned int addr);
    instruction *getFromIndex(unsigned int uIndex);
    instruction *get_base_instruction(unsigned int addr);
    unsigned int get_opcode(unsigned int addr);
    unsigned int get_rom(unsigned int addr);
    void put_rom(unsigned int addr, unsigned int value);
    char *get_opcode_name(unsigned int addr, char *buffer, unsigned int size);
    virtual unsigned int get_PC();
    virtual void set_PC(unsigned int);
    virtual Program_Counter *GetProgramCounter();

    void remove(unsigned int address, instruction *bp_instruction);

    void put_opcode(unsigned int addr, unsigned int new_opcode);
    // When a pic is replacing one of it's own instructions, this routine
    // is called.
    void put_opcode_start(unsigned int addr, unsigned int new_opcode);

    // Assign a cross reference object to an instruction
    void assign_xref(unsigned int address, XrefObject * cross_reference);

    void callback() override;
    void init(Processor *);
    // Helper functions for querying the program memory

    // hasValid_opcode -- returns true if the opcode at the address is valid
    bool hasValid_opcode_at_address(unsigned int address);
    bool hasValid_opcode_at_index(unsigned int uIndex);

    // step - step one of more instructions
    virtual void step(unsigned int steps, bool refresh = true);
    virtual void step_over(bool refresh = true);

    virtual void run(bool refresh = true);
    virtual void stop();
    virtual void finish();

    // isModified -- returns true if the program at the address has been modified
    // (this is only valid for those processor capable of writing to their own
    // program memory)
    bool isModified(unsigned int address);


    // Given a file and a line in that file, find the instrucion in the
    // processor's memory that's closest to it.
    virtual int  find_closest_address_to_line(int file_id, int src_line);
    virtual int  find_address_from_line(FileContext *fc, int src_line);
    //virtual int  find_closest_address_to_hll_line(int file_id, int src_line);

    // Given an address to an instruction, find the source line that
    // created it:

    int get_src_line(unsigned int address);

    // Return the file ID of the source program responsible for the opcode at address.
    int get_file_id(unsigned int address);

    // A couple of functions for manipulating  breakpoints
    virtual unsigned int  set_break_at_address(unsigned int address);
    virtual unsigned int  set_notify_at_address(unsigned int address,
            TriggerObject *cb);
    virtual unsigned int  set_profile_start_at_address(unsigned int address,
            TriggerObject *cb);
    virtual unsigned int  set_profile_stop_at_address(unsigned int address,
            TriggerObject *cb);
    virtual int clear_break_at_address(unsigned int address,
                                       enum instruction::INSTRUCTION_TYPES type);
    virtual int clear_break_at_address(unsigned int address,
                                       instruction * pInstruction);
    virtual int clear_notify_at_address(unsigned int address);
    virtual int clear_profile_start_at_address(unsigned int address);
    virtual int clear_profile_stop_at_address(unsigned int address);
    virtual int address_has_break(unsigned int address,
                                  enum instruction::INSTRUCTION_TYPES type = instruction::BREAKPOINT_INSTRUCTION);
    virtual int address_has_notify(unsigned int address);
    virtual int address_has_profile_start(unsigned int address);
    virtual int address_has_profile_stop(unsigned int address);
    virtual instruction *find_instruction(unsigned int address,
                                          enum instruction::INSTRUCTION_TYPES type);
    virtual void toggle_break_at_address(unsigned int address);
    virtual void set_break_at_line(unsigned int file_id, unsigned int src_line);
    virtual void clear_break_at_line(unsigned int file_id,
                                     unsigned int src_line);
    virtual void toggle_break_at_line(unsigned int file_id,
                                      unsigned int src_line);

    void set_hll_mode(unsigned int);
    enum HLL_MODES get_hll_mode()
    {
        return hll_mode;
    }
    bool isHLLmode()
    {
        return get_hll_mode() == HLL_MODE;
    }

private:
    ProgramMemoryCollection *m_pRomCollection;
    unsigned int _address;
    unsigned int _opcode;
    unsigned int _state;

    enum HLL_MODES hll_mode;

    // breakpoint instruction pointer. This is used by get_base_instruction().
    // If an instruction has a breakpoint set on it, then get_base_instruction
    // will return a pointer to the instruction and will initialize bpi to
    // the breakpoint instruction that has replaced the one in the processor's
    // program memory.
    Breakpoint_Instruction *bpi;
};


//---------------------------------------------------------
/// The RegisterMemoryAccess class is the interface used
/// by objects other than the simulator to manipulate the
/// cpu's register memory.

class RegisterMemoryAccess : public MemoryAccess
{
public:
    explicit RegisterMemoryAccess(Processor *pCpu);
    virtual ~RegisterMemoryAccess();

    virtual Register *get_register(unsigned int address);
    unsigned int get_size()
    {
        return nRegisters;
    }
    void set_Registers(Register **_registers, int _nRegisters);

    // The insertRegister and removeRegister methods are used primarily
    // to set and clear breakpoints.
    bool insertRegister(unsigned int address, Register *);
    bool removeRegister(unsigned int address, Register *);
    bool hasBreak(unsigned int address);
    void reset(RESET_TYPE r);

    Register &operator [](unsigned int address);

private:
    unsigned int nRegisters;
    Register **registers;       // Pointer to the array of registers.
};


//------------------------------------------------------------------------
//
/// FileContext - Maintain state information about files.
/// The state of each source file for a processor is recorded in the
/// FileContext class. Clients can query information like the name
/// of the source file or the line number responsible for generating
/// a specific instruction.

class FileContext
{
private:
    std::string name_str;         // File name
    FILE *fptr = nullptr;         // File ptr when the file is opened
    std::vector<int> line_seek;   // A vector of file offsets to the start of lines
    std::vector<int> pm_address;  // A vector of program memory addresses for lines
    unsigned int m_uiMaxLine = 0; // number of lines in the file

    friend class FileContextList;

protected:
    bool m_bIsList = false;          // True if this is a list file.
    bool m_bIsHLL = false;           // True if this is a HLL file.

    void setListId(bool b)
    {
        m_bIsList = b;
    }
    void setHLLId(bool b)
    {
        m_bIsHLL = b;
    }

public:
    explicit FileContext(const std::string &new_name);
    ~FileContext();

    void ReadSource();
    char *ReadLine(unsigned int line_number, char *buf, unsigned int nBytes);
    char *gets(char *buf, unsigned int nBytes);
    void rewind();
    void open(const char *mode);
    void close();
    bool IsOpen()
    {
        return fptr != nullptr;
    }
    bool IsList()
    {
        return m_bIsList;
    }
    bool IsHLL()
    {
        return m_bIsHLL;
    }

    /// get_address - given a line number, return the program memory address
    int get_address(unsigned int line);
    /// put_address - associate a line number with a program memory address.
    void put_address(unsigned int line, unsigned int address);

    std::string &name()
    {
        return name_str;
    }
    unsigned int max_line();
};


//------------------------------------------------------------------------
//
// FileContextList - a vector of FileContext objects.
//
//
class FileContextList
{
public:
    FileContextList();
    ~FileContextList();

    int Add(const std::string& new_name, bool hll = false);
    int Add(const char *new_name, bool hll = false);

    int Find(const std::string &fname);

    FileContext *operator [](int file_number);

    void list_id(int new_list_id);
    int list_id()
    {
        return list_file_id;
    }

    int nsrc_files()
    {
        return (int) file_context_list.size();
    }

    char *ReadLine(int file_id, int line_number, char *buf, int nBytes);
    char *gets(int file_id, char *buf, int nBytes);
    void rewind(int file_id);
    void SetSourcePath(const char *pPath);

private:
    std::vector<FileContext> file_context_list;
    std::string sSourcePath;
    int lastFile;
    int list_file_id;
};


class CPU_Vdd : public Float
{
public:
    CPU_Vdd(Processor * _cpu, double freq); //const char *_name, double newValue, const char *desc);

    void set(double d) override;

private:
    Processor * cpu;
};


//------------------------------------------------------------------------
//
/// Processor - a generic base class for processors supported by gpsim

class Processor : public Module
{
public:
    typedef bool (*LPFNISPROGRAMFILE)(const char *, FILE *);

    /// Load the source code for this processor. The pProcessorName
    /// is an optional name that a user can assign to the processor.
    virtual bool LoadProgramFile(const char *hex_file,
                                 FILE *pFile,
                                 const char *pProcessorName) = 0;
    /// The source files for this processor.
    FileContextList files;

    /// Oscillator cycles for 1 instruction
    unsigned int clocks_per_inst;

    /// Supply voltage
    // double Vdd;

    /// Stimulus nodes for CVREF and V06REF
    Stimulus_Node         *CVREF = nullptr;
    Stimulus_Node         *V06REF = nullptr;

    /// Processor capabilities
    unsigned long m_Capabilities;
    enum
    {
        eSTACK                  = 0x00000001,
        eWATCHDOGTIMER          = 0x00000002,
        eBREAKONSTACKOVER       = 0x00000004,
        eBREAKONSTACKUNDER      = 0x00000009,
        eBREAKONWATCHDOGTIMER   = 0x00000010,
    };
    unsigned long GetCapabilities();

    /// Processor RAM

    Register **registers;
    RegisterCollection *m_UiAccessOfRegisters = nullptr; // should this be in rma class?

    /// Currently selected RAM bank
    Register **register_bank = nullptr;

    /// Program memory - where instructions are stored.

    instruction **program_memory = nullptr;

    /// Program memory interface
    ProgramMemoryAccess  *pma;
    virtual ProgramMemoryAccess * createProgramMemoryAccess(Processor *processor);
    virtual void                  destroyProgramMemoryAccess(ProgramMemoryAccess *pma);
    virtual instruction *         ConstructInvalidInstruction(Processor *processor,
            unsigned int address, unsigned int new_opcode)
    {
        return new invalid_instruction(processor, address, new_opcode);
    }
    /// register memory interface
    RegisterMemoryAccess rma;

    /// eeprom memory interface (if present).
    RegisterMemoryAccess ema;
    unsigned int m_uPageMask;
    unsigned int m_uAddrMask;

    /// Program Counter
    Program_Counter *pc;

    /// Context debugging is a way of debugging the processor while it is
    /// in different states. For example, when the interrupt flag is set
    /// (for those processors that support interrupts), the processor is
    /// in a different 'state' then when the interrupt flag is cleared.

    std::list<ProgramMemoryAccess *> pma_context;

    /// Tracing
    /// The readTT and writeTT are TraceType objects for tracing
    /// register reads and writes.
    /// The mTrace map is a collection of special trace types that
    /// share the same trace function code. For example, interrupts
    /// and resets are special trace events that don't warrant thier
    /// own trace function code.
    TraceType *readTT, *writeTT;
    std::map <unsigned int, TraceType *> mTrace;

    // Processor's 'bad_instruction' object
    invalid_instruction bad_instruction;

    // --- TSD removed 01JAN07 These don't appear to be used anywhere
    //virtual void set(const char *cP,int len=0);
    //virtual void get(char *, int len);

    //
    // Creation and manipulation of registers
    //

    void create_invalid_registers();
    void delete_invalid_registers();
    void add_file_registers(unsigned int start_address,
                            unsigned int end_address,
                            unsigned int alias_offset);
    void delete_file_registers(unsigned int start_address,
                               unsigned int end_address, bool bRemoveWithoutDelete = false);
    void alias_file_registers(unsigned int start_address,
                              unsigned int end_address,
                              unsigned int alias_offset);
    virtual int  map_rm_address2index(int address) { return address; }
    virtual int  map_rm_index2address(int index) { return index; }
    virtual void init_register_memory(unsigned int memory_size);
    virtual unsigned int register_memory_size() const = 0;
    virtual unsigned int CalcJumpAbsoluteAddress(unsigned int /* uInstAddr */,
            unsigned int uDestAddr)
    {
        return uDestAddr;
    }
    virtual unsigned int CalcCallAbsoluteAddress(unsigned int /* uInstAddr */,
            unsigned int uDestAddr)
    {
        return uDestAddr;
    }

    //
    // Creation and manipulation of Program Memory
    //

    virtual void init_program_memory(unsigned int memory_size);
    virtual void init_program_memory(unsigned int address, unsigned int value);
    virtual void erase_program_memory(unsigned int address);
    virtual void init_program_memory_at_index(unsigned int address,
            unsigned int value);
    virtual void init_program_memory_at_index(unsigned int address,
            const unsigned char *, int nBytes);
    virtual unsigned int program_memory_size() const
    {
        return 0;
    }
    virtual unsigned int program_address_limit() const
    {
        return map_pm_index2address(program_memory_size());
    }
    virtual unsigned int get_program_memory_at_address(unsigned int address);
    void build_program_memory(unsigned int *memory,
                              unsigned int minaddr,
                              unsigned int maxaddr);

    virtual int  map_pm_address2index(int address) const
    {
        return address;
    }
    virtual int  map_pm_index2address(int index) const
    {
        return index;
    }
    virtual void set_out_of_range_pm(unsigned int address, unsigned int value);
    guint64 cycles_used(unsigned int address);
    virtual bool IsAddressInRange(unsigned int address)
    {
        return address < program_address_limit();
    }

    // opcode_size - number of bytes for an opcode.
    virtual int opcode_size() { return 2; }

    //
    // Symbolic debugging
    //
    // First the source files:

    void attach_src_line(unsigned int address,
                         unsigned int file_id,
                         unsigned int sline,
                         unsigned int lst_line);
    void read_src_files();


    virtual void dump_registers();
    virtual instruction * disasm(unsigned int address, unsigned int inst) = 0;

    virtual bool get_intedgx(int ) {fprintf(stderr, "Unexpected call to get_intedgx\n"); return true;}

    //virtual void initializeAttributes();

    //
    // Processor State
    //
    // copy the entire processor state to a file
    virtual void save_state(FILE *);
    // take an internal snap shot of the current state.
    virtual void save_state();

    // restore the processor state
    virtual void load_state(FILE *);

    //
    // Execution control
    //

    virtual void run(bool refresh = true) = 0;
    virtual void run_to_address(unsigned int destination);
    virtual void finish() = 0;

    virtual void sleep() {}
    virtual void exit_sleep()
    {
        fputs("RRR exit_sleep\n", stderr);
    }
    virtual void inattentive(unsigned int count) {}
    virtual void step(unsigned int steps, bool refresh = true) = 0;
    virtual void step_over(bool refresh = true);
    virtual void step_one(bool refresh = true) = 0;
    virtual void step_cycle() = 0;
    virtual void interrupt() = 0 ;

    // Simulation modes

    /// setWarnMode - when true, gpsim will issue warnings whenever
    /// something suspicious is occuring.
    virtual void setWarnMode(bool);
    virtual bool getWarnMode()
    {
        return bWarnMode;
    }

    /// setSafeMode - when true, gpsim will model the 'official'
    /// behavior of the chip. When false, the simulator behaves the same
    /// as the hardware.
    virtual void setSafeMode(bool);
    virtual bool getSafeMode()
    {
        return bSafeMode;
    }

    /// setUnknownMode - when true, gpsim will implement three-state logic
    /// for data. When false, unkown data are treated as zeros.
    virtual void setUnknownMode(bool);
    virtual bool getUnknownMode()
    {
        return bUnknownMode;
    }

    /// setBreakOnReset - when true, gpsim will implement three-state logic
    /// for data. When false, unkown data are treated as zeros.
    virtual void setBreakOnReset(bool);
    virtual bool getBreakOnReset()
    {
        return bBreakOnReset;
    }

    bool getBreakOnInvalidRegisterRead()
    {
        return *m_pbBreakOnInvalidRegisterRead;
    }
    bool getBreakOnInvalidRegisterWrite()
    {
        return *m_pbBreakOnInvalidRegisterWrite;
    }

    ///
    /// Notification of breakpoint set
    virtual void NotifyBreakpointSet(Breakpoints::BreakStatus & /* bs */, TriggerObject * /* bpo */) { }
    virtual void NotifyBreakpointCleared(Breakpoints::BreakStatus & /* bs */, TriggerObject * /* bpo */) { }

    // Tracing control

    virtual void trace_dump(int type, int amount);
    virtual int trace_dump1(int type, char *buffer, int bufsize);
    virtual RegisterValue getWriteTT(unsigned int addr);
    virtual RegisterValue getReadTT(unsigned int addr);

    //
    // Processor Clock control
    //

    void set_frequency(double f);
    void set_frequency_rc(double f);
    void set_RCfreq_active(bool);
    virtual double get_frequency();

    void set_ClockCycles_per_Instruction(unsigned int cpi)
    {
        clocks_per_inst = cpi;
    }
    unsigned int get_ClockCycles_per_Instruction()
    {
        return clocks_per_inst;
    }

    void update_cps();

    virtual double get_OSCperiod();

    virtual double get_InstPeriod()
    {
        return get_OSCperiod() * get_ClockCycles_per_Instruction();
    }

    virtual void disassemble(signed int start_address,
                             signed int end_address);
    virtual void list(unsigned int file_id,
                      unsigned int pcval,
                      int start_line,
                      int end_line);

    // Configuration control

    virtual bool set_config_word(unsigned int /* address */, unsigned int /* cfg_word */)
    {
        return false; // fixme - make this a pure virtual function...
    }
    virtual unsigned int get_config_word(unsigned int address) = 0;
    virtual unsigned int config_word_address()
    {
        return 0;
    }
    virtual int get_config_index(unsigned int /* address */)
    {
        return -1;
    }

    //
    // Processor reset
    //

    virtual void reset(RESET_TYPE r) override = 0;

    double get_Vdd() override
    {
        return m_vdd->getVal();
    }
    void set_Vdd(double v) override
    {
        m_vdd->set(v);
    }
    virtual void update_vdd();

    //
    // Debugging - used to view the state of the processor (or whatever).
    //

    virtual void Debug();

    //
    // FIXME -- create -- a way of constructing a processor (why not use constructors?)
    //

    virtual void create();
    static Processor *construct();
    ProcessorConstructor  *m_pConstructorObject;

    Processor(const char *_name = nullptr, const char *desc = nullptr);
    virtual ~Processor();

    CPU_Vdd   *m_vdd;
    phaseExecute1Cycle	*mExecute1Cycle = nullptr;
    ClockPhase 		*mCurrentPhase = nullptr;
    phaseExecute2ndHalf 	*mExecute2ndHalf = nullptr;     // misnomer - should be 2-cycle
    phaseCaptureInterrupt	*mCaptureInterrupt = nullptr;
    phaseIdle 		*mIdle = nullptr;
    phaseSkip		*mSkip = nullptr;

private:
    CPU_Freq *mFrequency;
    unsigned int  m_ProgramMemoryAllocationSize;

    // Simulation modes
    bool bSafeMode;
    bool bWarnMode;
    bool bUnknownMode;
    bool bBreakOnReset;
    Boolean *m_pbBreakOnInvalidRegisterRead;
    Boolean *m_pbBreakOnInvalidRegisterWrite;
    Boolean *m_pWarnMode;
    Boolean *m_pSafeMode;
    Boolean *m_pUnknownMode;
    Boolean *m_pBreakOnReset;
};


//-------------------------------------------------------------------
//
// ProcessorConstructor -- a class to handle all of gpsim's supported
// processors
//
// gpsim supports dozens of processors. All of these processors are
// grouped together in the ProcessConstructor class. Within the class
// is a static STL list<> object that holds an instance of a
// ProcessorConstructor for each gpsim supported processor. Whenever
// the user selects a processor to simulate, the find() member
// function will search through the list and find the one that matches
// the user supplied ASCII string.
//
// Why have this class?
// The idea behind this class is that a ProcessorConstructor object
// can be instantiated for each processor and that instantiation will
// place the object into list of processors. Prior to gpsim-0.21, a
// giant array held the list of all available processors. However,
// there were two problems with this: it was painful to look at and
// it precluded processors that were defined outside of the gpsim
// core library.

// THE list of all of gpsim's processors:
using ProcessorConstructorList = std::list <ProcessorConstructor *>;

class ProcessorConstructor
{
public:
    typedef Processor * (*tCpuContructor)(const char *_name);

protected:
    // A pointer to a function that when called will construct a processor
    tCpuContructor cpu_constructor;

public:
    virtual Processor * ConstructProcessor(const char *opt_name = nullptr);

    // The processor name (plus upto three aliases).
#define nProcessorNames 4
    const char *names[nProcessorNames];

    //------------------------------------------------------------
    // contructor --
    //
    ProcessorConstructor(
        tCpuContructor    _cpu_constructor,
        const char *name1,
        const char *name2,
        const char *name3 = nullptr,
        const char *name4 = nullptr);

    virtual ~ProcessorConstructor()
    {
    }


    static ProcessorConstructorList * GetList();
    static ProcessorConstructor * findByType(const char *type);
    static std::string listDisplayString();

private:
    static ProcessorConstructorList * processor_list;
};



//----------------------------------------------------------
// Global definitions:

#if defined(IN_MODULE) && defined(_WIN32)
// we are in a module: don't access active_cpu object directly!
LIBGPSIM_EXPORT Processor * get_active_cpu();
#else
// we are in gpsim: use of get_active_cpu() and set_active_cpu() is recommended,
// even if active_cpu object can be accessed directly.
extern Processor *active_cpu;

inline Processor *get_active_cpu()
{
    return active_cpu;
}

inline void set_active_cpu(Processor *act_cpu)
{
    active_cpu = act_cpu;
}
#endif


#endif
