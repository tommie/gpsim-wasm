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

#include <assert.h>
#include <iostream>
#include <map>
#include <utility>

#include "../config.h"
#include "breakpoints.h"
#include "gpsim_time.h"
#include "trace.h"
#include "modules.h"
#include "pic-instructions.h"
#include "processor.h"
#include "trace_orb.h"
#include "xref.h"

extern "C"
{
#include "lxt_write.h"
}

#define NEWTRACE

//#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("%s:%d-%s() ",__FILE__,__LINE__,__FUNCTION__); printf arg; }
#else
#define Dprintf(arg) {}
#endif


Trace trace;               /* Instantiate the trace buffer class.
                            * This is where *everything* including the
                            * kitchen sink gets stored in a trace buffer.
                            * Since everything is stored here, it gets
                            * rather difficult to post process traced info
                            * efficiently. So this buffer is primarily used
                            * to record program flow that the user may post
                            * analyze by dumping its contents.
                            */

// create an instance of inline get_trace() method by taking its address
Trace &(*dummy_trace)() = get_trace;

TraceLog trace_log;
ProfileKeeper profile_keeper;


#if defined(_WIN32)
TraceLog &GetTraceLog()
{
  return trace_log;
}
#endif


//========================================================================
traceValue::traceValue()
{
}


unsigned int traceValue::get_value()
{
  return trace.trace_index;
}


/****************************************************************************
 *
 *   gpsim Trace
 *

   General:

   gpsim traces almost everything simulated: instructions executed,
   register reads/writes, clock cycles, special register accesses
   break points, instruction skips, external modules, and a few
   other miscellaneous things.

   The tracing subsystem is implemented as a C++ class. In theory,
   multiple traces could be instantiated, but (currently) there is
   one global trace instantiated and all the pieces of gpsim make
   direct references to it.

   How can gpsim trace every thing and still be the fastest
   microcontroller simulator? Well, gpsim writes trace
   information into a giant circular buffer. So one optimization
   is that there are no array bounds to check. Another optimization
   is that most of the trace operations are C++ inline functions.
   A third optimization is that the trace operations are efficiently
   encoded into 32-bit words. The one exception is the cycle counter
   trace, it takes two 32-bit words (see the cycle counter trace
   comment below).

   The upper 8-bits of the trace word are reserved for describing
   the trace type. The upper two bits of these 8-bits are reservered
   for encoding the cycle counter. The lower 6-bits allow 64 enumerated
   types to be encoded. Only a small portion of these are currently
   used. The lower 24-bits of the 32-bit trace word store the
   information we wish to trace. For example, for register reads and
   writes, there are 8-bits of data and (upto) 16-bits of address.


   Details

   Each trace takes exactly one 32-bit word. The upper 8-bits define
   the trace type and the lower 24 are the trace value. For example,
   a register write will get traced with a 32 bit encoded like:

      TTAAAAVV

   TT - Register write trace type
   AAAA - 4-hexdigit address
   VV - 2-hexdigit (8-bit) value

   The cycle counter is treated slightly differently. Since it is a
   64-bit object, it has to be split across at least two trace
   entries. The upper few bits of the cycle counter aren't
   traced. (This is a bug for simulations that run for several
   centuries!) The trace member function is_cycle_trace() describes
   the cycle counter encoding in detail.

   Trace Types.

   gpsim differentiates individually traced items by the TT field
   (upper 8 bits of the 32-bit trace word). Except for the cycle
   counter trace, these trace types are dynamically allocated whenever
   a TraceType class is instantiated. As described above, the other
   gpsim classes use this dynamically allocated 32-bit trace word as a
   handle for efficiently storing information into the trace
   buffer. In addition to allocating the 32-bit word for tracing, the
   TraceType class will use it as a hash index into the
   'trace_map'. The trace_map is a locally (to trace.cc) instantiated
   STL map that cross references the 32-bit trace types to the
   instantiated TraceType class that create them.

   When the trace buffer is decoded, the trace type (upper 8-bits) of
   the 32-bit trace word is extracted and used to look up the
   TraceType object in the trace_map map. The lower 24-bits of the
   trace word are then passed to the TraceType object's decode()
   method. Continuing with the example from above,

       TTAAAAVV

      TT - Used to look up the TraceType object in the trace_map
      AAAAVV - Passed to the decode() method of the object.

   The TraceType decode() method will usually create a TraceObject and
   place it on a TraceFrame.


   Trace Frames and Trace Objects

   A trace frame is defined to be the decoded contents of the trace
   buffer corresponding to a single time quantum (i.e. single
   simulation cycle). Each frame has an STL list to hold the
   TraceObjects. The TraceObjects are created by the TraceType
   classes. This happens when the 32-bit trace word is decoded by the
   TraceType class. The TraceObject has several purposes. First,
   unique TraceObjects are created for the variety of things gpsim
   traces. For example, when the simulated processor writes to a
   register, a corresponding RegisterWriteTraceObject will get created
   when the trace buffer is decoded. Another purpose of the
   TraceObject is to record the system state. Take for example the
   register write trace. When a register write occurs, the register
   has a value before the write and a value after the write. When the
   register write is traced, only the value *before* the write is
   recorded. When the trace buffer is decoded, the simulation is
   effectively run backwards. The current state is known before trace
   decoding commences. Then as the decoding steps backwards through
   the trace history, the state change at each trace event is recorded
   in the trace object. So the register write trace event gets decoded
   into a TraceObject. This trace object knows the current state of
   the register; that's simply the register's current contents. The
   trace object knows the contents of the register prior to the
   register write operation; that's stored in the trace buffer.


****************************************************************************/


/*
  Trace Logging

  gpsim supports two modes of trace logging. The first mode is the
  "log all" mode. In this mode, the entire trace buffer is periodically
  written to a file. The second mode is a "log selective". In this mode
  individual trace operations are written to a file.

*/


TraceRawLog::TraceRawLog()
{
}


TraceRawLog::~TraceRawLog()
{
  if (log_file) {
    log();
    fclose(log_file);
  }
}


void TraceRawLog::log()
{
  if (log_file) {
    for (unsigned int i = 0; i < trace.trace_index; i++) {
      fprintf(log_file, "%08X\n", trace[i]);
    }

    trace.trace_index = 0;
  }
}


void TraceRawLog::enable(const char *fname)
{
  if (!fname) {
    std::cout << "Trace logging - invalid file name\n";
    return;
  }

  log_filename = fname;
  log_file = fopen(fname, "w");

  if (log_file) {
    trace.bLogging = true;
    std::cout << "Trace logging enabled to file " << fname << '\n';

  } else {
    std::cout << "Trace logging: could not open: " << fname << '\n';
  }
}


