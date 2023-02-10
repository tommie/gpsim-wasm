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
#include "breakpoints.h"
#include "errors.h"
#include "expr.h"
#include "modules.h"
#include "operator.h"
#include "processor.h"
#include "protocol.h"
#include "ui.h"
#include "xref.h"
class Register;

char * TrimWhiteSpaceFromString(char * pBuffer)
{
  size_t iPos = 0;
  char * pChar = pBuffer;
  while (*pChar != 0 && ::isspace(*pChar)) {
    pChar++;
  }
  if (pBuffer != pChar) {
    memmove(pBuffer, pChar, strlen(pBuffer) - iPos);
  }
  iPos = strlen(pBuffer);
  if (iPos > 0) {
    pChar = pBuffer + iPos - 1;
    while (pBuffer != pChar && ::isspace(*pChar)) {
      *pChar = 0;
      pChar--;
    }
  }
  return pBuffer;
}

char * UnquoteString(char * pBuffer)
{
  char cQuote;
  if (*pBuffer == '\'') {
    cQuote = '\'';
  }
  else if (*pBuffer == '"') {
    cQuote = '"';
  }
  else {
    return pBuffer;
  }
  int iLen = strlen(pBuffer);
  if (iLen > 1) {
    if (pBuffer[iLen - 1] == cQuote) {
      memmove(&pBuffer[0], &pBuffer[1], iLen - 2);
      pBuffer[iLen - 2] = 0;
    }
  }
  return pBuffer;
}

std::string &toupper(std::string & sStr)
{
  std::string::iterator it = sStr.begin();
  std::string::iterator itEnd = sStr.end();

  for ( ; it != itEnd; ++it) {
    if (isalpha(*it)) {
      *it = toupper((int)*it);
    }
  }
  return sStr;
}

//------------------------------------------------------------------------
Value::Value()
  : _xref(nullptr), cpu(nullptr)
{
}

Value::Value(const char *_name, const char *desc, Module *pMod)
  : gpsimObject(_name, desc), _xref(nullptr), cpu(pMod)
{
}

Value::~Value()
{
  // Remove references of this Value from the symbol table:
  if (cpu) {
    //cout << "Deleting value named:" << name_str <<  " addr "<< this << endl;
    cpu->removeSymbol(name_str);

    //cout << "m_aka ==" << m_aka << endl;
    for (const auto &it : m_aka) {
      cpu->removeSymbol(it);
    }
  }
  delete _xref;
}

void Value::update()
{
  if (_xref)
      _xref->_update();
}

void Value::add_xref(void *an_xref)
{
  if (!_xref)
     _xref = new XrefObject();
  _xref->_add(an_xref);
}

void Value::remove_xref(void *an_xref)
{
  _xref->clear(an_xref);
}

void Value::set(const char *, int )
{
  throw Error(" cannot assign string to a " + showType());
}

void Value::set(double)
{
  throw Error(" cannot assign a double to a " + showType());
}

void Value::set(int64_t)
{
  throw Error(" cannot assign an integer to a " + showType());
}

void Value::set(bool)
{
  throw Error(" cannot assign a boolean to a " + showType());
}

void Value::set(int i)
{
  int64_t i64 = i;
  set(i64);
}

void Value::set(Value *)
{
  throw Error(" cannot assign a Value to a " + showType());
}

void Value::set(Expression *expr)
{
  try {
    Value *v = nullptr;

    if (!expr)
      throw Error(" null expression ");
    if (verbose)
      std::cout << toString() << " is being assigned expression " << expr->toString() << '\n';
    v = expr->evaluate();
    if (!v)
      throw Error(" cannot evaluate expression ");

    set(v);
    delete v;
  }

  catch (const Error &err) {
    std::cout << "ERROR:" << err.what() << '\n';
  }
}

void Value::set(Packet &)
{
  std::cout << "Value," << name() << " is ignoring packet buffer for set()\n";
}

