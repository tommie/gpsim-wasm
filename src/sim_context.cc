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

#include <stdio.h>

#include <iostream>
#include <string>
#include <utility>

#ifndef _WIN32
#if !defined(_MAX_PATH)
#define _MAX_PATH 1024
#endif
#include <unistd.h>
#else
#include <direct.h>
#endif

#include "fopen-path.h"
#include "gpsim_interface.h"
#include "gpsim_time.h"
#include "modules.h"
#include "processor.h"
#include "program_files.h"
#include "sim_context.h"
#include "breakpoints.h"
#include "trace.h"
#include "symbol.h"
#include "ui.h"

//================================================================================
// Global Declarations
//  FIXME -  move these global references somewhere else

// don't print out a bunch of garbage while initializing


//================================================================================
//
// pic_processor
//
// This file contains all (most?) of the code that simulates those features
// common to all pic microcontrollers.
//
//

CSimulationContext::CSimulationContext()
  :  m_bEnableLoadSource(*new Boolean("EnableSourceLoad", true,
                                      "Enables and disables loading of source code"))
{
  globalSymbolTable().addSymbol(&m_bEnableLoadSource);
}


CSimulationContext::~CSimulationContext()
{
  globalSymbolTable().deleteSymbol("EnableSourceLoad");
}


void CSimulationContext::Initialize()
{
}


bool CSimulationContext::SetDefaultProcessor(const char * processor_type,
    const char * processor_new_name)
{
  if (processor_type) {
    ProcessorConstructor *pc = ProcessorConstructor::findByType(processor_type);

    if (pc) {
      m_DefProcessorName = processor_type;

      if (processor_new_name == nullptr) {
        m_DefProcessorNameNew.clear();

      } else {
        m_DefProcessorNameNew = processor_new_name;
      }

      return true;
    }

  } else {
    m_DefProcessorNameNew = processor_new_name;
  }

  return false;
}


//-------------------------------------------------------------------
Processor * CSimulationContext::SetProcessorByType(const char * processor_type,
    const char * processor_new_name)
{
  Processor *p;
  CProcessorList::iterator it = find_by_type(processor_type);
  GetBreakpoints().clear_all(GetActiveCPU());
  std::cout << __func__ << " FIXME \n";
  // GetSymbolTable().Reinitialize();

  if (processor_list.end() == it) {
    p = add_processor(processor_type, processor_new_name);

  } else {
    delete it->second;
    p = add_processor(processor_type, processor_new_name);
    //    p->init
  }

  return p;
}


//-------------------------------------------------------------------
Processor * CSimulationContext::add_processor(const char * processor_type,
    const char * processor_new_name)
{
  if (verbose && processor_new_name) {
    std::cout << "Trying to add new processor '" << processor_type << "' named '"
         << processor_new_name << "'\n";
  }

  ProcessorConstructor *pc = ProcessorConstructor::findByType(processor_type);

  if (pc) {
    return add_processor(pc, processor_new_name ? processor_new_name : m_DefProcessorNameNew.c_str());

  } else {
    std::cout << processor_type << " is not a valid processor.\n"
         "(try 'processor list' to see a list of valid processors.\n";
  }

  return nullptr;
}


Processor * CSimulationContext::add_processor(ProcessorConstructor *pc,
    const char * processor_new_name)
{
  Processor *p = pc->ConstructProcessor(processor_new_name);

  if (p) {
    add_processor(p);
    p->m_pConstructorObject = pc;

  } else {
    std::cout << " unable to add a processor (BUG?)\n";
  }

  return p;
}


Processor * CSimulationContext::add_processor(Processor *p)
{
  processor_list.insert(CProcessorList::value_type(p->name(), p));
  //p->initializeAttributes();
  active_cpu = p;
  //p->processor_id =
  active_cpu_id = ++cpu_ids;

  if (verbose) {
    std::cout << p->name()
      << "\nProgram Memory size " <<  p->program_memory_size()
      << " words\nRegister Memory size " <<  p->register_memory_size() << '\n';
  }

  trace.switch_cpus(p);
  // Tell the gui or any modules that are interfaced to gpsim
  // that a new processor has been declared.
  gi.new_processor(p);
  return p;
}