void TraceRawLog::disable()
{
  log();

  if (trace.cpu) {
    trace.cpu->save_state(log_file);
  }

  log_filename.clear();

  if (log_file) {
    fclose(log_file);
  }

  log_file = nullptr;
  std::cout << "Trace logging disabled\n";
  trace.bLogging = false;
}


//========================================================================
// TraceFrame
//
// A TraceFrame is a collection of traced items that belong to a single
// simulation cycle. The TraceFrame is only built up whenever the user
// requests trace history. Each frame contains a list of traceObjects
// that describe the specific information that the simulation has traced.

TraceFrame::TraceFrame()
{
}


TraceFrame::~TraceFrame()
{
  for (auto & to : traceObjects) {
    delete to;
  }
}


void TraceFrame::add(TraceObject *to)
{
  traceObjects.push_back(to);
}


void TraceFrame::print(FILE *fp)
{
  for (auto && to : traceObjects) {
    to->print_frame(this, fp);
  }
}


void TraceFrame::update_state()
{
  for (auto && to : traceObjects) {
    to->getState(this);
  }
}


//============================================================
// Trace::addFrame
//
// The Trace class maintains a list of traceFrames. Here is where
// a new one gets added. Note that traceFrames are created only
// when a user requests to see the trace history.

void Trace::addFrame(TraceFrame *newFrame)
{
  current_frame = newFrame;
  traceFrames.push_back(newFrame);
}


void Trace::addToCurrentFrame(TraceObject *to)
{
  if (current_frame) {
    current_frame->add(to);

  } else {
    delete to;
  }
}


void Trace::deleteTraceFrame()
{
  if (!current_frame) {
    return;
  }

  for (auto & tf : traceFrames) {
    delete tf;
  }

  traceFrames.clear();
  current_frame = nullptr;
  current_cycle_time = 0;
}


void Trace::printTraceFrame(FILE *fp)
{
  std::list<TraceFrame *>::reverse_iterator tfIter = traceFrames.rbegin();

  for ( ; tfIter != traceFrames.rend(); ++tfIter) {
    (*tfIter)->print(fp);
  }
}


//========================================================================
// TraceObject
//
// A TraceObject is a base class for decoded traces. TraceObjects are only
// created when the user requests to see the TraceHistory.
//
TraceObject::TraceObject()
{
}


void TraceObject::print_frame(TraceFrame *, FILE *)
{
  // by default, a trace object doesn't know how to print a frame
  // special trace objects derived from this class will be designated
  // printers.
}


void TraceObject::getState(TraceFrame *)
{
  // Provide an opportunity for derived classes to copy specific state
  // information to the TraceFrame.
}


//========================================================================
// InvalidTraceObject
//
InvalidTraceObject::InvalidTraceObject(int type)
  : mType(type)
{
}


void InvalidTraceObject::print(FILE *fp)
{
  fprintf(fp, "  Invalid Trace entry: 0x%x\n", mType);
}


//========================================================================
// ModuleTraceObject

void ModuleTraceObject::print(FILE *fp)
{
  fprintf(fp, " Module Trace: ");

  if (pModule) {
    fprintf(fp, "%s ", pModule->name().c_str());
  }

  if (pModuleTraceType && pModuleTraceType->cpDescription()) {
    fprintf(fp, "%s ", pModuleTraceType->cpDescription());
  }

  fprintf(fp, "0x%x\n", mTracedData & 0xffffff);
}


//========================================================================
// RegisterTraceObject
//
RegisterWriteTraceObject::RegisterWriteTraceObject(Processor *_cpu,
    Register *_reg,
    RegisterValue trv)
  : ProcessorTraceObject(_cpu), reg(_reg), from(trv)
{
  if (reg) {
    to = reg->get_trace_state();
    reg->put_trace_state(from);
  }
}


void RegisterWriteTraceObject::getState(TraceFrame * )
{
}


void RegisterWriteTraceObject::print(FILE *fp)
{
  if (reg) {
    char sFrom[16];
    char sTo[16];

    fprintf(fp, "  Wrote: 0x%s to %s(0x%04X) was 0x%s\n",
            to.toString(sTo, sizeof(sTo)),
            reg->name().c_str(), reg->address,
            from.toString(sFrom, sizeof(sFrom)));
  }
}


RegisterReadTraceObject::RegisterReadTraceObject(Processor *_cpu,
    Register *_reg,
    RegisterValue trv)
  : RegisterWriteTraceObject(_cpu, _reg, trv)
{
  if (reg) {
    reg->put_trace_state(from);
  }
}


void RegisterReadTraceObject::print(FILE *fp)
{
  if (reg) {
    char sFrom[16];

    fprintf(fp, "  Read: 0x%s from %s(0x%04X)\n",
            from.toString(sFrom, sizeof(sFrom)), reg->name().c_str(), reg->address);
  }
}


void RegisterReadTraceObject::getState(TraceFrame * )
{
}


//========================================================================
PCTraceObject::PCTraceObject(Processor *_cpu, unsigned int _address)
  : ProcessorTraceObject(_cpu), address(_address)
{
}


void PCTraceObject::print(FILE *fp)
{
  char a_string[200];
  unsigned addr = cpu->map_pm_index2address(address & 0xffff);
  fprintf(fp, "0x%04X 0x%04X %s\n",
          addr,
          (cpu->pma->getFromAddress(addr))->get_opcode(),
          (cpu->pma->getFromAddress(addr))->name(a_string, sizeof(a_string)));
  instruction * pInstr = cpu->pma->getFromAddress(addr);
  int srcLine = pInstr->get_src_line();

  if (srcLine >= 0)
    fprintf(fp, "%d: %s",
            srcLine,
            cpu->files.ReadLine(pInstr->get_file_id(),
                                pInstr->get_src_line(),
                                a_string, sizeof(a_string)));
}


void PCTraceObject::print_frame(TraceFrame *tf, FILE *fp)
{
  if (!tf) {
    return;
  }

  std::list<TraceObject *>::reverse_iterator toIter;
  fprintf(fp, "0x%016" PRINTF_GINT64_MODIFIER "X %s ",
          tf->cycle_time, cpu->name().c_str());
  print(fp);

  for (toIter = tf->traceObjects.rbegin();
       toIter != tf->traceObjects.rend();
       ++toIter)
    if (*toIter != this) {
      (*toIter)->print(fp);
    }
}


//========================================================================
// Trace Type for Resets
//------------------------------------------------------------------------
const char * resetName(RESET_TYPE r)
{
  switch (r) {
  case POR_RESET:
    return "POR_RESET";

  case WDT_RESET:
    return "WDT_RESET";

  case IO_RESET:
    return "IO_RESET";

  case MCLR_RESET:
    return "MCLR_RESET";

  case SOFT_RESET:
    return "SOFT_RESET";

  case BOD_RESET:
    return "BOD_RESET";

  case SIM_RESET:
    return "SIM_RESET";

  case EXIT_RESET:
    return "EXIT_RESET";

  case STKUNF_RESET:
    return "STKUNF_RESET";

  case STKOVF_RESET:
    return "STKOVF_RESET";

  case OTHER_RESET:
    return "OTHER_RESET";

  case WDTWV_RESET:
    return "WDTWV_RESET";
  }

  return "unknown reset";
}