void Value::get_as(int64_t &)
{
  throw Error(showType() + " cannot be converted to an integer ");
}

void Value::get_as(int &i)
{
  int64_t i64;
  get_as(i64);
  i = (int) i64;
}

void Value::get_as(uint64_t &i)
{
  // FIXME - casting a signed int to an unsigned int -- probably should issue a warning
  int64_t i64;
  get_as(i64);
  i = (int64_t) i64;
}

void Value::get_as(bool &)
{
  throw Error(showType() + " cannot be converted to a boolean");
}

void Value::get_as(double &)
{
  throw Error(showType() + " cannot be converted to a double ");
}

// get as a string - no error is thrown if the derived class
// does not provide a method for converting to a string -
// instead we'll return a bogus value.

void Value::get_as(char *buffer, int buf_size)
{
  if (buffer && buf_size > 0) {
    snprintf(buffer, buf_size, "INVALID");
  }
}

void Value::get_as(Packet &)
{
  std::cout << "Value," << name() << " is ignoring packet buffer for get_as()\n";
}

bool Value::compare(ComparisonOperator *compOp, Value *)
{
  throw Error(compOp->showOp() +
                  " comparison is not defined for " + showType());
}

Value *Value::copy()
{
  throw Error(" cannot copy " + showType());
}

/*
void Value::set_xref(Value *v)
{
  delete xref;
  xref = v;
}
Value *Value::get_xref()
{
  return xref;
}
*/

Processor *Value::get_cpu() const
{
  return static_cast<Processor *>(cpu);
}

void Value::set_cpu(Processor *new_cpu)
{
  cpu = new_cpu;
}

void Value::set_module(Module *new_cpu)
{
  cpu = new_cpu;
}

Module *Value::get_module()
{
  return cpu;
}

void Value::addName(const std::string &r_sAliasedName)
{
  m_aka.push_back(r_sAliasedName);
}

//------------------------------------------------------------------------
ValueWrapper::ValueWrapper(Value *pCopy)
  : m_pVal(pCopy)
{
}

ValueWrapper::~ValueWrapper()
{
}

unsigned int ValueWrapper::get_leftVal()
{
  return m_pVal->get_leftVal();
}

unsigned int ValueWrapper::get_rightVal()
{
  return m_pVal->get_rightVal();
}

void ValueWrapper::set(const char *cP, int len)
{
  m_pVal->set(cP,len);
}

void ValueWrapper::set(double d)
{
  m_pVal->set(d);
}

void ValueWrapper::set(int64_t i)
{
  m_pVal->set(i);
}

void ValueWrapper::set(int i)
{
  m_pVal->set(i);
}

void ValueWrapper::set(bool b)
{
  m_pVal->set(b);
}

void ValueWrapper::set(Value *v)
{
  m_pVal->set(v);
}

void ValueWrapper::set(Expression *e)
{
  m_pVal->set(e);
}

void ValueWrapper::set(Packet &p)
{
  m_pVal->set(p);
}

void ValueWrapper::get_as(bool &b)
{
  m_pVal->get_as(b);
}

void ValueWrapper::get_as(int &i)
{
  m_pVal->get_as(i);
}

void ValueWrapper::get_as(uint64_t &i)
{
  m_pVal->get_as(i);
}

void ValueWrapper::get_as(int64_t &i)
{
  m_pVal->get_as(i);
}

void ValueWrapper::get_as(double &d)
{
  m_pVal->get_as(d);
}

void ValueWrapper::get_as(char *pC, int len)
{
  m_pVal->get_as(pC, len);
}

void ValueWrapper::get_as(Packet &p)
{
  m_pVal->get_as(p);
}

Value *ValueWrapper::copy()
{
  return m_pVal->copy();
}

void ValueWrapper::update()
{
  m_pVal->update();
}

Value *ValueWrapper::evaluate()
{
  return m_pVal->evaluate();
}

