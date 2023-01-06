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


#ifndef SRC_OPERATOR_H_
#define SRC_OPERATOR_H_

#include <string>

#include "expr.h"
#include "gpsim_object.h"

class Value;


class Operator : public Expression {
public:
  explicit Operator(const std::string & newOpString)
    : opString(newOpString)
  {}

  virtual ~Operator()
  {
  }

  std::string showOp()
  {
    return opString;
  }

private:
  std::string opString;
};


class BinaryOperator : public Operator {
public:
  BinaryOperator(const std::string & opString, Expression* leftExpr, Expression* rightExpr);
  virtual ~BinaryOperator();

  virtual Value* shortCircuit(Value* leftValue);
  virtual Value* applyOp(Value* leftValue, Value* rightValue) = 0;
  virtual Value* evaluate() override;
  virtual Expression *getLeft();
  virtual Expression *getRight();

  std::string show();
  std::string showType();
  std::string toString() override;

protected:
  Expression* leftExpr;
  Expression* rightExpr;
  Value* value = nullptr;
};


class UnaryOperator : public Operator {
public:
  UnaryOperator(const std::string & opString, Expression* expr);
  virtual ~UnaryOperator();

  virtual Value* applyOp(Value* value) = 0;

  Value* evaluate() override;

  std::string show();
  std::string showType();
  std::string toString() override;

protected:
  Expression*  expr;
  Value* value = nullptr;
};


class ComparisonOperator : public BinaryOperator {
public:
  ComparisonOperator(const std::string & opString, Expression*, Expression*);
  virtual ~ComparisonOperator();

  enum ComparisonTypes {
    eOpEq,
    eOpGe,
    eOpGt,
    eOpLe,
    eOpLt,
    eOpNe
  };

  Value* applyOp(Value* leftValue, Value* rightValue) override;

  bool less()
  {
    return bLess;
  }
  bool equal()
  {
    return bEqual;
  }
  bool greater()
  {
    return bGreater;
  }

  virtual ComparisonTypes isa() = 0;
  int set_break(ObjectBreakTypes bt = eBreakAny,
                ObjectActionTypes at = eActionHalt,
                Expression *expr = nullptr) override;

protected:
  bool bLess = false;
  bool bEqual = false;
  bool bGreater = false;
};


//-----------------------------------------------------------------
class OpAbstractRange : public BinaryOperator {
public:
  OpAbstractRange(Expression *leftExpr, Expression *rightExpr);
  virtual ~OpAbstractRange();

  Value* applyOp(Value* leftValue, Value* rightValue) override;
};


//-----------------------------------------------------------------
class OpAdd : public BinaryOperator {
public:
  OpAdd(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpAdd();

  virtual Value* applyOp(Value* leftValue, Value* rightValue) override;
};


//-----------------------------------------------------------------
class OpAnd : public BinaryOperator {
public:
  OpAnd(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpAnd();

  Value* applyOp(Value* leftValue, Value* rightValue) override;
};


//-----------------------------------------------------------------
class OpDiv : public BinaryOperator {
public:
  OpDiv(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpDiv();

  Value* applyOp(Value* leftValue, Value* rightValue) override;
};


//-----------------------------------------------------------------
class OpEq : public ComparisonOperator {
public:
  OpEq(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpEq();

  ComparisonOperator::ComparisonTypes isa() override
  {
    return ComparisonOperator::eOpEq;
  }
};


//-----------------------------------------------------------------
class OpGe : public ComparisonOperator {
public:
  OpGe(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpGe();

  ComparisonOperator::ComparisonTypes isa() override
  {
    return ComparisonOperator::eOpGe;
  }
};


//-----------------------------------------------------------------
class OpGt : public ComparisonOperator {
public:
  OpGt(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpGt();

  ComparisonOperator::ComparisonTypes isa() override
  {
    return ComparisonOperator::eOpGt;
  }
};


//-----------------------------------------------------------------
class OpLe : public ComparisonOperator {
public:
  OpLe(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpLe();

  ComparisonOperator::ComparisonTypes isa() override
  {
    return ComparisonOperator::eOpLe;
  }
};


//-----------------------------------------------------------------
class OpLogicalAnd : public BinaryOperator {
public:
  OpLogicalAnd(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpLogicalAnd();

  Value* applyOp(Value* leftValue, Value* rightValue) override;
};


//-----------------------------------------------------------------
class OpLogicalOr : public BinaryOperator {
public:
  OpLogicalOr(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpLogicalOr();

  Value* applyOp(Value* leftValue, Value* rightValue) override;
};


//-----------------------------------------------------------------
class OpLt : public ComparisonOperator {
public:
  OpLt(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpLt();

  ComparisonOperator::ComparisonTypes isa() override
  {
    return ComparisonOperator::eOpLt;
  }
};


//-----------------------------------------------------------------
class OpMpy : public BinaryOperator {
public:
  OpMpy(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpMpy();

  Value* applyOp(Value* leftValue, Value* rightValue) override;
};


//-----------------------------------------------------------------
class OpNe : public ComparisonOperator {
public:
  OpNe(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpNe();

  ComparisonOperator::ComparisonTypes isa() override
  {
    return ComparisonOperator::eOpNe;
  }
};


//-----------------------------------------------------------------
class OpOr : public BinaryOperator {
public:
  OpOr(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpOr();

  Value* applyOp(Value* leftValue, Value* rightValue) override;
};


//-----------------------------------------------------------------
class OpShl : public BinaryOperator {
public:
  OpShl(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpShl();

  Value* applyOp(Value* leftValue, Value* rightValue) override;
};


//-----------------------------------------------------------------
class OpShr : public BinaryOperator {
public:
  OpShr(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpShr();

  Value* applyOp(Value* leftValue, Value* rightValue) override;
};


//-----------------------------------------------------------------
class OpSub : public BinaryOperator {
public:
  OpSub(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpSub();

  Value* applyOp(Value* leftValue, Value* rightValue) override;
};


//-----------------------------------------------------------------
class OpXor : public BinaryOperator {
public:
  OpXor(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpXor();

  Value* applyOp(Value* leftValue, Value* rightValue) override;
};


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//            Unary objects
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// -----------------------------------------------------------------
class OpLogicalNot : public UnaryOperator {
public:
  explicit OpLogicalNot(Expression* expr);
  virtual ~OpLogicalNot();

  Value* applyOp(Value* value) override;
};


// -----------------------------------------------------------------
class OpNegate : public UnaryOperator {
public:
  explicit OpNegate(Expression* expr);
  virtual ~OpNegate();

  Value* applyOp(Value* value) override;
};


// -----------------------------------------------------------------
class OpOnescomp : public UnaryOperator {
public:
  explicit OpOnescomp(Expression* expr);
  virtual ~OpOnescomp();

  Value* applyOp(Value* value) override;
};


// -----------------------------------------------------------------
class OpPlus : public UnaryOperator {
public:
  explicit OpPlus(Expression* expr);
  virtual ~OpPlus();

  Value* applyOp(Value* value) override;
};


// -----------------------------------------------------------------
class OpIndirect : public UnaryOperator {
public:
  explicit OpIndirect(Expression* expr);
  virtual ~OpIndirect();

  Value* applyOp(Value* value) override;
};


// -----------------------------------------------------------------
class OpAddressOf : public UnaryOperator {
public:
  explicit OpAddressOf(Expression* expr);
  virtual ~OpAddressOf();

  Value* evaluate() override;
  Value* applyOp(Value* value) override;
};


#endif // SRC_OPERATOR_H_
