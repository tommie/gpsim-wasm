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

class Processor;
class Module;

class Expression;
class ComparisonOperator;
class Packet;

//------------------------------------------------------------------------
//
/// Value - the base class that supports types
///
/// Everything that can hold a value is derived from the Value class.
/// The primary purpose of this is to provide external objects (like
/// the gui) an abstract way of getting the value of diverse things
/// like registers, program counters, cycle counters, etc.
///
/// In addition, expressions of Values can be created and operated
/// on.


class Value : public gpsimObject
{
public:
  Value();
  Value(const char *name, const char *desc, Module *pM = nullptr);
  virtual ~Value();

  virtual unsigned int get_leftVal() {return 0;}
  virtual unsigned int get_rightVal() {return 0;}

  /// Value 'set' methods provide a mechanism for casting values to the
  /// the type of this value. If the type cast is not supported in a
  /// derived class, an Error will be thrown.

  virtual void set(const char *cP, int len = 0);
  virtual void set(double);
  virtual void set(int64_t);
  virtual void set(int);
  virtual void set(bool);
  virtual void set(Value *);
  virtual void set(Expression *);
  virtual void set(Packet &);

  /// Value 'get' methods provide a mechanism of casting Value objects
  /// to other value types. If the type cast is not supported in a
  /// derived class, an Error will be thrown.

  virtual void get_as(bool &b);
  virtual void get_as(int &);
  virtual void get_as(uint64_t &);
  virtual void get_as(int64_t &);
  virtual void get_as(double &);
  virtual void get_as(char *, int len);
  virtual void get_as(Packet &);

  inline operator int64_t() {
    int64_t i;
    get_as(i);
    return i;
  }

  inline operator int() {
    int64_t i;
    get_as(i);
    return (int)i;
  }

  inline operator unsigned int() {
    int64_t i;
    get_as(i);
    return (unsigned int)i;
  }

  inline Value & operator =(int i) {
    set(i);
    return *this;
  }

  inline Value & operator =(unsigned int i) {
    set((int)i);
    return *this;
  }

  /// compare - this method will compare another object to this
  /// object. It takes a pointer to a ComparisonOperator as its
  /// input. Of the object's are mismatched for the particular
  /// operator, a 'Type Mismatch' Error will be thown.

  virtual bool compare(ComparisonOperator *compOp, Value *rvalue);

  /// copy - return an object that is identical to this one.

  virtual Value *copy();

  virtual Value* evaluate() { return copy(); }

  virtual void set_module(Module *new_cpu);
  Module *get_module();
  virtual void set_cpu(Processor *new_cpu);
  Processor *get_cpu() const;
  //virtual string toString();

  void addName(const std::string &r_sAliasedName);

protected:
  // A pointer to the module that owns this value.
  Module *cpu;
  // Aliased names for this Value
  std::list<std::string> m_aka;
};


/*****************************************************************
 ValueWrapper
 */
class ValueWrapper : public Value
{
public:
  explicit ValueWrapper(Value *pCopy);
  virtual ~ValueWrapper();

  unsigned int get_leftVal() override;
  unsigned int get_rightVal() override;
  void set(const char *cP, int len = 0) override;
  void set(double) override;
  void set(int64_t) override;
  void set(int) override;
  void set(bool) override;
  void set(Value *) override;
  void set(Expression *) override;
  void set(Packet &) override;

  void get_as(bool &b) override;
  void get_as(int &) override;
  void get_as(uint64_t &) override;
  void get_as(int64_t &) override;
  void get_as(double &) override;
  void get_as(char *, int len) override;
  void get_as(Packet &) override;
  Value *copy() override;
  Value* evaluate() override;
  bool compare(ComparisonOperator *compOp, Value *rvalue) override;

private:
  Value *m_pVal;
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

  static bool Parse(const char *pValue, bool &bValue);
  static Boolean * NewObject(const char *_name, const char *pValue, const char *desc);
  virtual ~Boolean();

  std::string toString() override;
  std::string toString(const char* format);
  static std::string toString(bool value);
  static std::string toString(const char* format, bool value);

  void get_as(bool &b) override;
  void get_as(int &i) override;
  void get_as(char *, int len) override;
  void get_as(Packet &) override;

