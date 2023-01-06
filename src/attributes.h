/*
   Copyright (C) 1998-2005 T. Scott Dattalo

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

#ifndef SRC_ATTRIBUTES_H_
#define SRC_ATTRIBUTES_H_

#include "value.h"

class Processor;

/// gpsim attributes
///

/// WarnModeAttribute
class WarnModeAttribute : public Boolean
{
protected:
  Processor *cpu;
public:
  explicit WarnModeAttribute(Processor *_cpu);
  void set(Value *v) override;
  void get(bool &b) override;
};


/// SafeModeAttribute
class SafeModeAttribute : public Boolean
{
protected:
  Processor *cpu;
public:
  explicit SafeModeAttribute(Processor *_cpu);
  ~SafeModeAttribute();
  void set(Value *v) override;
  void get(bool &b) override;
};

/// UnknownModeAttribute
class UnknownModeAttribute : public Boolean
{
protected:
  Processor *cpu;
public:
  explicit UnknownModeAttribute(Processor *_cpu);
  void set(Value *v) override;
  void get(bool &b) override;
};


/// BreakOnResetAttribute
class BreakOnResetAttribute : public Boolean
{
protected:
  Processor *cpu;
public:
  explicit BreakOnResetAttribute(Processor *_cpu);
  void set(Value *v) override;
  void get(bool &b) override;
};
#endif // SRC_ATTRIBUTES_H_