//------------------------------------------------------------
ResetTraceObject::ResetTraceObject(Processor *_cpu, RESET_TYPE r)
  : ProcessorTraceObject(_cpu), m_reset(r)
{
}


void ResetTraceObject::print(FILE *fp)
{
  fprintf(fp, "  Reset: %s\n", resetName(m_reset));
}


//========================================================================

TraceType::TraceType(unsigned int nTraceEntries, const char *desc)
  : mSize(nTraceEntries), mpDescription(desc)
{
}


void TraceType::showInfo()
{
  std::cout << cpDescription();
  std::cout << "\n  Type: 0x" << std::hex << mType
       << "\n  Size: " << mSize << '\n';
}


const char *TraceType::cpDescription()
{
  return mpDescription ? mpDescription : "No Description";
}


//----------------------------------------
//
// isValid
//
// If the trace record starting at the trace buffer index 'tbi' is of the
// same type as this trace object, then return true.
//
bool TraceType::isValid(Trace *pTrace, unsigned int tbi)
{
  if (!pTrace) {
    return false;
  }

  unsigned int i;

  // The upper 8-bits of the 'type' specify the trace type for this object.
  // This is assigned whenever Trace::allocateTraceType() is called. Multi-
  // sized trace records occupy consecutive types.
  for (i = 0; i < size(); i++) {
    //if(pTrace->type(tbi + i) != (type() + (i<<24)))
    if (!isValid(pTrace->get(tbi + i))) {
      return false;
    }
  }

  return true;
}


int TraceType::dump_raw(Trace *pTrace, unsigned int tbi, char *buf, int bufsize)
{
  if (!pTrace || !buf) {
    return 0;
  }

  int total_chars = 0;
  int iUsed = entriesUsed(pTrace, tbi);

  for (int i = 0; i < iUsed; i++) {
    int n = snprintf(buf, bufsize, " %08X:", pTrace->get(tbi + i));

    if (n < 0) {
      break;
    }

    total_chars += n;
    buf += n;
    bufsize -= n;
  }

  return total_chars;
}


//============================================================
//
// entriesUsed
//
// given a trace buffer and an index into it, return the number
// of trace buffer entries at that point that match the type of
// this trace.

int TraceType::entriesUsed(Trace *pTrace, unsigned int tbi)
{
  int iUsed = 0;

  if (pTrace)
    while (pTrace->type(tbi + iUsed) == (mType + (iUsed << 24))) {
      iUsed++;
    }

  return iUsed;
}


//========================================================================
ModuleTraceType::ModuleTraceType(Module *_pModule,
                                 unsigned int nTraceEntries,
                                 const char *desc)
  : TraceType(nTraceEntries, desc), pModule(_pModule)
{
}


TraceObject *ModuleTraceType::decode(unsigned int tbi)
{
  ModuleTraceObject *mto = new ModuleTraceObject(pModule, this, trace.get(tbi) & 0xffffff);
  return mto;
}


int ModuleTraceType::dump_raw(Trace *pTrace, unsigned int tbi, char *buf, int bufsize)
{
  if (!pTrace) {
    return 0;
  }

  int n = TraceType::dump_raw(pTrace, tbi, buf, bufsize);
  buf += n;
  bufsize -= n;
  unsigned int tv = pTrace->get(tbi);
  int m = snprintf(buf, bufsize,
                   " Module: %s 0x%x",
                   (pModule ? pModule->name().c_str() : "no name"),
                   (tv & 0xffffff));
  return m > 0 ? (m + n) : n;
}


//========================================================================
CycleTraceType::CycleTraceType(unsigned int s)
  : TraceType(s, "Cycle")
{
}


TraceObject *CycleTraceType::decode(unsigned int /* tbi */ )
{
  return nullptr;
}


bool CycleTraceType::isFrameBoundary()
{
  return false;
}


int CycleTraceType::dump_raw(Trace *pTrace, unsigned tbi, char *buf, int bufsize)
{
  int n = TraceType::dump_raw(pTrace, tbi, buf, bufsize);
  buf += n;
  bufsize -= n;
  int m = 0;

  if (pTrace) {
    uint64_t cycle;

    if (pTrace->is_cycle_trace(tbi, &cycle) == 2) {
      m = snprintf(buf, bufsize, "  Cycle 0x%016" PRINTF_GINT64_MODIFIER "X", cycle);
    }
  }

  return m > 0 ? (m + n) : n;
}


int CycleTraceType::entriesUsed(Trace *pTrace, unsigned int tbi)
{
  return pTrace ? pTrace->is_cycle_trace(tbi, 0) : 0;
}


//========================================================================
ProcessorTraceType::ProcessorTraceType(Processor *_cpu,
                                       unsigned int nTraceEntries,
                                       const char *pDesc)
  : TraceType(nTraceEntries, pDesc), cpu(_cpu)
{
}


//========================================================================

RegisterWriteTraceType::RegisterWriteTraceType(Processor *_cpu,
    unsigned int s)
  : ProcessorTraceType(_cpu, s, "Reg Write")
{
}


TraceObject *RegisterWriteTraceType::decode(unsigned int tbi)
{
  unsigned int tv = trace.get(tbi);
  RegisterValue rv = RegisterValue(tv & 0xff, 0);
  unsigned int address = (tv >> 8) & 0xfff;
  RegisterWriteTraceObject *rto = new RegisterWriteTraceObject(cpu, cpu->rma.get_register(address), rv);
  return rto;
}


int RegisterWriteTraceType::dump_raw(Trace *pTrace, unsigned int tbi, char *buf, int bufsize)
{
  unsigned int val = 0;

  if (!pTrace) {
    return 0;
  }

  int n = TraceType::dump_raw(pTrace, tbi, buf, bufsize);
  buf += n;
  bufsize -= n;
  unsigned int tv = pTrace->get(tbi);
  unsigned int address = (tv >> 8) & 0xfff;
  Register *reg = cpu->rma.get_register(address);

  if (reg) {
    val = reg->get_value();
  }

  int m = snprintf(buf, bufsize,
                   "  Reg Write: 0x%0x to %s(0x%04X) was 0x%0X ",
                   val & 0xff,
                   (reg ? reg->name().c_str() : ""), address,
                   tv & 0xff);
  Dprintf(("dump_raw %s %x\n", buf, tv & 0xff));

  if (m > 0) {
    n += m;
  }

  return n;
}


//========================================================================