  void set(bool) override;
  void set(Value *) override;
  void set(const char *cP, int len = 0) override;
  void set(Packet &) override;

  bool getVal() { return value; }

  static Boolean* typeCheck(Value* val, std::string valDesc);
  bool compare(ComparisonOperator *compOp, Value *rvalue) override;

  Value *copy() override;

  /// copy the object value to a user char array
  char *toString(char *return_str, int len) override;
  char *toBitStr(char *return_str, int len) override;

  inline operator bool() {
    bool bValue;
    get_as(bValue);
    return bValue;
  }

  inline Boolean &operator = (bool bValue) {
    set(bValue);
    return *this;
  }

private:
  bool value;
};

inline bool operator!=(Boolean &LValue, Boolean &RValue) {
  return (bool)LValue != (bool)RValue;
}


//------------------------------------------------------------------------
/// Integer - built in gpsim type for a 64-bit integer.

class Integer : public Value {
public:
  Integer(const Integer &new_value);
  explicit Integer(int64_t new_value);
  Integer(const char *_name, int64_t new_value, const char *desc = nullptr);

  static bool       Parse(const char *pValue, int64_t &iValue);
  static Integer *  NewObject(const char *_name, const char *pValue, const char *desc);

  virtual ~Integer();

  std::string toString() override;
  std::string toString(const char* format);
  static std::string toString(int64_t value);
  static std::string toString(const char* format, int64_t value);

  void get_as(int64_t &i) override;
  void get_as(double &d) override;
  void get_as(char *, int len) override;
  void get_as(Packet &) override;

  void set(int64_t v) override;
  void set(int) override;
  void set(double d) override;
  void set(Value *) override;
  void set(const char *cP, int len = 0) override;
  void set(Packet &) override;

  static void setDefaultBitmask(int64_t bitmask);

  inline void setBitmask(int64_t bitmask) {
    this->bitmask = bitmask;
  }

  inline int64_t getBitmask() {
    return bitmask;
  }

  int64_t getVal() { return value; }

  int set_break(ObjectBreakTypes bt = eBreakAny,
                ObjectActionTypes at = eActionHalt,
                Expression *expr = nullptr) override;

  Value *copy() override;
  /// copy the object value to a user char array
  char *toString(char *, int len) override;
  char *toBitStr(char *, int len) override;

  static Integer* typeCheck(Value* val, std::string valDesc);
  static Integer* assertValid(Value* val, std::string valDesc, int64_t valMin);
  static Integer* assertValid(Value* val, std::string valDesc, int64_t valMin, int64_t valMax);
  bool compare(ComparisonOperator *compOp, Value *rvalue) override;

  inline operator int64_t() {
    int64_t i;
    get_as(i);
    return i;
  }

  inline operator uint64_t() {
    int64_t i;
    get_as(i);
    return (uint64_t)i;
  }

  inline operator bool() {
    int64_t i;
    get_as(i);
    return i != 0;
  }

  inline operator int() {
    int64_t i;
    get_as(i);
    return (int)i;
  }

  inline operator unsigned int() {
    int64_t i;
    get_as(i);
    return (unsigned int)i;
  }

  inline Integer & operator =(const Integer &i) {
    Integer & ii = (Integer &)i;
    int64_t iNew = (int64_t)ii;
    set(iNew);
    bitmask = i.bitmask;
    return *this;
  }

  inline Integer & operator =(int i) {
    set(i);
    return *this;
  }

  inline Integer & operator =(unsigned int i) {
    set((int)i);
    return *this;
  }

  inline Integer & operator &=(int iValue) {
    int64_t i;
    get_as(i);
    set((int)i & iValue);
    return *this;
  }

  inline Integer & operator |=(int iValue) {
    int64_t i;
    get_as(i);
    set((int)i | iValue);
    return *this;
  }

  inline Integer & operator +=(int iValue) {
    int64_t i;
    get_as(i);
    set((int)i + iValue);
    return *this;
  }

  inline Integer & operator ++(int) {
    int64_t i;
    get_as(i);
    set((int)i + 1);
    return *this;
  }

  inline Integer & operator --(int) {
    int64_t i;
    get_as(i);
    set((int)i - 1);
    return *this;
  }

