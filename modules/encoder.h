/*
   Copyright (C) 2004 Chris Emerson
   (based on push button by Carlos Ghirardelli)

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


#ifndef MODULES_ENC_H_
#define MODULES_ENC_H_

/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include "../src/trace.h"
#include "../src/trigger.h"
#include "../src/modules.h"
class IOPIN;

class Encoder : public Module, private TriggerObject {
  void create_widget(Encoder *enc);

public:
  explicit Encoder(const char *);
  ~Encoder();

  void test();
  void update();

  /* Send clockwise pulses */
  void send_cw();
  /* Send anticlockwise pulses */
  void send_ccw();

  // Inheritances from the Package class
  virtual void create_iopin_map() { _create_iopin_map(); }

  // Inheritance from Module class
  const virtual char *type()
  {
    return "encoder";
  }
  static Module *construct(const char *new_name);

  IOPIN *a_pin;
  IOPIN *b_pin;

private:
  enum rotate_state {
    rot_detent,
    rot_moving_cw,
    rot_moving_ccw
  };
  rotate_state rs;
  void toggle_a();
  void toggle_b();
  void schedule_tick();
  void callback() override;
  void _create_iopin_map();
};

#endif // MODULES_ENC_H_