int CSimulationContext::LoadProgram(const char *filename,
                                    const char *pProcessorType,
                                    Processor **ppProcessor,
                                    const char *pProcessorName)
{
  bool bReturn = false;
  Processor *pProcessor;
  FILE *pFile = fopen_path(filename, "rb");

  if (pFile == nullptr) {
    char cw[_MAX_PATH];
    perror((std::string("failed to open program file ") + filename).c_str());

    if (!getcwd(cw, sizeof(cw))) {
      perror("getcwd failed: ");

    } else {
      std::cerr << "current working directory is " << cw << '\n';
    }

    return false;
  }

  if (pProcessorType) {
    pProcessor = SetProcessorByType(pProcessorType, nullptr);

    if (pProcessor) {
      bReturn  = pProcessor->LoadProgramFile(filename, pFile, pProcessorName, this);
    }

  } else if (!m_DefProcessorName.empty()) {
    pProcessor = SetProcessorByType(m_DefProcessorName.c_str(), nullptr);

    if (pProcessor) {
      bReturn  = pProcessor->LoadProgramFile(filename, pFile, pProcessorName, this);
    }

  } else {
    pProcessor = nullptr;

    if (!m_DefProcessorNameNew.empty()) {
      pProcessorName = m_DefProcessorNameNew.c_str();
    }

    // use processor defined in program file
    bReturn  = ProgramFileTypeList::GetList().LoadProgramFile(
        &pProcessor, filename, pFile, pProcessorName, this);
  }

  fclose(pFile);

  if (bReturn) {
    // Tell all of the interfaces that a new program exists.
    gi.new_program(pProcessor);
  }

  if (ppProcessor) {
    *ppProcessor = pProcessor;
  }

  return bReturn;
}


//------------------------------------------------------------------------
// dump_processor_list - print out all of the processors a user is
//                       simulating.

void CSimulationContext::dump_processor_list()
{
  std::cout << "Processor List\n";

  for (const auto &vt : processor_list) {
    Processor *p = vt.second;
    std::cout << p->name() << '\n';
  }

  if (processor_list.empty()) {
    std::cout << "(empty)\n";
  }
}


void CSimulationContext::Clear()
{
  for (auto &vt : processor_list) {
    Processor *p = vt.second;
    GetBreakpoints().clear_all(p);
    delete p;
  }

  processor_list.clear();
}


void CSimulationContext::Reset(RESET_TYPE )
{
  /*
  Symbol_Table &ST = get_symbol_table();
  Symbol_Table::module_symbol_iterator it;
  Symbol_Table::module_symbol_iterator itEnd = ST.endModuleSymbol();
  for(it = ST.beginModuleSymbol(); it != itEnd; it++) {
      Module *m = (*it)->get_module();
      if(m) {
        m->reset(r);
      }
  }
  */
  std::cout << __func__ << " FIXME \n";
}


void CSimulationContext::NotifyUserCanceled()
{
  if (m_pbUserCanceled) {
    *m_pbUserCanceled = true;
    m_pbUserCanceled = nullptr;
    return;
  }

  if (GetActiveCPU() && GetActiveCPU()->simulation_mode == eSM_RUNNING) {
    // If we get a CTRL->C while processing a command file
    // we should probably stop the command file processing.
    GetBreakpoints().halt();
  }
}


SymbolTable & CSimulationContext::GetSymbolTable()
{
  return gSymbolTable;
}


Breakpoints & CSimulationContext::GetBreakpoints()
{
  return get_bp();
}


Processor * CSimulationContext::GetActiveCPU()
{
  return get_active_cpu();
}


Cycle_Counter * CSimulationContext::GetCycleCounter()
{
  return &cycles;
}


CSimulationContext::CProcessorList::iterator
CSimulationContext::find_by_type(const CProcessorList::key_type& Keyval)
{
  // First find a ProcessorConstructor that matches the
  // processor type we are looking for. This should handle
  // naming variations.
  ProcessorConstructor * pc = ProcessorConstructor::findByType(Keyval.c_str());
  auto itEnd = processor_list.end();

  if (pc == nullptr) {
    return itEnd;
  }

  // Now find the specific allocated processor that
  // was created with the ProcessorConstructor object
  // we found above.

  for (auto it = processor_list.begin(); it != itEnd; ++it) {
    if (it->second->m_pConstructorObject == pc) {
      return it;
    }
  }

  return itEnd;
}
