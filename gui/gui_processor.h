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

#ifndef GUI_GUI_PROCESSOR_H_
#define GUI_GUI_PROCESSOR_H_

#define NEW_SOURCE_BROWSER

// Forward references to all of the classes.

class RAM_RegisterWindow;
class EEPROM_RegisterWindow;
class SourceWindow;
class SourceBrowser_Window;
class SourceBrowserParent_Window;
class Symbol_Window;
class Watch_Window;
class Stack_Window;
class Breadboard_Window;
class Trace_Window;
class Profile_Window;
class StopWatch_Window;
class Scope_Window;
class Processor;
class GUIRegisterList;

//  The gui_processor structure ties the gui window(s)
// to a pic that is being simulated.
//

class GUI_Processor {
 public:
  GUI_Processor();
  GUI_Processor(const GUI_Processor &) = delete;  // Mark non-copiable
  GUI_Processor& operator =(const GUI_Processor &) = delete;
  void SetCPU(Processor *new_cpu);

  RAM_RegisterWindow *regwin_ram;
  EEPROM_RegisterWindow *regwin_eeprom;
  SourceWindow *source_window;
  SourceBrowser_Window *program_memory;
  SourceBrowserParent_Window *source_browser;
  Symbol_Window *symbol_window;
  Watch_Window *watch_window;
  Stack_Window *stack_window;
  Breadboard_Window *breadboard_window;
  Trace_Window *trace_window;
  Profile_Window *profile_window;
  StopWatch_Window *stopwatch_window;
  Scope_Window *scope_window;

  // The pic that's associated with the gui
  Processor *cpu;

  GUIRegisterList * m_pGUIRamRegisters;
  GUIRegisterList * m_pGUIEEPromRegisters;
};


#endif // GUI_GUI_PROCESSOR_H_

