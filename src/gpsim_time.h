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

#ifndef SRC_GPSIM_TIME_H_
#define SRC_GPSIM_TIME_H_

#include <glib.h>

#include "breakpoints.h"
#include "trigger.h"
class Boolean;
class Integer;

//---------------------------------------------------------
// Cycle Counter
//
// The cycle counter class is used to coordinate the timing
// between the different peripherals within a processor and
// in some cases, the timing between several simulated processors
// and modules.
//
// The smallest quantum of simulated time is called a 'cycle'.
// The simuluation engine increments a 'Cycle Counter' at quantum
// simulation step. Simulation objects that wished to be notified
// at a specific instance in time can set a cycle counter break
// point that will get invoked whenever the cycle counter reaches
// that instance.



//------------------------------------------------------------
//
// Cycle counter breakpoint list
//
// This is a friend class to the Cycle Counter class. Its purpose
// is to maintain a doubly linked list of cycle counter break
// points.

class Cycle_Counter_breakpoint_list {
public:
  Cycle_Counter_breakpoint_list();

  Cycle_Counter_breakpoint_list *getNext();
  Cycle_Counter_breakpoint_list *getPrev();
  void clear();
  void invoke();

  // This is the value compared to the cycle counter.
  guint64 break_value;

  // True when this break is active.
  bool bActive = false;

  // The breakpoint_number is a number uniquely identifying this
  // cycle counter break point. Note, this number is used only
  // when the break point was assigned by a user

  unsigned int breakpoint_number = 0;

  // If non-null, the TriggerObject will point to an object that will get invoked
  // when the breakpoint is encountered.

  TriggerObject *f = nullptr;

  // Doubly-linked list mechanics..
  // (these will be made private eventually)
  Cycle_Counter_breakpoint_list *next = nullptr;
  Cycle_Counter_breakpoint_list *prev = nullptr;
};


class Cycle_Counter {
public:
#define BREAK_ARRAY_SIZE  4
#define BREAK_ARRAY_MASK  (BREAK_ARRAY_SIZE -1)

  Cycle_Counter();
  ~Cycle_Counter();

  void preset(guint64 new_value);     // not used currently.

  /*
    increment - This inline member function is called once or
    twice for every simulated instruction. Its purpose is to
    increment the cycle counter using roll over arithmetic.
    If there's a breakpoint set on the new value of the cycle
    counter then the simulation is either stopped or a callback
    function is invoked. In either case, the break point is
    cleared.
  */

  inline void increment()
  {
    // This has been changed so the cycle counter (value)
    // is incremented after processing breakpoints

    // Increment the current cycle then check if
    // we have a break point set here
    if (value == break_on_this) {
      breakpoint();
    }

    value++;
    // Note that it's really inefficient to trace every cycle increment.
    // Instead, we implicitly trace the increments with the instruction traces.
  }

  /*
    advance the Cycle Counter by more than one instruction quantum.
    This is almost identical to the increment() function except that
    we allow the counter to be advanced by an arbitrary amount.
    They're separated only for efficiency reasons. This one runs slower.
  */
  inline void advance(guint64 step)
  {
    while (step--) {
      if (value == break_on_this) {
        breakpoint();
      }
    }

    value++;
  }

  // Return the current cycle counter value
  guint64 get()
  {
    return value;
  }

  // Return the cycle counter for some time off in the future:
  guint64 get(double future_time_from_now);

  bool set_break(guint64 future_cycle,
                 TriggerObject *f = nullptr, unsigned int abp = MAX_BREAKPOINTS);
  bool set_break_delta(guint64 future_cycle,
                       TriggerObject *f = nullptr, unsigned int abp = MAX_BREAKPOINTS);
  bool reassign_break(guint64 old_cycle, guint64 future_cycle, TriggerObject *f = nullptr);
  void clear_current_break(TriggerObject *f = nullptr);
  void dump_breakpoints();

  void clear_break(guint64 at_cycle);
  void clear_break(TriggerObject *f);
  void set_instruction_cps(guint64 cps);
  double instruction_cps()
  {
    return m_instruction_cps;
  }
  double seconds_per_cycle()
  {
    return m_seconds_per_cycle;
  }

  // Largest cycle counter value

  static const guint64  END_OF_TIME = 0xFFFFFFFFFFFFFFFFULL;


  bool reassigned = false;        // Set true when a break point is reassigned (or deleted)

  Cycle_Counter_breakpoint_list
  active,     // Head of the active breakpoint linked list
  inactive;   // Head of the inactive one.

  bool bSynchronous = false; // a flag that's true when the time per counter tick is constant

private:
  // The number of instruction cycles that correspond to one second
  double m_instruction_cps;
  double m_seconds_per_cycle;

  guint64 value = 0;          // Current value of the cycle counter.
  guint64 break_on_this;  // If there's a pending cycle break point, then it'll be this

  /*
    breakpoint
    when the member function "increment()" encounters a break point,
    breakpoint() is called.
  */

  void breakpoint();
};


#if defined(IN_MODULE) && defined(_WIN32)
// we are in a module: don't access cycles object directly!
#include "exports.h"
LIBGPSIM_EXPORT Cycle_Counter &get_cycles();
#else
// we are in gpsim: use of get_cycles() is recommended,
// even if cycles object can be accessed directly.
extern Cycle_Counter cycles;

inline Cycle_Counter &get_cycles()
{
  return cycles;
}
#endif



/// The stopwatch object is used to keep track of the amount of
/// time between events. It can be controlled either through the
/// class API or through its attributes
class StopWatch : public TriggerObject {
public:
  StopWatch();
  StopWatch(const StopWatch &) = delete;
  StopWatch& operator = (const StopWatch &) = delete;
  ~StopWatch();

  guint64 get();
  double get_time();

  void set_enable(bool);
  void set_direction(bool);
  void set_rollover(guint64);
  void set_value(guint64);

  void set_break(bool);

  void update();

  void callback() override;
  void callback_print() override;

private:
  Integer *value;
  Integer *rollover;
  Boolean *enable;
  Boolean *direction;

  bool count_dir;

  guint64 offset = 0;
  guint64 break_cycle = 0;
};


extern StopWatch *stop_watch;


#endif