bool ValueWrapper::compare(ComparisonOperator *compOp, Value *rvalue)
{
  if (!compOp || !rvalue)
    return false;

  int64_t i,r;

  m_pVal->get_as(i);
  rvalue->get_as(r);

  if (i < r)
    return compOp->less();

  if (i > r)
    return compOp->greater();

  return compOp->equal();
}

/*****************************************************************
 * The AbstractRange class.
 */
AbstractRange::AbstractRange(unsigned int newLeft, unsigned int newRight)
  : left(newLeft), right(newRight)
{
}

AbstractRange::~AbstractRange()
{
}

std::string AbstractRange::toString()
{
  char buff[256];

  snprintf(buff, sizeof(buff), "%u:%u", left, right);

  return buff;
}

std::string AbstractRange::toString(const char* format)
{
  char cvtBuf[1024];

  snprintf(cvtBuf, sizeof(cvtBuf), format, left, right);
  return cvtBuf;
}

char *AbstractRange::toString(char *return_str, int len)
{
  if (return_str) {
    snprintf(return_str, len, "%u:%u", left, right);
  }

  return return_str;
}

unsigned int AbstractRange::get_leftVal()
{
  return left;
}

unsigned int AbstractRange::get_rightVal()
{
  return right;
}

AbstractRange* AbstractRange::typeCheck(Value* val, std::string valDesc)
{
  if (typeid(*val) != typeid(AbstractRange)) {
    throw TypeMismatch(valDesc, "AbstractRange", val->showType());
  }
  // This static cast is totally safe in light of our typecheck, above.
  return static_cast<AbstractRange *>(val);
}

bool AbstractRange::compare(ComparisonOperator *compOp, Value *)
{
  throw Error(compOp->showOp() +
                  " comparison is not defined for " + showType());
}

Value *AbstractRange::copy()
{
  return new AbstractRange(get_leftVal(), get_rightVal());
}

void AbstractRange::set(Value *v)
{
  AbstractRange *ar = typeCheck(v, "");
  left = ar->get_leftVal();
  right = ar->get_rightVal();
}

/*
bool AbstractRange::operator<(Value *rv)
{
  AbstractRange *_rv = typeCheck(rv,"OpLT");
  return right < _rv->left;
}
*/

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

bool Boolean::Parse(const char *pValue, bool &bValue)
{
  if (strncmp("true", pValue, sizeof("true") - 1) == 0) {
    bValue = true;
    return true;
  }
  else if (strncmp("false", pValue, sizeof("false") - 1) == 0) {
    bValue = false;
    return true;
  }
  return false;
}

Boolean * Boolean::NewObject(const char *_name, const char *pValue, const char *)
{
  bool bValue;
  if (Parse(pValue, bValue)) {
    return new Boolean(_name, bValue);
  }
  return nullptr;
}

Boolean::~Boolean()
{
}

std::string Boolean::toString()
{
  bool b;
  get_as(b);
  return b ? "true" : "false";
}

std::string Boolean::toString(bool value)
{
  return value ? "true" : "false";
}

char *Boolean::toString(char *return_str, int len)
{
  if (return_str) {
    bool b;
    get_as(b);
    snprintf(return_str, len, "%s", (b ? "true" : "false"));
  }

  return return_str;
}

char *Boolean::toBitStr(char *return_str, int len)
{
  if (return_str) {
    bool b;
    get_as(b);
    snprintf(return_str, len, "%d", (b ? 1 : 0));
  }

  return return_str;
}

std::string Boolean::toString(const char* format)
{
  char cvtBuf[1024];
  bool b;
  get_as(b);

  snprintf(cvtBuf, sizeof(cvtBuf), format, b);
  return cvtBuf;
}

Boolean* Boolean::typeCheck(Value* val, std::string valDesc)
{
  if (typeid(*val) != typeid(Boolean)) {
    throw TypeMismatch(valDesc, "Boolean", val->showType());
  }

  // This static cast is totally safe in light of our typecheck, above.
  return static_cast<Boolean *>(val);
}