RegisterReadTraceType::RegisterReadTraceType(Processor *_cpu,
    unsigned int s)
  : ProcessorTraceType(_cpu, s, "Reg Read")
{
}


TraceObject *RegisterReadTraceType::decode(unsigned int tbi)
{
  unsigned int tv = trace.get(tbi);
  RegisterValue rv = RegisterValue(tv & 0xff, 0);
  unsigned int address = (tv >> 8) & 0xfff;
  RegisterReadTraceObject *rto = new RegisterReadTraceObject(cpu, cpu->rma.get_register(address), rv);
  return rto;
}


int RegisterReadTraceType::dump_raw(Trace *pTrace, unsigned int tbi, char *buf, int bufsize)
{
  if (!pTrace) {
    return 0;
  }

  int n = TraceType::dump_raw(pTrace, tbi, buf, bufsize);
  buf += n;
  bufsize -= n;
  unsigned int tv = pTrace->get(tbi);
  unsigned int address = (tv >> 8) & 0xfff;
  Register *reg = cpu->rma.get_register(address);
  int m = snprintf(buf, bufsize,
                   "  Reg Read:  %s(0x%04X) was 0x%0X",
                   (reg ? reg->name().c_str() : ""), address,
                   tv & 0xff);

  if (m > 0) {
    n += m;
  }

  return n;
}


//========================================================================
PCTraceType::PCTraceType(Processor *_cpu,
                         unsigned int s)
  : ProcessorTraceType(_cpu, s, "PC")
{
}


TraceObject *PCTraceType::decode(unsigned int tbi)
{
  unsigned int tv = trace.get(tbi);
  trace.addFrame(new TraceFrame());
  PCTraceObject *pcto = new PCTraceObject(cpu, tv);

  // If this was a branch (i.e. tv bits 17:16 == 01), it took two cycles
  if ((tv & (3 << 16)) == (1 << 16)) {
    trace.current_cycle_time -= 2;

  } else {
    trace.current_cycle_time -= 1;
  }

  trace.current_frame->cycle_time = trace.current_cycle_time;
  return pcto;
}


int PCTraceType::dump_raw(Trace *pTrace, unsigned int tbi, char *buf, int bufsize)
{
  if (!pTrace) {
    return 0;
  }

  int n = TraceType::dump_raw(pTrace, tbi, buf, bufsize);
  buf += n;
  bufsize -= n;
  int m = snprintf(buf, bufsize, "FRAME ==============  PC: %04X",
                   cpu->map_pm_index2address(pTrace->get(tbi) & 0xffff));

  if (m > 0) {
    n += m;
  }

  return n;
}


//------------------------------------------------------------
ResetTraceType::ResetTraceType(Processor *_cpu)
  : ProcessorTraceType(_cpu, 1, "Reset")
{
  m_uiTT = trace.allocateTraceType(this);
}


TraceObject *ResetTraceType::decode(unsigned int tbi)
{
  unsigned int tv = trace.get(tbi);
  return new ResetTraceObject(cpu, (RESET_TYPE)(tv & 0xff));
}


void ResetTraceType::record(RESET_TYPE r)
{
  trace.raw(m_uiTT | r);
}


int ResetTraceType::dump_raw(Trace *pTrace, unsigned int tbi, char *buf, int bufsize)
{
  if (!pTrace) {
    return 0;
  }

  int n = TraceType::dump_raw(pTrace, tbi, buf, bufsize);
  buf += n;
  bufsize -= n;
  RESET_TYPE r = (RESET_TYPE)(pTrace->get(tbi) & 0xff);
  int m = snprintf(buf, bufsize,
                   " %s Reset: %s",
                   (cpu ? cpu->name().c_str() : ""),
                   resetName(r));
  return m > 0 ? (m + n) : n;
}


//========================================================================

#define TRACE_ALL (0xffffffff)

//
// The trace_map is an STL map object that associates dynamically
// created trace types with a unique number. The simulation engine
// uses the number as a 'command' for tracing information of a
// specific type. This number along with information specific to
// to the trace type is written into the trace buffer. When the
// simulation is halted and the trace buffer is parsed, the
// trace type can be extracted. This can then be used as an input
// to the trace_map to access an object that can further process
// the traced information.
//
// Here's an example:
//
// The pic_processor class during construction will request a trace
// type for tracing 8-bit register writes.
//
std::map<unsigned int, TraceType *> trace_map;
CycleTraceType *pCycleTrace = nullptr;


Trace::Trace()
{
  *string_buffer = 0;
  for (trace_index = 0; trace_index < TRACE_BUFFER_SIZE; trace_index++) {
    trace_buffer[trace_index] = NOTHING;
  }

  trace_index = 0;
  xref = new XrefObject(&trace_value);
}


Trace::~Trace()
{
  delete xref;
}


//--------------------------------------------------------------
//
void Trace::showInfo()
{
  std::map<unsigned int, TraceType *>::iterator tti;

  for (unsigned int index = 0; index < 0x3f000000; index += 0x1000000) {
    tti = trace_map.find(index);

    if (tti != trace_map.end()) {
      TraceType *tt = (*tti).second;
      tt->showInfo();
    }
  }
}


//--------------------------------------------------------------
//
unsigned int Trace::type(unsigned int index)
{
  unsigned int traceType = operator[](index) & TYPE_MASK;
  unsigned int cycleType = traceType & (CYCLE_COUNTER_LO | CYCLE_COUNTER_MI);
  return cycleType ? cycleType : traceType;
}


//--------------------------------------------------------------
// is_cycle_trace(unsigned int index)
//
//  Given an index into the trace buffer, this function determines
// if the trace is a cycle counter trace.
//
// INPUT: index - index into the trace buffer
//        *cvt_cycle - a pointer to where the cycle will be decoded
//                     if the trace entry is a cycle trace.
// RETURN: 0 - trace is not a cycle counter
//         1 - trace is the middle or high integer of a cycle trace
//         2 - trace is the low integer of a cycle trace