  inline Integer & operator <<(int iShift) {
    int64_t i;
    get_as(i);
    set(i << iShift);
    return *this;
  }

  inline bool operator !() {
    int64_t i;
    get_as(i);
    return i == 0;
  }

private:
  int64_t value;
  // Used for display purposes
  int64_t bitmask;
  static int64_t def_bitmask;
};

inline bool operator!=(Integer &iLValue, Integer &iRValue) {
  return (int64_t)iLValue != (int64_t)iRValue;
}

//------------------------------------------------------------------------
/// Float - built in gpsim type for a 'double'

class Float : public Value {
public:
  explicit Float(double newValue = 0.0);
  Float(const char *_name, double newValue, const char *desc = nullptr);

  static bool Parse(const char *pValue, double &fValue);
  static Float * NewObject(const char *_name, const char *pValue, const char *desc);
  virtual ~Float();

  std::string toString() override;
  std::string toString(const char* format);
  static std::string toString(double value);
  static std::string toString(const char* format, double value);

  void get_as(int64_t &i) override;
  void get_as(double &d) override;
  void get_as(char *, int len) override;
  void get_as(Packet &) override;

  void set(int64_t v) override;
  void set(double d) override;
  void set(Value *) override;
  void set(const char *cP, int len = 0) override;
  void set(Packet &) override;

  double getVal() { return value; }

  Value *copy() override;
  /// copy the object value to a user char array
  char *toString(char *, int len) override;

  static Float* typeCheck(Value* val, std::string valDesc);
  bool compare(ComparisonOperator *compOp, Value *rvalue) override;

  inline operator double() {
    double d;
    get_as(d);
    return d;
  }

  inline Float & operator = (double d) {
    set((double)d);
    return *this;
  }

  inline Float & operator = (int d) {
    set((double)d);
    return *this;
  }

  inline Float & operator += (Float &d) {
    set((double)*this + (double)d );
    return *this;
  }

  inline Float & operator *= (Float &d) {
    set((double)*this * (double)d );
    return *this;
  }

  inline Float & operator *= (double d) {
    set((double)*this * d );
    return *this;
  }

private:
  double value;
};

inline bool operator!=(Float &iLValue, Float &iRValue) {
  return (double)iLValue != (double)iRValue;
}


/*****************************************************************/
class String : public Value {
public:
  explicit String(const char *newValue);
  String(const char *newValue, size_t len);
  String(const char *_name, const char *newValue, const char *desc = nullptr);
  virtual ~String();

  std::string toString() override;

  const char *getVal();

  void set(Value *) override;
  void set(const char *cP, int len = 0) override;
  void set(Packet &) override;

  void get_as(char *, int len) override;
  void get_as(Packet &) override;

  Value *copy() override;
  /// copy the object value to a user char array
  char *toString(char *, int len) override;

  inline operator const char *() {
    return getVal();
  }

private:
  std::string value;
};

inline bool operator!=(String &LValue, String &RValue) {
  return strcmp((const char *)LValue, (const char *)RValue) != 0;
}


/*****************************************************************/

class AbstractRange : public Value {
public:
  AbstractRange(unsigned int leftVal, unsigned int rightVal);
  virtual ~AbstractRange();

  std::string toString() override;
  std::string toString(const char* format);

  unsigned int get_leftVal() override;
  unsigned int get_rightVal() override;

  void set(Value *) override;

  Value *copy() override;
  /// copy the object value to a user char array
  char *toString(char *return_str, int len) override;

  static AbstractRange* typeCheck(Value* val, std::string valDesc);
  bool compare(ComparisonOperator *compOp, Value *rvalue) override;

private:
  unsigned int left;
  unsigned int right;
};

//------------------------------------------------------------------------
// Function -- maybe should go into its own header file.
//

typedef std::list<Expression*> ExprList_t;

namespace gpsim {
  class Function : public gpsimObject {

  public:
    explicit Function(const char *_name, const char *desc = nullptr);
    virtual ~Function();

    std::string description() override;
    std::string toString() override;

    void call(ExprList_t *vargs);
  };
}

char * TrimWhiteSpaceFromString(char * pBuffer);
char * UnquoteString(char * pBuffer);
std::string &toupper(std::string & sStr);

#endif // SRC_VALUE_H_