bool Boolean::compare(ComparisonOperator *compOp, Value *rvalue)
{
  Boolean *rv = typeCheck(rvalue, "");

  switch (compOp->isa()) {
  case ComparisonOperator::eOpEq:
    return value == rv->value;
  case ComparisonOperator::eOpNe:
    return value != rv->value;
  default:
    Value::compare(compOp, rvalue);  // error
  }

  return false; // keep the compiler happy.
}

Value *Boolean::copy()
{
  bool b;
  get_as(b);
  return new Boolean(b);
}

// get_as(bool&) - primary method for accessing the value.
void Boolean::get_as(bool &b)
{
  b = value;
}

// get_as(int&) - type cast an integer into a boolean. Note
// that we call get_as(bool &) instead of directly accessing
// the member value. The reason for this is so that derived
// classes can capture the access.
void Boolean::get_as(int &i)
{
  bool b;
  get_as(b);
  i = b ? 1 : 0;
}

/*
void Boolean::get_as(double &d)
{
  bool b;
  get_as(b);
  d = b ? 1.0 : 0.0;
}
*/
void Boolean::get_as(char *buffer, int buf_size)
{
  if (buffer) {
    bool b;
    get_as(b);
    snprintf(buffer, buf_size, (b ? "true" : "false"));
  }
}

void Boolean::get_as(Packet &pb)
{
  bool b;
  get_as(b);
  pb.EncodeBool(b);
}

void Boolean::set(Value *v)
{
  Boolean *bv = typeCheck(v, "set ");
  bool b = bv->getVal();
  set(b);
}

void Boolean::set(bool v)
{
  value = v;
  //if(get_xref())
  //  get_xref()->set(v);
}

void Boolean::set(const char *buffer, int )
{
  if (buffer) {
    bool bValue;
    if (Parse(buffer, bValue)) {
      set(bValue);
    }
  }
}

void Boolean::set(Packet &p)
{
  bool b;
  if (p.DecodeBool(b))
    set(b);
}

/*
bool Boolean::operator&&(Value *rv)
{
  Boolean *_rv = typeCheck(rv,"Op&&");
  return value && _rv->value;
}

bool Boolean::operator||(Value *rv)
{
  Boolean *_rv = typeCheck(rv,"Op||");
  return value || _rv->value;
}

bool Boolean::operator==(Value *rv)
{
  Boolean *_rv = typeCheck(rv,"OpEq");
  return value == _rv->value;
}

bool Boolean::operator!=(Value *rv)
{
  Boolean *_rv = typeCheck(rv,"OpNe");
  return value != _rv->value;
}
*/

/*****************************************************************
 * The Integer class.
 */
Integer::Integer(const Integer &new_value)
{
  Integer & nv = (Integer&)new_value;
  nv.get_as(value);
  bitmask = new_value.bitmask;
}

Integer::Integer(int64_t newValue)
  : value(newValue), bitmask(def_bitmask)
{
}

Integer::Integer(const char *_name, int64_t newValue, const char *_desc)
  : Value(_name, _desc), value(newValue), bitmask(def_bitmask)
{
}

int64_t Integer::def_bitmask = 0xffffffff;

Integer::~Integer()
{
}

void Integer::setDefaultBitmask(int64_t bitmask)
{
  def_bitmask = bitmask;
}

Value *Integer::copy()
{
  int64_t i;
  get_as(i);
  return new Integer(i);
}

void Integer::set(double d)
{
  int64_t i = (int64_t)d;
  set(i);
}

void Integer::set(int64_t i)
{
  value = i;
  //if(get_xref())
  //  get_xref()->set(i);
}

void Integer::set(int i)
{
  int64_t ii = i;
  set(ii);
}

