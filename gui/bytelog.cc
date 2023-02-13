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


#include "bytelog.h"
#include <memory>

namespace gpsim {

  ByteLogger::ByteLogger(int _bufsize) 
    : index(0), bufsize(_bufsize), buffer(_bufsize)
  {
  }

  int ByteLogger::modIndex(int i)
  {
    if (i < 0) {
      i += bufsize;
      if (i < 0)
        return index;
    } else if (i >= bufsize)
      return index;

    return i;
  }


  void ByteLogger::start(unsigned long long t)
  {
    buffer[index].start = t;
  }

  void ByteLogger::stop(unsigned long long t) 
  {
    buffer[index].stop = t;

    if (++index > bufsize)
      index = 0;
  }

  void ByteLogger::byte(unsigned int b) 
  {
    buffer[index].b = b & 0xff;
  }

  void ByteLogger::rts(unsigned long long r) 
  {
    buffer[index].rts = r;
  }

  /*
    void ByteLogger::statistics(int i=-1)
    {
    i = modIndex(i+index);

    unsigned long long t = (buffer[i].stop-buffer[i].start);
    double td= t/2.0;

    cout << "0x" << hex << buffer[i].b 
    << " start time: 0x" << buffer[i].start
    << " byte time: 0x" << t << " cycles = "
    << td  << " uS\n";
    }
  */

  unsigned long long ByteLogger::getStart(int i)
  {
    return buffer[modIndex(i + index)].start;
  }

  void ByteLogger::get(int i, TimedByte &b)
  {
    b = buffer[modIndex(i + index)];
  }

} // end of namespace gpsim