int Trace::is_cycle_trace(unsigned int index, uint64_t *cvt_cycle)
{
  if (!(get(index) & (CYCLE_COUNTER_LO | CYCLE_COUNTER_MI))) {
    return 0;
  }

  // Cycle counter
  // A cycle counter occupies three consecutive trace buffer entries.
  // We have to determine if the current entry (pointed to by index) is
  // the high or low integer of the cycle counter.
  //
  // The upper two bits of the trace are used to decode the two 32-bit
  // integers that comprise the cycle counter. The encoding algorithm is
  // optimized for speed:
  // CYCLE_COUNTER_LO is defined as 2<<30
  // CYCLE_COUNTER_MI is defined as 1<<30
  // CYCLE_COUNTER_HI is defined as 3<<30
  //
  //   trace[i  ] = low 24 bits of cycle counter | CYCLE_COUNTER_LO
  //   trace[i+1] = middle 24 bits of "    " | CYCLE_COUNTER_MI
  //   trace[i+2] = upper  16 bits of "    " | CYCLE_COUNTER_HI
  //
  // The low 24-bits are always saved in the trace buffer with the msb (CYCLE_COUNTER_LO)
  // set.
  // Looking at the upper two bits of trace buffer, we can make these
  // observations:
  //
  // 00 - not a cycle counter trace
  // 10 - current index points at the low int of a cycle counter
  // 01 - current index points at the middle int of a cycle counter
  // 11 - current index points at the high int of a cycle counter
  int j = index;                         // Assume that the index is pointing to the low int.
  int k = (j + 1) & TRACE_BUFFER_MASK;   // and that the next entry is the middle int.
  int l = (j + 2) & TRACE_BUFFER_MASK;   // and that the next entry is the high int.

  if ((get(j) & CYCLE_COUNTER_LO) &&
      (get(k) & CYCLE_COUNTER_MI) &&
      (get(l) & CYCLE_COUNTER_HI)) {
    // extract the ~64bit cycle counter from the trace buffer.
    if (cvt_cycle) {
      *cvt_cycle = get(l) & 0xffff;
      *cvt_cycle = (*cvt_cycle << 16) | (get(k) & 0xffffff);
      *cvt_cycle = (*cvt_cycle << 24) | (get(j) & 0xffffff);
    }

    return 2;
  }

  Dprintf(("trace index %d does not point to lower part (0x%x)\n", j, get(j)));
  return 1;
}


//------------------------------------------------------------------------
//
// dump1 - decode a single trace buffer item.
//
//
// RETURNS 2 if the trace item takes two trace entries, otherwise returns 1.

int Trace::dump1(unsigned index, char *buffer, int bufsize)
{
  uint64_t cycle;
  int return_value = is_cycle_trace(index, &cycle);

  if (bufsize) {
    buffer[0] = 0;  // 0 terminate just in case no string is created
  }

  if (return_value == 2) {
    return return_value;
  }

  return_value = 1;

  switch (type(index)) {
  case NOTHING:
    snprintf(buffer, bufsize, "  empty trace cycle");
    break;

  /*
  case WRITE_TRIS:
  snprintf(buffer, bufsize,"  wrote: 0x%02x to TRIS",
           get(index)&0xff);
  break;
  case BREAKPOINT:
  snprintf(buffer, bufsize,"BREAK: ");
  bp.dump_traced(get(index) & 0xffffff);
  break;
  case _RESET:
  switch( (RESET_TYPE) (get(index)&0xff))
    {
    case POR_RESET:
      snprintf(buffer, bufsize," POR");
      break;
    case WDT_RESET:
      snprintf(buffer, bufsize," WDT reset");
      break;
    case SOFT_RESET:
      snprintf(buffer, bufsize,"SOFT reset");
      break;
    default:
      snprintf(buffer, bufsize,"unknown reset");
    }
  break;

  case OPCODE_WRITE:
  if(type(index-1) == OPCODE_WRITE)
    snprintf(buffer, bufsize," wrote opcode: 0x%04x to pgm memory: 0x%05x",
           get(index)&0xffff,
           get(index - 1) & 0xffffff);

  break;
  */
  default:
    if ((type(index) != (unsigned) CYCLE_COUNTER_HI) && (type(index) != CYCLE_COUNTER_MI)) {
      std::map<unsigned int, TraceType *>::iterator tti = trace_map.find(type(index));

      if (tti != trace_map.end()) {
        TraceType *tt = (*tti).second;

        if (tt) {
          tt->dump_raw(this, index, buffer, bufsize);
          return_value = tt->entriesUsed(this, index);
        }

        break;
      }

      if (cpu) {
        return_value = cpu->trace_dump1(get(index), buffer, bufsize);
      }
    }
  }

  return return_value;
}


//------------------------------------------------------------------
void Trace::enableLogging(const char *fname)
{
  if (fname) {
    logger.enable(fname);
  }
}


void Trace::disableLogging()
{
  logger.disable();
}


//------------------------------------------------------------------
// int Trace::dump(int n, FILE *out_stream)
//

int Trace::dump(int n, FILE *out_stream)
{
  if (!cpu) {
    return 0;
  }

  if (n < 0) {
    n = TRACE_BUFFER_SIZE - 1;
  }

  if (!n) {
    n = 5;
  }

  if (!out_stream) {
    return 0;
  }

  if (!pCycleTrace) {
    // ugh
    // the trace_map needs to be a member of Trace, other wise
    // there's a global constructor initialization race condition.
    pCycleTrace = new CycleTraceType(2);
    trace_map[CYCLE_COUNTER_LO] = pCycleTrace;
    trace_map[CYCLE_COUNTER_MI] = pCycleTrace;
    trace_map[CYCLE_COUNTER_HI] = pCycleTrace;
  }

  unsigned int frames = n + 1;
  unsigned int frame_start = tbi(trace_index - 3);
  uint64_t cycle = 0;

  if (trace.is_cycle_trace(frame_start, &cycle) !=  2) {
    return 0;
  }

  unsigned int frame_end = trace_index;
  unsigned int k = frame_start;
  // Save the state of the CPU here.
  cpu->save_state();
  //
  // Decode the trace buffer
  //
  // Starting at the end of the trace buffer, step backwards
  // and count 'n' trace frames. A trace frame describes a
  // boundary. All of the traced information between frames
  // describe what happened at the boundary. For example,
  // when a movf temp,W executes, the Program counter creates
  // the frame boundary and the write to temp and read from W
  // are stored in it. The frame boundary is recorded at the
  // end of the frame.
  current_frame = 0;

  while (traceFrames.size() < frames && inRange(k, frame_end, frame_start)) {
    // Look up this trace type in the trace map
    std::map<unsigned int, TraceType *>::iterator tti = trace_map.find(type(k));

    if (tti != trace_map.end()) {
      // The trace type was found in the trace map
      // Now decode it. Note that this is where things
      // like trace frames are created (e.g. for PCTraceType
      // decode() creates a new trace frame).
      // If we're on the last frame, and this trace type is a
      // new frame, then we're done.
      TraceType *tt = (*tti).second;

      if (tt) {
        if (tt->isFrameBoundary() && traceFrames.size() == frames - 1) {
          break;  // We're done!
        }

        TraceObject *pTO = tt->decode(k);

        if (pTO) {
          addToCurrentFrame(pTO);
        }
      }

      if (is_cycle_trace(k, &cycle) == 2) {
        current_cycle_time = cycle;
      }

    } else if (get(k) != NOTHING) {
      std::cout << " could not decode trace type: 0x" << std::hex << get(k) << '\n';
      addToCurrentFrame(new InvalidTraceObject(get(k)));
    }

    k = tbi(k - 1);
  }

  printTraceFrame(out_stream);
  deleteTraceFrame();
  fflush(out_stream);
  return n;
}


