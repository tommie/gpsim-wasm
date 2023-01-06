/*
   Copyright (C) 1998-2000 T. Scott Dattalo

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

#ifndef SRC_SIM_CONTEXT_H_
#define SRC_SIM_CONTEXT_H_

#include <string>
#include <map>

#include "gpsim_classes.h"
#include "value.h"

class Breakpoints;
class Cycle_Counter;
class Processor;
class ProcessorConstructor;
class SymbolTable;


//-------------------------------------------------------------------
//
// Define a list for keeping track of the processors being simulated.
// (Recall, gpsim can simultaneously simulate more than one processor.)

class CSimulationContext {
  using CProcessorList = std::map<const std::string, Processor *>;
  CProcessorList::iterator find_by_type(const CProcessorList::key_type& Keyval);

public:
  CSimulationContext();
  ~CSimulationContext();

  Processor * add_processor(const char * processor_type,
                            const char * processor_new_name = nullptr);
  Processor * add_processor(ProcessorConstructor *pc,
                            const char * processor_new_name = nullptr);
  Processor * SetProcessorByType(const char * processor_type,
                                 const char * processor_new_name);
  Processor * add_processor(Processor  * p);
  int         LoadProgram(const char *filename,
                          const char *pProcessorType = nullptr,
                          Processor **ppProcessor = nullptr,
                          const char * processor_new_name = nullptr);
  void        dump_processor_list();
  bool        SetDefaultProcessor(const char * processor_type,
                                  const char * processor_new_name);
  void        Clear();
  void        Reset(RESET_TYPE r);

  void            Initialize();
  SymbolTable &   GetSymbolTable();
  Breakpoints &   GetBreakpoints();
  Processor *     GetActiveCPU();
  Cycle_Counter * GetCycleCounter();
  bool            IsSourceEnabled()
  {
    return m_bEnableLoadSource;
  }
  void            NotifyUserCanceled();
  void            SetUserCanceledFlag(bool *pbUserCanceled)
  {
    m_pbUserCanceled = pbUserCanceled;
  }

  static CSimulationContext *GetContext();

protected:
  CProcessorList processor_list;
  std::string m_DefProcessorName;
  std::string m_DefProcessorNameNew;
  bool *m_pbUserCanceled = nullptr;

  // active_cpu_id is the id of the currently active cpu. In other words:
  //  active_cpu_id == active_cpu->processor_id
  // It's redundant to define this id in addition to the *active_cpu pointer.
  // However, if there ever comes a day when the cli is truely separate from
  // the simulator, then it would be more convenient to deal with ints than
  // pointers.

  int active_cpu_id = 0;

  // cpu_ids is a counter that increments everytime a processor is added by the
  // user. If the user deletes a processor, this counter will not be affected.
  // The value of this counter will be assigned to the processor's id when a
  // new processor is added. It's purpose is to uniquely identifier user
  // processors.

  int cpu_ids = 0;
  Boolean &m_bEnableLoadSource; // deleted by Symbol_Table
  static CSimulationContext *s_SimulationContext;
};


#endif
