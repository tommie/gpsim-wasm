/*
 *    Copyright (C) 1998-2007 T. Scott Dattalo
 *    Copyright (C) 2007 Roy R Rankin
 *
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

#ifndef RCON_H
#define RCON_H

#include "registers.h"

// The methods of this class are typically called from Status_register
//---------------------------------------------------------
class RCON :  public sfr_register
{
public:

  enum
  {
    BOR  = 1 << 0,
    POR  = 1 << 1,
    PD   = 1 << 2,
    TO   = 1 << 3,
    RI   = 1 << 4,
    LWRT = 1 << 6,
    IPEN = 1 << 7
  };

  RCON(Processor *, const char *pName, const char *pDesc = nullptr);

  inline void put_PD(unsigned int new_pd)
  {
    emplace_value_trace<trace::WriteRegisterEntry>();
    value.put((value.get() & ~PD) | ((new_pd) ? PD : 0));
  }

  inline unsigned int get_PD()
  {
    emplace_value_trace<trace::ReadRegisterEntry>();
    return ((value.get() & PD) == 0) ? 0 : 1;
  }

  inline void put_TO(unsigned int new_to)
  {
    emplace_value_trace<trace::WriteRegisterEntry>();
    value.put((value.get() & ~TO) | ((new_to) ? TO : 0));
  }

  inline unsigned int get_TO()
  {
    emplace_value_trace<trace::ReadRegisterEntry>();
    return ((value.get() & TO) == 0) ? 0 : 1;
  }
};

#endif // RCON_H
