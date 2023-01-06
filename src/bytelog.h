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

#ifndef SRC_BYTELOG_h_
#define SRC_BYTELOG_h_

#include <vector>

namespace gpsim {

  //========================================================================

  struct TimedByte {
    unsigned long long start;
    unsigned long long stop;
    unsigned long long rts;
    unsigned int b;
  };

  class ByteLogger
    {
    protected:
      int index;
      int bufsize;
      std::vector<TimedByte> buffer;

    public:
      explicit ByteLogger(int bufsize = 128);

      int modIndex(int i);
      void start(unsigned long long t);
      void stop(unsigned long long t);
      void byte(unsigned int b);
      void rts(unsigned long long r);
      //  void statistics(int i=-1);
      unsigned long long getStart(int i = -1);
      void get(int i, TimedByte &b);
    };

} // end of namespace gpsim


#endif // SRC_BYTELOG_h_
