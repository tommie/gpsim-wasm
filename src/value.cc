/*
   Copyright (C) 1998-2003 Scott Dattalo

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

#include "value.h"

#include <ctype.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <typeinfo>

#include <config.h>
#include "errors.h"
#include "modules.h"
#include "processor.h"
#include "protocol.h"
#include "ui.h"

//------------------------------------------------------------------------
Value::Value(const char *_name, const char *desc, Module *pMod)
  : gpsimObject(_name, desc), module(pMod)
{
}

/*****************************************************************
 * The Boolean class.
 */
Boolean::Boolean(bool newValue)
  : value(newValue)
{
}

Boolean::Boolean(const char *_name, bool newValue, const char *_desc)
  : Value(_name, _desc), value(newValue)
{
}

std::string Boolean::toString()
{
  return get() ? "true" : "false";
}

/*****************************************************************
 * The Integer class.
 */
int64_t Integer::def_bitmask = 0xff;

Integer::Integer(int64_t newValue)
  : value(newValue), bitmask(def_bitmask)
{
}

Integer::Integer(const char *_name, int64_t newValue, const char *_desc)
  : Value(_name, _desc), value(newValue), bitmask(def_bitmask)
{
}

std::string Integer::toString()
{
  int64_t i = get();
  IUserInterface & TheUI = GetUserInterface();
  if (bitmask == 0xff && i > 256)
      return std::string(TheUI.FormatValue(i, 0xffff));
  else
      return std::string(TheUI.FormatValue(i, (unsigned int)bitmask));
}

/*****************************************************************
 * The Float class.
 */
Float::Float(double newValue)
  : value(newValue)
{
}

Float::Float(const char *_name, double newValue, const char *_desc)
  : Value(_name, _desc), value(newValue)
{
}

std::string Float::toString()
{
  char buf[32];
  snprintf(buf, sizeof(buf), "%#-16.16g", get());
  return buf;
}

/*****************************************************************
 * The String class.
 */
String::String(std::string_view newValue)
  : value(newValue)
{
}

String::String(const char *_name, std::string_view newValue, const char *_desc)
  : Value(_name, _desc), value(newValue)
{
}

std::string String::toString()
{
  return value;
}
