/*
   Copyright (C) 1998,1999,2000,2001,2002,2003,2004
   T. Scott Dattalo and Ralf Forsberg

This file is part of gpsim.

gpsim is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

gpsim is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with gpsim; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#ifndef GUI_GUI_REGISTER_H_
#define GUI_GUI_REGISTER_H_

#include "gui.h"
#include <string>

//========================================================================
//
// A GUI register is a shadow of a register in a simulated cpu (or module).
//
//
// FIXME -- why not just derive from the Register base class?

class GUIRegister {
 public:
  RegisterMemoryAccess *rma;  // Pointer to the rma that controls this register.
  int address;                // index in rma register array.

  int row;             // row & col in register window
  int col;
  RegisterValue shadow;// value displayed in register window.
                       // This shadows the real register value. It's used
                       // to monitor if register has changed between gui updates

  int register_size;   // The size (in bytes) of a single register
  bool bUpdateFull;    // true if a full update needs to be performed

  bool bIsAliased;     // true if this register is aliased
                       // and this instance is not the base.

  bool bIsValid();     // true if this register is a valid one (i.e.
                       // there's a defined register at the address).

  bool bIsSFR();       // true if this register is a special function register

  bool hasBreak();     // True if there's a breakpoint set on this register.

  bool hasChanged(RegisterValue &current_value) const;  // True if register has changed values since last updated.

  CrossReferenceToGUI *xref;

  char *getValueAsString(char *, int,  RegisterValue value);

  void put_value(unsigned int new_value);
  unsigned int get_value();
  RegisterValue getRV();

  inline operator RegisterValue() {
    return getRV();
  }

  Register *get_register();

  // put and get for updating the shadow
  void put_shadow(RegisterValue new_value);
  RegisterValue get_shadow()
    {
      return shadow;
    }

  void Clear_xref();
  void Assign_xref(CrossReferenceToGUI *);

  std::string name();

  GUIRegister();
  ~GUIRegister();
};

#define MAX_REGISTERS        0x10000

class GUIRegisterList {
public:
  explicit GUIRegisterList(RegisterMemoryAccess *pRMA);
  ~GUIRegisterList();
  RegisterMemoryAccess *m_pRMA;  // Apointer to the Processor's rma or ema.
  GUIRegister * m_paRegisters[MAX_REGISTERS];

  GUIRegister * Get(int iAddress);
  GUIRegister * Get(unsigned int uAddress);
};

inline GUIRegister *GUIRegisterList::Get(int iAddress) {
   if (iAddress >= MAX_REGISTERS)
    return nullptr;
  return m_paRegisters[iAddress];
}

inline GUIRegister *GUIRegisterList::Get(unsigned int uAddress) {
  if (uAddress >= MAX_REGISTERS)
    return nullptr;
  return m_paRegisters[uAddress];
}


#endif // GUI_GUI_REGISTER_H_
