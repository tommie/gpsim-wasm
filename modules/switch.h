/*
   Copyright (C) 2002 Ralf Forsberg

This file is part of the libgpsim_modules library of gpsim

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


#ifndef MODULES_SWITCH_H_
#define MODULES_SWITCH_H_

/* IN_MODULE should be defined for modules */
#define IN_MODULE
#include "../config.h"
#include "../src/modules.h"
#include "../src/trigger.h"
class Float;

#ifdef HAVE_GUI
#include <gtk/gtk.h>
#else
struct GtkToggleButton;
#endif

namespace Switches {
class SwitchPin;       // defined and implemented in switch.cc
class SwitchAttribute; //    "            "

class SwitchBase : public Module, public TriggerObject {
public:
  SwitchBase(const char *_name, const char *_desc);
  ~SwitchBase();

  void update();
  virtual void setState(bool);

  // Inheritances from the Package class
  virtual void create_iopin_map();

  // Inheritance from Module class
  const virtual char *type()
  {
    return "switch";
  }

  virtual void do_voltage();
  virtual bool switch_closed()
  {
    return m_bCurrentState;
  }
  virtual SwitchPin * other_pin(SwitchPin *pin);

  // Attributes call back into the switch through here:
  double getZopen();
  double getZclosed();

protected:
  SwitchPin *m_pinA;
  SwitchPin *m_pinB;

  // State of the switch
  bool m_bCurrentState;
  SwitchAttribute *m_aState;

  // Switch resistance.
  Float *m_Zopen;
  Float *m_Zclosed;
};


class Switch : public SwitchBase {
public:
  explicit Switch(const char *_name = nullptr);
  ~Switch();

  void setState(bool) override;
  void buttonToggled();

  static Module *construct(const char *new_name);

protected:
  // The switch's graphical representation.
  GtkToggleButton *m_button;

private:
  void create_widget(Switch *sw);
  static void cb_buttonToggle(GtkToggleButton *button, SwitchBase *This);
};


}


#endif //  MODULES_SWITCH_H_
