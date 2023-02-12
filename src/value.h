/*
   Copyright (C) 1998-2004 Scott Dattalo

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

#ifndef SRC_VALUE_H_
#define SRC_VALUE_H_

#include "gpsim_object.h"
#include <cstddef>
#include <cstring>
#include <list>
#include <string>

class Module;

//------------------------------------------------------------------------
//
/// Value - the base class that supports types
///
/// Everything that can hold a value is derived from the Value class.
/// The primary purpose of this is to provide external objects (like
/// the gui) an abstract way of getting the value of diverse things
/// like registers, program counters, cycle counters, etc.


class Value : public gpsimObject
{
public:
  Value() = default;
  Value(const char *name, const char *desc, Module *pM = nullptr);

  Module *get_module() const { return module; }

private:
  // A pointer to the module that owns this value.
  Module *module = nullptr;
};

/*****************************************************************
 * Now we introduce classes for the basic built-in data types.
 * These classes are created by extending the Value class.  For
 * convenience, they all must instantiate a getVal() method that
 * returns valueof the object in question as a simple value of
 * the base data type.  For example, invoking getVal() on a
 * Boolean oject must return a simple 'bool' value.
 */
/*****************************************************************/
class Boolean : public Value {
public:
  explicit Boolean(bool newValue);
  Boolean(const char *_name, bool newValue, const char *desc = nullptr);

  std::string toString() override;

  virtual void set(bool v) { value = v; }
  bool get() const { return value; }

  operator bool() const { return get(); }
  Boolean &operator =(bool bValue) { set(bValue); return *this; }

  bool operator !=(const Boolean &bValue) const {
    return get() != bValue.get();
  }

private:
  bool value;
};


//------------------------------------------------------------------------
/// Integer - built in gpsim type for a 64-bit integer.

class Integer : public Value {
public:
  explicit Integer(int64_t new_value);
  Integer(const char *_name, int64_t new_value, const char *desc = nullptr);

  inline void setBitmask(int64_t bitmask) {
    this->bitmask = bitmask;
  }

  inline int64_t getBitmask() const { return bitmask; }

  virtual void set(int64_t v) { value = v; }
  int64_t get() const { return value; }

  std::string toString() override;

  operator int64_t() const { return get(); }
  Integer& operator =(int64_t i) { set(i); return *this; }

  operator uint64_t() const { return get(); }
  operator bool() const { return get() != 0; }
  bool operator !() const { return get() == 0; }

  Integer& operator &=(int64_t iValue) { set(get() & iValue); return *this; }
  Integer& operator |=(int64_t iValue) { set(get() | iValue); return *this; }
  Integer& operator +=(int64_t iValue) { set(get() + iValue); return *this; }
  Integer& operator ++(int) { set(get() + 1); return *this; }
  Integer& operator --(int) { set(get() - 1); return *this; }

  bool operator !=(const Integer &that) const {
    return get() != that.get();
  }

private:
  int64_t value;
  int64_t bitmask;  // Used for display purposes
  static int64_t def_bitmask;
};

//------------------------------------------------------------------------
/// Float - built in gpsim type for a 'double'

class Float : public Value {
public:
  explicit Float(double newValue = 0.0);
  Float(const char *_name, double newValue, const char *desc = nullptr);

  virtual void set(double v) { value = v; }
  double get() const { return value; }

  std::string toString() override;

  operator double() const { return get(); }

  Float& operator =(double d) { set(d); return *this; }
  Float& operator += (double d) { set(get() + d); return *this; }
  Float& operator -= (double d) { set(get() - d); return *this; }
  Float& operator *= (double d) { set(get() * d); return *this; }

  bool operator !=(const Float &that) const {
    return get() != that.get();
  }

private:
  double value;
};

/*****************************************************************/
class String : public Value {
public:
  explicit String(std::string_view newValue);
  String(const char *_name, std::string_view newValue, const char *desc = nullptr);

  virtual void set(std::string_view s) { value = s; }
  std::string_view get() const { return value; }

  std::string toString() override;

  operator std::string_view() const { return get(); }

  bool operator !=(const String &that) const {
    return get() != that.get();
  }

private:
  std::string value;
};

#endif // SRC_VALUE_H_
