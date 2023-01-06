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

#include "xref.h"
#include "gpsim_interface.h"
#include "gpsim_object.h"

XrefObject::XrefObject()
{
}


XrefObject::XrefObject(gpsimObject *value)
  : data(value)
{
}


XrefObject::~XrefObject()
{
  for (auto ioi = xrefs.begin(); ioi != xrefs.end(); ++ioi)
  {
    delete (XrefObject *) *ioi;
  }
}


void *XrefObject::first_xref()
{
  if (!xrefs.empty()) {
    std::list<void *>::iterator ioi = xrefs.begin();
      return *ioi;

  } else
    return nullptr;
}


void XrefObject::_add(void *xref)
{
  xrefs.push_back(xref);
}


void XrefObject::clear(void *xref)
{
  xrefs.remove(xref);
}


void XrefObject::_update()
{
  std::list<void *>::iterator ioi = xrefs.begin();

  for ( ; ioi != xrefs.end(); ++ioi) {
    void **xref = (void **) *ioi;

    gi.update_object(xref, get_val());
  }
}


int XrefObject::get_val()
{
  if (data)
    return data->get_value();

  return 0;
}


void XrefObject::assign_data(gpsimObject *new_data)
{
  data = new_data;
}
