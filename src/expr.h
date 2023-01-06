/* Parser for gpsim
   Copyright (C) 2004 Scott Dattalo

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

#ifndef SRC_EXPR_H_
#define SRC_EXPR_H_

#include <list>
#include <string>
#include <glib.h>
#include "gpsim_object.h"
#include "value.h"

class Boolean;
class Float;
class Integer;
class String;
class Value;

class Expression : public gpsimObject {
public:
  bool get_address = false;
  Expression();
  virtual ~Expression();

  virtual Value* evaluate() = 0;
};

typedef std::list<Expression *> ExprList_t;
typedef std::list<Expression *>::iterator ExprList_itor;


//************************************************************************//
//
// Literal Expressions
//
// A Literal Expression is a wrapper around a Value object.

//----------------------------------------------------------------

class IndexedSymbol : public Expression {
public:
  IndexedSymbol(gpsimObject *, ExprList_t*);
  virtual ~IndexedSymbol();

  Value* evaluate() override;
  std::string toString() override;

private:
  Value *       m_pSymbol;
  ExprList_t *  m_pExprList;
};


//-----------------------------------------------------------------
class LiteralSymbol : public Expression {
public:
  explicit LiteralSymbol(gpsimObject *);
  //LiteralSymbol(gpsimObject *, ExprList_t*);
  virtual ~LiteralSymbol();

  Value* evaluate() override;
  int set_break(ObjectBreakTypes bt = eBreakAny,
                ObjectActionTypes at = eActionHalt,
                Expression *expr = nullptr) override;
  int clear_break() override;
  std::string toString() override;
  Value *GetSymbol();

private:
  Value *sym;
};


//-----------------------------------------------------------------
class LiteralArray : public Expression {
public:
  explicit LiteralArray(ExprList_t*);
  virtual ~LiteralArray();

  Value* evaluate() override;
  std::string toString() override;

private:
  ExprList_t *  m_pExprList;
};


//-----------------------------------------------------------------
class LiteralBoolean : public Expression {
public:
  explicit LiteralBoolean(Boolean* value);
  virtual ~LiteralBoolean();

  Value* evaluate() override;
  std::string toString() override;

private:
  Boolean* value;
};


//-----------------------------------------------------------------
class LiteralInteger : public Expression {
public:
  explicit LiteralInteger(Integer* value);
  virtual ~LiteralInteger();

  Value* evaluate() override;
  int set_break(ObjectBreakTypes bt = eBreakAny,
                ObjectActionTypes at = eActionHalt,
                Expression *expr = nullptr) override;
  std::string toString() override;

private:
  Integer* value;
};


//-----------------------------------------------------------------
class LiteralFloat : public Expression {
public:
  explicit LiteralFloat(Float* value);
  virtual ~LiteralFloat();

  Value* evaluate() override;
  std::string toString() override;

private:
  Float* value;
};


//-----------------------------------------------------------------
class LiteralString : public Expression {
public:
  explicit LiteralString(String* newValue);
  virtual ~LiteralString();

  Value* evaluate() override;
  std::string toString() override;

private:
  String* value;
};


class RegisterExpression : public Expression {
public:
  explicit RegisterExpression(unsigned int uAddress);
  virtual ~RegisterExpression();

  Value* evaluate() override;
  std::string toString() override;

private:
  unsigned int  m_uAddress;
};


#endif // SRC_EXPR_H_
