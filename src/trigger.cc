/*
   Copyright (C) 2004 T. Scott Dattalo

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


#include "trigger.h"
#include "value.h"
#include "errors.h"
#include "ui.h"
#include "trace.h"
#include <iostream>

#include <stdio.h>
#include <string.h>

extern Integer *verbosity;  // in ../src/init.cc

static TriggerAction DefaultTrigger;

//------------------------------------------------------------------------
// TriggerAction
//
TriggerAction::TriggerAction()
{
}

TriggerAction::~TriggerAction()
{
}

bool TriggerAction::evaluate()
{
  action();
  return true;
}

bool TriggerAction::getTriggerState()
{
  return false;
}

void TriggerAction::action()
{
  if (verbosity && verbosity->getVal())
    std::cout << "Hit a Breakpoint!\n";
}

//------------------------------------------------------------------------
// SimpleTriggerAction
//
// For most cases... A single trigger action coupled with a single trigger
// object
SimpleTriggerAction::SimpleTriggerAction(TriggerObject *_to)
  : TriggerAction(), to(_to)
{
}

void SimpleTriggerAction::action()
{
  TriggerAction::action();
  if (to && verbosity && verbosity->getVal())
    to->print();
}

//------------------------------------------------------------------------
TriggerObject::TriggerObject()
  : bpn(0), CallBackID(0)
{
  set_action(&DefaultTrigger);
}

TriggerObject::TriggerObject(TriggerAction *ta)
  : bpn(0), CallBackID(0)
{
  if (ta)
    set_action(ta);
  else
    set_action(&DefaultTrigger);
}

TriggerObject::~TriggerObject()
{
  if (m_action != &DefaultTrigger)
    delete m_action;
}

void TriggerObject::callback()
{
  std::cout << "generic callback\n";
}

void TriggerObject::callback_print()
{
  std::cout << " has callback, ID =  0x" << CallBackID << '\n';
}

void  TriggerObject::clear_trigger()
{
}

void TriggerObject::print()
{
  char buf[256];
  buf[0] = '\0';
  if (buf[0]) {
    GetUserInterface().DisplayMessage("    Expr:%s\n", buf);
  }
  if (!message().empty())
    GetUserInterface().DisplayMessage("    Message:%s\n", message().c_str());
}

int TriggerObject::printTraced(Trace *, unsigned int, char *, int )
{
  return 0;
}

void TriggerObject::clear()
{
  std::cout << "clear Generic breakpoint " << bpn << '\n';
}

//------------------------------------------------------------------------
void TriggerObject::invokeAction()
{
  m_action->action();
}

//-------------------------------------------------------------------
void TriggerObject::new_message(const char *s)
{
  m_sMessage = s;
}


void TriggerObject::new_message(std::string &new_message)
{
  m_sMessage = new_message;
}