//------------------------------------------------------------------------
// allocateTraceType - allocate one or more trace commands
//
//
unsigned int Trace::allocateTraceType(TraceType *tt)
{
  if (tt) {
    unsigned int i;
    unsigned int *ltt = &lastTraceType;
    unsigned int n = 1 << 24;

    if (tt->bitsTraced() < 24) {
      if (lastSubTraceType == 0) {
        lastSubTraceType = lastTraceType;
        lastTraceType += n;
      }

      ltt = &lastSubTraceType;
      n = 1 << 16;
    }

    tt->setType(*ltt);;

    for (i = 0; i < tt->size(); i++) {
      trace_map[*ltt] = tt;
      *ltt += n;
    }

    return tt->type();
  }

  return 0;
}


//---------------------------------------------------------
// dump_raw
// mostly for debugging,
void Trace::dump_raw(int n)
{
  if (!n) {
    return;
  }

  FILE *out_stream = stdout;
  const int BUFFER_SIZE = 256;
  char buffer[BUFFER_SIZE];
  unsigned int i = (trace_index - n)  & TRACE_BUFFER_MASK;
  trace_flag = TRACE_ALL;

  do {
    fprintf(out_stream, "%04X:", i);
    std::map<unsigned int, TraceType *>::iterator tti = trace_map.find(type(i));
    unsigned int tSize = 1;
    TraceType *tt = tti != trace_map.end() ? (*tti).second : 0;
    buffer[0] = 0;
    tSize = 0;

    if (tt) {
      tSize = tt->entriesUsed(this, i);
      /*
        fprintf(out_stream, "%02X:",tSize);
        for (unsigned int ii=0; ii<tSize; ii++)
          fprintf(out_stream, "%08X:",get(i+ii));
      */
      tt->dump_raw(this, i, buffer, sizeof(buffer));
    }

    if (!tSize) {
      fprintf(out_stream, "%08X:  ??", get(i));
    }

    if (buffer[0]) {
      fprintf(out_stream, "%s", buffer);
    }

    tSize = tSize ? tSize : 1;
    i = (i + tSize) & TRACE_BUFFER_MASK;
    putc('\n', out_stream);
  } while ((i != trace_index) && (i != ((trace_index + 1)&TRACE_BUFFER_MASK)));

  putc('\n', out_stream);
  putc('\n', out_stream);
}


//
// dump_last_instruction()

void Trace::dump_last_instruction()
{
  if (trace_log.log_file) {
    dump(1, trace_log.log_file);
  }

  dump(1, stdout);
}


/*****************************************************************
 *
 *         Logging
 */
TraceLog::TraceLog()
{
  buffer.trace_flag = TRACE_ALL;
}


TraceLog::~TraceLog()
{
  disable_logging();
  close_logfile();
}


void TraceLog::callback()
{
  /*
  int n = 0;
  get_trace().cycle_counter(get_cycles().get());

  if((log_file||lxtp) && logging) {
    if(last_trace_index < get_trace().trace_index) {
      for (unsigned int c=last_trace_index; c<get_trace().trace_index; c++)
        if ((get_trace().trace_buffer[c] & 0xff000000) == Trace::INSTRUCTION)
          n++;
    } else {
      for (unsigned int c=last_trace_index; c<=TRACE_BUFFER_MASK; c++)
        if ((get_trace().trace_buffer[c] & 0xff000000) == Trace::INSTRUCTION)
          n++;
      for (unsigned int c=0; c<get_trace().trace_index; c++)
        if ((get_trace().trace_buffer[c] & 0xff000000) == Trace::INSTRUCTION)
          n++;
    }

    //trace.dump(n, log_file, watch_reg);
    if(file_format==TRACE_FILE_FORMAT_ASCII)
        trace.dump(n, log_file);

    last_trace_index = trace.trace_index;
    get_cycles().set_break(get_cycles().get() + 1000,this);
  }
  */
}


void TraceLog::open_logfile(const char *new_fname, int format)
{
  if (!new_fname) {
    switch (format) {
    case TRACE_FILE_FORMAT_LXT:
      new_fname = "gpsim.lxt";
      break;

    case TRACE_FILE_FORMAT_ASCII:
      new_fname = "gpsim.log";
      break;
    }
  }

  if (!log_filename.empty()) {
    //
    // Looks like there's a log file open and now we
    // want to open a different one.
    //
    if (log_filename == new_fname) {
      return;  // the file with this name is already opened
    }

    close_logfile();
  }

  file_format = format;

  switch (file_format) {
  case TRACE_FILE_FORMAT_ASCII:
    log_file = fopen(new_fname, "w");
    lxtp = nullptr;
    break;

  case TRACE_FILE_FORMAT_LXT:
    lxtp = lt_init(new_fname);
    lt_set_timescale(lxtp, -8);
    lt_set_clock_compress(lxtp);
    lt_set_initial_value(lxtp, 'X');
    log_file = nullptr;
    break;
  }

  log_filename = new_fname;
  items_logged = 0;
}


void TraceLog::close_logfile()
{
  if (!log_filename.empty()) {
    switch (file_format) {
    case TRACE_FILE_FORMAT_ASCII:
#ifndef NEWTRACE
      write_logfile();
#endif
      fclose(log_file);
      break;

    case TRACE_FILE_FORMAT_LXT:
      lt_close(lxtp);
      break;
    }

    log_file = nullptr;
    log_filename.clear();
  }
}


void TraceLog::write_logfile()
{
#ifndef NEWTRACE
  unsigned int i, j;
  char buf[256];
  uint64_t cycle = 0;

  if (log_file) {
    buffer.trace_flag = TRACE_ALL;

    // Loop through the trace buffer and decode each entry.
    // Note that the second loop counter, j, keeps tabs on first, i.

    for (i = 0, j = 0; i < buffer.trace_index && j < buffer.trace_index; j++) {
      buf[0] = 0;

      if (buffer.is_cycle_trace(i, &cycle)) {
        fprintf(log_file, "Cycle 0x%016" PRINTF_GINT64_MODIFIER "X ", cycle);
      }

      i = (i + buffer.dump1(i, buf, sizeof(buf))) & TRACE_BUFFER_MASK;

      if (buf[0]) {
        items_logged++;
        fprintf(log_file, "%s\n", buf);

      } else {
        fprintf(log_file, "\n");
      }
    }

    buffer.trace_index = 0;
  }

#endif //NEWTRACE
}


