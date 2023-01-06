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

#ifndef SRC_BITLOG_H_
#define SRC_BITLOG_H_

#include <vector>

class Cycle_Counter;

// For guint64, etc.
#include <glib.h>

/**********************************************************************
 * ThreeState event logging
 *
 * The ThreeState event logger is a simple class for logging the time
 * of 3-state events. (FixMe - the bitlog and bytelog classes should
 * be deprecated and merged into this class).
 *
 * The event buffer stores both the event state and the 64-bit time
 * at which it occurred. Event states are 'chars' so it is up to the
 * client of this class to interpret what the events mean.
 *
 * Repeated events are not logged. E.g.. if two 1's are logged, the
 * second one is ignored.
 *
 */

class ThreeStateEventLogger {
public:
  explicit ThreeStateEventLogger(unsigned int _max_events = 4096);

  // Log an Event
  void event(char state);

  inline unsigned int get_index()
  {
    return index;
  }

  unsigned int get_index(guint64 event_time);
  unsigned int get_nEvents(guint64 start_time, guint64 stop_time);
  unsigned int get_nEvents(unsigned int start_index, unsigned int stop_index);
  char get_state(unsigned int index)
  {
    return pEventBuffer[index & max_events];
  }

  char get_state(guint64 event_time)
  {
    return get_state(get_index(event_time));
  }

  guint64 get_time(unsigned int index)
  {
    return pTimeBuffer[index & max_events];
  }
  void dump(int start_index, int end_index = -1);
  void dump_ASCII_art(guint64 time_step,
                      guint64 start_time,
                      int end_index = -1);

private:
  Cycle_Counter *gcycles;             // Point to gpsim's cycle counter.
  unsigned int   index;               // Index into the buffer
  std::vector<guint64> pTimeBuffer;   // Where the time is stored
  std::vector<char>    pEventBuffer;  // Where the events are stored
  unsigned int   max_events;          // Size of the event buffer
  bool           bHaveEvents = false; // True if any events have been acquired
};


#endif // SRC_BITLOG_H_