void Integer::set(Value *v)
{
  int64_t iv = 0;
  if (v)
    v->get_as(iv);

  set(iv);
}

void Integer::set(Packet &p)
{
  unsigned int i;
  if (p.DecodeUInt32(i)) {

    set((int)i);
    return;
  }

  uint64_t i64;
  if (p.DecodeUInt64(i64)) {

    set((int64_t)i64);
    return;
  }
}

void Integer::set(const char *buffer, int )
{
  if (buffer) {
    int64_t i;
    if (Parse(buffer, i)) {
      set(i);
    }
  }
}

bool Integer::Parse(const char *pValue, int64_t &iValue)
{
  if (::isdigit(*pValue)) {
    if (strchr(pValue, '.')) {
      return false;
    }
    else {
      // decimal or 0x integer
      return sscanf(pValue, "%" PRINTF_GINT64_MODIFIER "i", &iValue) == 1;
    }
  }
  else if (*pValue == '$' && ::isxdigit(*(pValue + 1))) {
    // hexidecimal integer
    char szHex[10] = "0x";
    strcat(szHex, pValue + 1);
    return sscanf(szHex, "%"  PRINTF_GINT64_MODIFIER "i" , &iValue) == 1;
  }
  return false;
}

Integer * Integer::NewObject(const char *_name, const char *pValue, const char *desc)
{
  int64_t iValue;
  if (Parse(pValue, iValue)) {
    return new Integer(_name, iValue, desc);
  }
  return nullptr;
}

void Integer::get_as(int64_t &i)
{
  i = value;
}

void Integer::get_as(double &d)
{
  int64_t i;
  get_as(i);
  d = (double)i;
}

void Integer::get_as(char *buffer, int buf_size)
{
  if (buffer) {

    int64_t i;
    get_as(i);
    long long int j = i;
    snprintf(buffer, buf_size, "%" PRINTF_INT64_MODIFIER "d", j);
  }
}

void Integer::get_as(Packet &pb)
{
  int64_t i;
  get_as(i);

  unsigned int j = (unsigned int) (i & 0xffffffff);
  pb.EncodeUInt32(j);
}

int Integer::set_break(ObjectBreakTypes bt, ObjectActionTypes at, Expression *expr)
{
  Processor *pCpu = get_active_cpu();
  if (pCpu) {

    // Legacy code compatibility!

    if ( bt == eBreakWrite || bt == eBreakRead ) {

      // Cast the integer into a register and set a register break point
      unsigned int iRegAddress = (unsigned int) value;
      Register *pReg = &pCpu->rma[iRegAddress];
      return get_bp().set_break(bt, at, pReg, expr);
    } else if ( bt == eBreakExecute) {

      unsigned int iProgAddress = (unsigned int) value;
      return get_bp().set_execution_break(pCpu, iProgAddress, expr);
    }
  }

  return -1;
}

std::string Integer::toString()
{
  int64_t i;
  get_as(i);
  IUserInterface & TheUI = GetUserInterface();
  if (bitmask == 0xff && i > 256)
      return std::string(TheUI.FormatValue(i, 0xffff));
  else
      return std::string(TheUI.FormatValue(i, (unsigned int)bitmask));
}

std::string Integer::toString(const char* format)
{
  char cvtBuf[1024];

  int64_t i;
  get_as(i);

  snprintf(cvtBuf, sizeof(cvtBuf), format, i);
  return cvtBuf;
}

std::string Integer::toString(const char* format, int64_t value)
{
  char cvtBuf[1024];

  snprintf(cvtBuf, sizeof(cvtBuf), format, value);
  return cvtBuf;
}

std::string Integer::toString(int64_t value)
{
  char cvtBuf[1024];
  long long int v = value;
  snprintf(cvtBuf, sizeof(cvtBuf), "%" PRINTF_INT64_MODIFIER "d", v);
  return cvtBuf;
}