void TraceLog::enable_logging(const char *new_fname, int format)
{
  if (logging) {
    return;
  }

  if (!cpu) {
    if (get_active_cpu()) {
      cpu = get_active_cpu();

    } else {
      std::cout << "Warning: Logging can't be enabled until a cpu has been selected.";
    }
  }

  buffer.cpu = cpu;
  open_logfile(new_fname, format);
  last_trace_index = buffer.trace_index;
  // cycles.set_break(cycles.value + 1000,this);
  logging = 1;
}


void TraceLog::disable_logging()
{
  if (!logging) {
    return;
  }

  close_logfile();
  logging = false;
}


void TraceLog::status()
{
  if (logging) {
    std::cout << "Logging to file: " << log_filename;

    switch (file_format) {
    case TRACE_FILE_FORMAT_LXT:
      std::cout << " in LXT mode\n";
      break;

    case TRACE_FILE_FORMAT_ASCII:
    default:
      std::cout << " in ASCII mode\n";
      break;
    }

    // note that there's the cycle counter is traced for every
    // item that is in the log buffer, so the actual events that
    // triggered a log is the total buffer size divided by 2.
    //int total_items = (buffer.trace_index + items_logged)/2;
    int total_items = (buffer.trace_index + items_logged);

    if (total_items) {
      std::cout << "So far, it contains " << std::hex << "0x" << total_items << " logged events\n";

    } else {
      std::cout << "Nothing has been logged yet\n";
    }

    int first = 1;

    for (int i = 0; i < MAX_BREAKPOINTS; i++) {
      if (
        (bp.break_status[i].type == bp.NOTIFY_ON_REG_READ) ||
        (bp.break_status[i].type == bp.NOTIFY_ON_REG_WRITE) ||
        (bp.break_status[i].type == bp.NOTIFY_ON_REG_READ_VALUE) ||
        (bp.break_status[i].type == bp.NOTIFY_ON_REG_WRITE_VALUE)
      ) {
        if (first) {
          std::cout << "Log triggers:\n";
        }

        first = 0;
        bp.dump1(i);
      }
    }

  } else {
    std::cout << "Logging is disabled\n";
  }
}


void TraceLog::switch_cpus(Processor *pcpu)
{
  cpu = pcpu;
}


void TraceLog::lxt_trace(unsigned int address, unsigned int value, uint64_t /* cc */ )
{
  char *name = (char *)cpu->registers[address]->name().c_str();
  items_logged++;
  lt_set_time(lxtp, (int)(get_cycles().get() * 4.0e8 * cpu->get_OSCperiod()));
  symp = lt_symbol_find(lxtp, name);

  if (symp == 0) {
    symp = lt_symbol_add(lxtp,
                         name,         // name
                         0,            // rows
                         7,            // msb
                         0,            // lsb
                         LT_SYM_F_BITS //flags
                        );
    assert(symp != 0);
  }

  lt_emit_value_int(lxtp, symp, 0, value);
}


void TraceLog::register_read(Register *pReg, uint64_t cc)
{
  if (!pReg) {
    return;
  }

  switch (file_format) {
  case TRACE_FILE_FORMAT_ASCII:
    Dprintf(("cycle=%" PRINTF_GINT64_MODIFIER "x %s(0x%02x) value 0x%02x\n", cc, pReg->name().c_str(), pReg->getAddress(), pReg->get_value()));
#ifdef NEWTRACE
    bp.set_logging();
    return;
#else
    buffer.cycle_counter(cc);
    buffer.raw(pReg->read_trace.get() | pReg->get_value());

    if (buffer.near_full()) {
      write_logfile();
    }

#endif //NEWTRACE
    break;

  case TRACE_FILE_FORMAT_LXT:
    Dprintf(("LXT cycle=%" PRINTF_GINT64_MODIFIER "x %s(0x%02x) value 0x%02x\n", cc, pReg->name().c_str(), pReg->getAddress(), pReg->get_value()));
    lxt_trace(pReg->getAddress(), pReg->get_value(), cc);
    break;
  }
}


void TraceLog::register_write(Register *pReg, uint64_t cc)
{
  if (!pReg) {
    return;
  }

  switch (file_format) {
  case TRACE_FILE_FORMAT_ASCII:
    Dprintf(("cycle=%" PRINTF_GINT64_MODIFIER "x %s(0x%02x) value 0x%02x\n", cc, pReg->name().c_str(), pReg->getAddress(), pReg->get_value()));
#ifdef NEWTRACE
    bp.set_logging();
    return;
#else
    buffer.cycle_counter(cc);
    buffer.raw(pReg->write_trace.get() | pReg->get_value());

    if (buffer.near_full()) {
      write_logfile();
    }

#endif //NEWTRACE
    break;

  case TRACE_FILE_FORMAT_LXT:
    Dprintf(("LXT cycle=%" PRINTF_GINT64_MODIFIER "x %s(0x%02x) value 0x%02x\n", cc, pReg->name().c_str(), pReg->getAddress(), pReg->get_value()));
    lxt_trace(pReg->getAddress(), pReg->get_value(), cc);
    break;
  }
}


void TraceLog::register_read_value(Register *pReg, uint64_t cc)
{
  if (!pReg) {
    return;
  }

  switch (file_format) {
  case TRACE_FILE_FORMAT_ASCII:
    Dprintf(("cycle=%" PRINTF_GINT64_MODIFIER "x %s(0x%02x) value 0x%02x\n", cc, pReg->name().c_str(), pReg->getAddress(), pReg->get_value()));
#ifdef NEWTRACE
    bp.set_logging();
    return;
#else
    buffer.cycle_counter(cc);
    buffer.raw(pReg->read_trace.get() | pReg->get_value());

    if (buffer.near_full()) {
      write_logfile();
    }

#endif //NEWTRACE
    break;

  case TRACE_FILE_FORMAT_LXT:
    Dprintf(("LXT cycle=%" PRINTF_GINT64_MODIFIER "x %s(0x%02x) value 0x%02x\n", cc, pReg->name().c_str(), pReg->getAddress(), pReg->get_value()));
    lxt_trace(pReg->getAddress(), pReg->get_value(), cc);
    break;
  }
}


void TraceLog::register_write_value(Register *pReg, uint64_t cc)
{
  if (!pReg) {
    return;
  }

  switch (file_format) {
  case TRACE_FILE_FORMAT_ASCII:
    Dprintf(("cycle=%" PRINTF_GINT64_MODIFIER "x %s(0x%x) value %x\n", cc, pReg->name().c_str(), pReg->getAddress(), pReg->get_value()));
#ifdef NEWTRACE
    bp.set_logging();
    return;
#else
    buffer.cycle_counter(cc);
    buffer.raw(pReg->write_trace.get() | pReg->get_value());

    if (buffer.near_full()) {
      write_logfile();
    }

#endif //NEWTRACE
    break;

  case TRACE_FILE_FORMAT_LXT:
    Dprintf(("LXT cycle=%" PRINTF_GINT64_MODIFIER "x %s(0x%x) value %x\n", cc, pReg->name().c_str(), pReg->getAddress(), pReg->get_value()));
    lxt_trace(pReg->getAddress(), pReg->get_value(), cc);
    break;
  }
}


