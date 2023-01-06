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


#ifndef SRC_GPSIM_OBJECT_H_
#define SRC_GPSIM_OBJECT_H_

#include <string>

class Expression;

/// gpsimObject - base class for most of gpsim's objects
///

class gpsimObject {
public:
  gpsimObject();
  explicit gpsimObject(const char *_name, const char *desc = nullptr);
  virtual ~gpsimObject();

  /// Get the name of the object
  virtual std::string &name() const;

  /// copy the name to a user char array
  virtual char *name(char *, int len);

  /// copy the object value to a user char array
  virtual char *toString(char *, int len);
  virtual char *toBitStr(char *, int len);

  /// Assign a new name to the object
  /// FIXME why have two ways of naming ???
  virtual void new_name(const char *);
  virtual void new_name(std::string &);

  /// TEMPORARY -- remove after gpsimValue and Value have been merged.
  virtual unsigned int get_value();

  /// description - get a description of this object. If the object has
  /// a name, then 'help value_name' at the command line will display
  /// the description.

  virtual std::string description();
  void set_description(const char *);

  /// Access object-specific information
  std::string show();
  std::string showType();
  virtual std::string toString();

  // Breakpoint types supported by Value
  enum ObjectBreakTypes {
    eBreakAny,    // ???
    eBreakWrite,  // Register write
    eBreakRead,   // Register read
    eBreakChange, // Register change
    eBreakExecute // Program memory execute
  };

  // Breakpoint types supported by Value
  enum ObjectActionTypes {
    eActionHalt,
    eActionLog,
  };

  /// breakpoints
  /// set a break point on a gpsim object. The BreakType specifies the
  /// the condition for which the break will trigger when this value
  /// is accessed. In addition, the optional expr is a boolean expression
  /// that is evaluated when the Object is accessed. The expression must
  /// evaluate to true for the break to trigger. If the break is successfully
  /// set then a non-negative number (the break point number) will be returned.
  /// If the break fails, then -1 is returned.
  /// The ActionType specifies the action to take when the break is triggered.

  virtual int set_break(ObjectBreakTypes bt = eBreakAny,
                        ObjectActionTypes at = eActionHalt,
                        Expression *expr = nullptr);
  virtual int clear_break();

protected:
  std::string  name_str;          // A unique name to describe the object
  const char *cpDescription;      // A desciption of the object
};

//------------------------------------------------------------------------
// BreakTypes
//
class BreakType
{
public:
  explicit BreakType(int _type)
    : m_type(_type)
  {
  }

  virtual ~BreakType()
  {
  }

  virtual int type()
  {
    return m_type;
  }
protected:
  int m_type;
};

#endif //  SRC_GPSIM_OBJECT_H_