char *Integer::toString(char *return_str, int len)
{
  if (return_str) {
    int64_t i;
    get_as(i);
    IUserInterface & TheUI = GetUserInterface();
    strncpy(return_str, TheUI.FormatValue(i), len);
//    snprintf(return_str,len,"%" PRINTF_INT64_MODIFIER "d",i);
  }

  return return_str;
}

char *Integer::toBitStr(char *return_str, int len)
{
  if (return_str) {
    int64_t i;
    get_as(i);
    int j = 0;
    int64_t mask = 1UL << 31;
    for ( ; mask ; mask >>= 1, j++)
      if (j < len)
        return_str[j] = ((i & mask) ? 1 : 0);

    if (j < len)
      return_str[j] = 0;
  }

  return return_str;
}

Integer* Integer::typeCheck(Value* val, std::string valDesc)
{
  if (typeid(*val) != typeid(Integer)) {
    throw TypeMismatch(valDesc, "Integer", val->showType());
  }

  // This static cast is totally safe in light of our typecheck, above.
  return static_cast<Integer *>(val);
}

Integer* Integer::assertValid(Value* val, std::string valDesc, int64_t valMin)
{
  Integer* iVal;
  int64_t i;

  iVal = Integer::typeCheck(val, valDesc);
  iVal->get_as(i);

  if (i < valMin) {
    throw Error(valDesc +
                    " must be greater than " + Integer::toString(valMin) +
                    ", saw " + Integer::toString(i)
                    );
  }

  return iVal;
}

Integer* Integer::assertValid(Value* val, std::string valDesc, int64_t valMin, int64_t valMax)
{
  Integer* iVal;
  int64_t i;

  iVal = (Integer::typeCheck(val, valDesc));

  iVal->get_as(i);

  if ((i < valMin) || (i>valMax)) {
    throw Error(valDesc +
                    " must be be in the range [" + Integer::toString(valMin) + ".." +
                    Integer::toString(valMax) + "], saw " + Integer::toString(i)
                    );
  }

  return iVal;
}

bool Integer::compare(ComparisonOperator *compOp, Value *rvalue)
{
  Integer *rv = typeCheck(rvalue,"");

  int64_t i, r;

  get_as(i);
  rv->get_as(r);


  if (i < r)
    return compOp->less();

  if (i > r)
    return compOp->greater();

  return compOp->equal();
}

/*
bool Integer::operator<(Value *rv)
{
  Integer *_rv = typeCheck(rv,"OpLT");
  return value < _rv->value;
}

bool Integer::operator>(Value *rv)
{
  Integer *_rv = typeCheck(rv,"OpGT");
  return value > _rv->value;
}

bool Integer::operator<=(Value *rv)
{
  Integer *_rv = typeCheck(rv,"OpLE");
  return value <= _rv->value;
}

bool Integer::operator>(Value *rv)
{
  Integer *_rv = typeCheck(rv,"OpGT");
  return value > _rv->value;
}
*/

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

bool Float::Parse(const char *pValue, double &fValue)
{
  return pValue ? sscanf(pValue, "%lg", &fValue) == 1 : false;
}

Float * Float::NewObject(const char *_name, const char *pValue, const char *)
{
  double fValue;
  if (Parse(pValue, fValue)) {
    return new Float(_name, fValue);
  }
  return nullptr;
}

Float::~Float()
{
}

void Float::set(double d)
{
  value = d;
  //if(get_xref())
  //  get_xref()->set(d);
}

void Float::set(int64_t i)
{
  double d = (double)i;
  set(d);
}

void Float::set(Value *v)
{
  /* typeCheck means cannot set integers - RRR
  Float *fv = typeCheck(v,string("set "));
  double d = fv->getVal();
  set(d);
  */
   double d;

  if (typeid(*v) != typeid(Float) &&
      typeid(*v) != typeid(Integer))
  {
    throw TypeMismatch("set ", "Float", v->showType());
  }
  v->get_as(d);
  set(d);
}