/*****************************************************************
 *
 *         Profiling
 */
ProfileKeeper::ProfileKeeper()
{
}


ProfileKeeper::~ProfileKeeper()
{
  disable_profiling();
}


void ProfileKeeper::catchup()
{
  //Register *r;
  if (!enabled) {
    return;
  }

  for (unsigned int i = last_trace_index; i != trace.trace_index; i = (i + 1)& TRACE_BUFFER_MASK) {
    /*
    switch (trace.trace_buffer[i] & 0xff000000)
    {
    case Trace::INSTRUCTION:
      instruction_address=trace_pc_value;
      cpu->program_memory[instruction_address]->cycle_count++;
      trace_pc_value++;
      break;

    case Trace::PROGRAM_COUNTER:
    case Trace::PC_SKIP:
      trace_pc_value=trace.trace_buffer[i]&0xffff;
      break;
    case Trace::CYCLE_INCREMENT:
      cpu->program_memory[instruction_address]->cycle_count++;
      break;
    case Trace::REGISTER_READ:
      r = cpu->registers[(trace.trace_buffer[i]>>8) & 0xfff];
      if(r->isa() == Register::FILE_REGISTER)
      {
          r->read_access_count++;
      }
      break;
    case Trace::REGISTER_WRITE:
      r = cpu->registers[(trace.trace_buffer[i]>>8) & 0xfff];
      if(r->isa() == Register::FILE_REGISTER)
      {
          r->write_access_count++;
      }
      break;
          break;
    }
      */
  }

  last_trace_index = trace.trace_index;
}


void ProfileKeeper::callback()
{
  if (enabled) {
    catchup();
    get_cycles().set_break(get_cycles().get() + 1000, this);
  }
}


void ProfileKeeper::enable_profiling()
{
  if (enabled) {
    return;
  }

  if (!cpu) {
    if (get_active_cpu()) {
      cpu = get_active_cpu();

    } else {
      std::cout << "Warning: Profiling can't be enabled until a cpu has been selected.";
    }
  }

  last_trace_index = trace.trace_index;
  get_cycles().set_break(get_cycles().get() + 1000, this);
  enabled = true;
}


void ProfileKeeper::disable_profiling()
{
  if (!enabled) {
    return;
  }

  enabled = false;
}


void ProfileKeeper::switch_cpus(Processor *pcpu)
{
  cpu = pcpu;
}


//*****************************************************************
// *** KNOWN CHANGE ***
//  Support functions that will get replaced by the CORBA interface.
//

//--------------------------------------------
void trace_dump_all()
{
  trace.dump(0, stdout);
}


//--------------------------------------------
void trace_dump_n(int numberof)
{
  trace.dump(numberof, stdout);
}


//--------------------------------------------
void trace_dump_raw(int numberof)
{
  trace.dump_raw(numberof);
}


//--------------------------------------------
void trace_enable_logging(char *file, int format)
{
  if (file) {
    trace_log.enable_logging(file, format);

  } else {
    trace_log.disable_logging();
  }
}


void trace_watch_register(int )
{
  //trace_log.watch_reg = reg;
}


//--------------------------------------------------
//  BoolEventBuffer::event(bool state)
//
//    Record a 0/1 event (e.g. the state of an I/O line).
//    returns false if this event has filled the buffer
//    or if the buffer is full. Note, an event will get lost
//    if the callee attempts to save it in a full buffer.

inline bool BoolEventBuffer::event(bool state)
{
  // If the new event is different than the most recently logged one
  // then we need to log this event. (Note that the event is implicitly
  // logged in the "index". I.e. 1 events are at odd indices.
  if (state ^ (index & 1) ^ (!bInitialState))  {
    if (index < max_events) {
      buffer[index++] = get_cycles().get() - start_time;
      return true;
    }

    return false;
  }

  return true;
}


//--------------------------------------------------
//
// BoolEventBuffer::get_index
//
// given an event time, get_index will perform a binary
// search for it in the event buffer.
//

unsigned int BoolEventBuffer::get_index(uint64_t event_time)
{
  uint32_t start_index, search_index, bstep;
  uint64_t time_offset;
  start_index = 0;
  bstep = (max_events + 1) >> 1;
  search_index = start_index + bstep;
  bstep >>= 1;
  time_offset = event_time - start_time;

  // Binary search for the event time:
  do {
    if (time_offset == buffer[search_index]) {
      return search_index;
    }

    if (time_offset < buffer[search_index]) {
      search_index = search_index - bstep;

    } else {
      search_index = search_index + bstep;
    }

    //cout << hex << "search index "<< search_index << "  buffer[search_index] " << buffer[search_index] << '\n';
    bstep >>= 1;
  } while (bstep);

  if (time_offset >= buffer[search_index]) {
    return search_index;

  } else {
    return (--search_index);
  }
}


//--------------------------------------------------
//
// BoolEventBuffer::activate
//
//
void BoolEventBuffer::activate(bool _initial_state)
{
  // If the buffer is activated already or the buffer is full,
  // then we can't activate it.
  if (isActive() || isFull()) {
    return;
  }

  // Save the time for this initial event
  start_time = get_cycles().get();
  bInitialState = _initial_state;
  index = 0;  // next state gets stored at the first position in the buffer.
  bActive = true;
  future_cycle = start_time + (1UL << 31);
  get_cycles().set_break(future_cycle, this);
}


//--------------------------------------------------
void BoolEventBuffer::deactivate()
{
  bActive = false;

  if (future_cycle) {
    get_cycles().clear_break(this);
  }

  future_cycle = 0;
}


//--------------------------------------------------
void BoolEventBuffer::callback()
{
  future_cycle = 0;

  if (isActive()) {
    deactivate();
  }
}


void BoolEventBuffer::callback_print()
{
  std::cout << "BoolEventBuffer\n";
}


//--------------------------------------------------
// BoolEventBuffer -- constructor
//
BoolEventBuffer::BoolEventBuffer(bool _initial_state, uint32_t _max_events)
    : max_events(_max_events)
{
  // Make sure that max_events is an even power of 2
  if (max_events & (max_events - 1)) {
    max_events <<= 1;

    while (1) {
      if (max_events && (max_events & (max_events - 1))) {
        max_events &= max_events - 1;

      } else {
        break;
      }
    }

  } else if (!max_events) {
    max_events = 4096;
  }

  max_events--;  // make the max_events a mask
  buffer = new uint64_t[max_events];
  activate(_initial_state);
}


BoolEventBuffer::~BoolEventBuffer()
{
  delete [] buffer;
}
