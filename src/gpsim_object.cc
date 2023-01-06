
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


#include <typeinfo>
#include <stdio.h>
#include <ctype.h>
#include <cstring>

#include "../config.h"

#include "gpsim_object.h"

//========================================================================

gpsimObject::gpsimObject()
  : cpDescription(nullptr)
{
}

gpsimObject::gpsimObject(const char *_name, const char *desc)
  : cpDescription(desc)
{
  if (_name)
    name_str = _name;
}

gpsimObject::~gpsimObject()
{
}

// The 'type' of any Viewable object is equivalent to the class name.
std::string gpsimObject::showType()
{
  const char* name = typeid(*this).name();

  /* Unfortunately, the class name string returned by typeid() is
   * implementation-specific.  If a particular compiler produces
   * ugly output, this is your chance to clean it up. */
#if defined __GNUC__
  /* GNU C++ puts the length of the class name in front of the
     actual class name.  We will skip over it for clarity. */
  if (*name == 'N') // Class names with format N nn name nn name E
  {
    char buf[256];
    int cnt;

    name++;
    buf[0] = 0;

    while (isdigit(*name)) {
      for (cnt = 0; isdigit(*name); name++)
        cnt = cnt * 10 + *name - '0';
      strncat(buf, name, cnt);
      name += cnt;
      if (isdigit(*name))
        strcat(buf, "::");
    }
    name = buf;
  }
  else  // just nn name
  {
      while (isdigit(*name))
        name++;
  }
#elif defined _MSC_VER
  /*
  From Visual C++ on line documentation
  The type_info::name member function returns a const char* to
  a null-terminated string representing the human-readable name
  of the type. The memory pointed to is cached and should never
  be directly deallocated.
  */
  // Skip over the word 'class '.
  name += 6;
#else
  #warning --->You might want to clean up the result of typeid() here...
#endif

  return name;
}

std::string gpsimObject::show()
{
  return showType() + ":" + toString();
}

void gpsimObject::new_name(const char *s)
{
  if (s)
    name_str = s;
}

// FIXME: The argument should be const std::string &
void gpsimObject::new_name(std::string &new_name)
{
  name_str = new_name;
}

char *gpsimObject::name(char *return_str, int len)
{
  if (return_str)
    snprintf(return_str, len, "%s", name_str.c_str());

  return return_str;
}

/// TEMPORARY -- remove after gpsimValue and Value have been merged.
unsigned int gpsimObject::get_value()
{
  return 0;
}

char *gpsimObject::toString(char *return_str, int len)
{
  if (return_str)
    snprintf(return_str, len, "%s", toString().c_str());

  return return_str;
}

char *gpsimObject::toBitStr(char *return_str, int )
{
  if (return_str)
    *return_str = '\0';

  return return_str;
}

// FIXME: Should not cast away constness
std::string &gpsimObject::name() const
{
  return (std::string &)name_str;
}

std::string gpsimObject::toString()
{
  //return showType();

  char buff[64];
  snprintf(buff, sizeof(buff), " = 0x%x", get_value());
  std::string s = name() + std::string(buff);
  return s;
}

// If derived classes don't redefine set_break and clear_break then
// the default behavior is to not support these capabilities.
int gpsimObject::set_break(ObjectBreakTypes , ObjectActionTypes , Expression *)
{
  //cout << showType() << " objects do not support break points\n";
  return -1;
}

int gpsimObject::clear_break()
{
  //cout << showType() << " objects do not support break points\n";
  return -1;
}

std::string gpsimObject::description()
{
  if (cpDescription)
    return cpDescription;
  else
    return "no description";
}

void  gpsimObject::set_description(const char *new_description)
{
  cpDescription = new_description;
}