void Float::set(const char *buffer, int )
{
  if (buffer) {
    double d;
    if (Parse(buffer, d)) {
      set(d);
    }
  }
}

void Float::set(Packet &p)
{
  double d;
  if (p.DecodeFloat(d)) {
    set(d);
  }
}

void Float::get_as(int64_t &i)
{
  double d;
  get_as(d);
  i = (int64_t)d;
}

void Float::get_as(double &d)
{
  d = value;
}

void Float::get_as(char *buffer, int buf_size)
{
  if (buffer) {

    double d;;
    get_as(d);

    snprintf(buffer, buf_size, "%g", d);
  }
}

void Float::get_as(Packet &pb)
{
  double d;
  get_as(d);

  pb.EncodeFloat(d);
}

Value *Float::copy()
{
  double d;
  get_as(d);
  return new Float(d);
}

std::string Float::toString()
{
  return toString("%#-16.16g");
}

std::string Float::toString(const char* format)
{
  char cvtBuf[1024];

  double d;
  get_as(d);

  snprintf(cvtBuf, sizeof(cvtBuf), format, d);
  return cvtBuf;
}

char *Float::toString(char *return_str, int len)
{
  if (return_str) {

    double d;
    get_as(d);
    snprintf(return_str, len, "%g", d);
  }

  return return_str;
}

Float* Float::typeCheck(Value* val, std::string valDesc)
{
  if (typeid(*val) != typeid(Float)) {
    throw TypeMismatch(valDesc, "Float", val->showType());
  }

  // This static cast is totally safe in light of our typecheck, above.
  return static_cast<Float *>(val);
}

bool Float::compare(ComparisonOperator *compOp, Value *rvalue)
{
  Float *rv = typeCheck(rvalue,"");

  double d,r;
  get_as(d);
  rv->get_as(r);

  if (d < r)
    return compOp->less();

  if (d > r)
    return compOp->greater();

  return compOp->equal();
}

/*
bool Float::operator<(Value *rv)
{
  Float *_rv = typeCheck(rv,"OpLT");
  return value < _rv->value;
}
*/

/*****************************************************************
 * The String class.
 */
String::String(const char *newValue)
{
  if (newValue)
    value = newValue;
}

String::String(const char *newValue, size_t len)
{
  if (newValue)
    value.assign(newValue, len);
}

String::String(const char *_name, const char *newValue, const char *_desc)
  : Value(_name, _desc)
{
  if (newValue)
    value = newValue;
}

String::~String()
{
}

std::string String::toString()
{
  return value;
}

char *String::toString(char *return_str, int len)
{
  if (return_str)
    snprintf(return_str, len, "%s", value.c_str());

  return return_str;
}

void String::set(Value *v)
{
  if (v) {
    std::string buf = v->toString();
    set(buf.c_str());
  }
}

// TODO: is this meant to do something
void String::set(Packet &)
{
  std::cout << " fixme String::set(Packet &) is not implemented\n";
}

// TODO: was len meant to do anything
void String::set(const char *s, int )
{
  if (s)
    value = s;
}

void String::get_as(char *buf, int len)
{
  if (buf)
    snprintf(buf, len, "%s", value.c_str());
}

void String::get_as(Packet &p)
{
  p.EncodeString(value.c_str());
}

const char *String::getVal()
{
  return value.c_str();
}

Value *String::copy()
{
  return new String(value.c_str());
}

//------------------------------------------------------------------------
namespace gpsim {
  Function::Function(const char *_name, const char *desc)
    : gpsimObject(_name,desc)
  {
  }

  Function::~Function()
  {
    std::cout << "Function destructor\n";
  }

  std::string Function::description()
  {
    return cpDescription ? cpDescription : "no description";
  }

  std::string Function::toString()
  {
    return name();
  }

  void Function::call(ExprList_t *)
  {
    std::cout << "calling " << name() << '\n';
  }

}
