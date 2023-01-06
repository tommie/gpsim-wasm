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

#include "bitlog.h"
#include "gpsim_time.h"

#include <iostream>
#include <iomanip>

//------------------------------------------------------------------------

ThreeStateEventLogger::ThreeStateEventLogger(unsigned int _max_events)
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

  pTimeBuffer.resize(max_events);
  pEventBuffer.resize(max_events);

  gcycles = &get_cycles();
  max_events--;  // make the max_events a mask
  index = max_events;
}


unsigned int ThreeStateEventLogger::get_index(guint64 event_time)
{
  if (!bHaveEvents) {
    return 0;
  }

  unsigned long start_index, search_index, bstep;
  start_index = (index + 1) & max_events;
  bstep = (max_events + 1) >> 1;
  search_index = (start_index + bstep) & max_events;
  bstep >>= 1;

  // Binary search for the event time:
  do {
    if (event_time < pTimeBuffer[search_index]) {
      search_index = (search_index - bstep) & max_events;

    } else {
      search_index = (search_index + bstep) & max_events;
    }

    bstep >>= 1;
  } while (bstep);

  if (event_time == pTimeBuffer[search_index]) {
    return search_index;
  }

  if (event_time < pTimeBuffer[search_index] &&
      pTimeBuffer[search_index] != Cycle_Counter::END_OF_TIME) {
    search_index = (search_index - 1) & max_events;
  }

  return search_index;
}


unsigned int ThreeStateEventLogger::get_nEvents(unsigned int start_index, unsigned int stop_index)
{
  return (stop_index >= start_index) ? (stop_index - start_index) : (max_events - stop_index + start_index);
}


unsigned int ThreeStateEventLogger::get_nEvents(guint64 start_time, guint64 stop_time)
{
  unsigned int start_index = get_index(start_time);
  unsigned int stop_index  = get_index(stop_time);
  return get_nEvents(start_index, stop_index);
}


void ThreeStateEventLogger::event(const char state)
{
  // If the new event is different the most recently logged one
  // then we need to log this event. (Note that the event is implicitly
  // logged in the "index". I.e. 1 events are at odd indices.
  if (state != pEventBuffer[index]) {
    index = (index + 1) & max_events;
    pTimeBuffer[index] = gcycles->get();
    pEventBuffer[index] = state;
    bHaveEvents = true;
  }
}


void ThreeStateEventLogger::dump(int start_index, int end_index)
{
  if (!bHaveEvents) {
    return;
  }

  if ((start_index > (int)max_events) || (start_index <= 0)) {
    start_index = 0;
  }

  if (end_index == -1) {
    end_index = index;
  }

  if (start_index == end_index) {
    return;
  }

  // Loop through and dump events between the start and end points requested

  do {
    std::cout << std::hex << "0x" << start_index << " = 0x" << pTimeBuffer[start_index];
    std::cout << " : " << pEventBuffer[start_index] << '\n';
    start_index = (start_index + 1) & max_events;
  } while (start_index != end_index);
}


void ThreeStateEventLogger::dump_ASCII_art(guint64 time_step,
    guint64 start_time,
    int end_index)
{
  int start_index = get_index(start_time);

  if ((start_index > (int)max_events) || (start_index <= 0)) {
    start_index = 0;
    start_time = pTimeBuffer[0];
  }

  if (pTimeBuffer[start_index] == 0) {
    start_index = 0;
    start_time = pTimeBuffer[0];
  }

  if ((end_index > (int)max_events) || (end_index <= 0)) {
    end_index = index;
  }

  if (start_index == end_index) {
    return;
  }

  // Loop through and dump events between the start and end points requested
  guint64 min_pulse = pTimeBuffer[end_index] - pTimeBuffer[start_index];
  unsigned long i = start_index;
  int j = (start_index + 1) & max_events;

  do {
    if ((pTimeBuffer[j] - pTimeBuffer[i]) < min_pulse) {
      min_pulse = (pTimeBuffer[j] - pTimeBuffer[i]);
    }

    i = j;
    j = (j + 1) & max_events;
  } while (j != end_index);

  std::cout << "minimum pulse width :" << min_pulse << '\n';

  if (min_pulse == 0) { // bummer - there's an error in the log
    min_pulse = 1;
    std::cout << "log error - minimum pulse width shouldn't be zero\n";
  }

  time_step = 0;
  time_step = time_step ? time_step : ((min_pulse > 2) ? min_pulse / 2 : 1);
  int num_chars = 0;
  guint64 t = start_time; //buffer[start_index];
  guint64 stop_time = gcycles->get();
  i = start_index;

  do {
    if (t <= pTimeBuffer[end_index]) {
      j = get_index(t);

    } else {
      j = end_index;
    }

    std::cout << pEventBuffer[j];
    /*
    switch(j-i) {
    case 0:
    case 1:
      if(i&1)
        cout <<'-';
      else
        cout <<'_';
      break;
    case 2:
      cout << '|';
      break;
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
      cout << (j-i);
      break;
    default:
      cout << '*';
    }
    */
    //i = j;
    t += time_step;
  } while (t < stop_time && num_chars++ < 1000);

  std::cout << '\n';
  //cout << "\nend of ASCII art\n";
}
