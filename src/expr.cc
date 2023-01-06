#include <string>
#include <cstdio>
#include <assert.h>

#include "expr.h"
#include "errors.h"
#include "ValueCollections.h"
#include "processor.h"
#include "registers.h"
#include "value.h"

//------------------------------------------------------------------------

Expression::Expression()
{
}


Expression::~Expression()
{
}


/*****************************************************************
 * The LiteralArray class.
 */
LiteralArray::LiteralArray(ExprList_t *pExprList)
  : m_pExprList(pExprList)
{
}


LiteralArray::~LiteralArray()
{
}


Value *LiteralArray::evaluate()
{
  return new Boolean(true);
}


std::string LiteralArray::toString()
{
  return "FixMe";
}


/*****************************************************************
 * The LiteralBoolean class.
 */
LiteralBoolean::LiteralBoolean(Boolean* value_)
{
  assert(value_ != nullptr);
  value = value_;
}


LiteralBoolean::~LiteralBoolean()
{
  delete value;
}


Value* LiteralBoolean::evaluate()
{
  bool b;
  value->get_as(b);
  return new Boolean(b);
}


std::string LiteralBoolean::toString()
{
  return value->toString();
}


//------------------------------------------------------------------------

LiteralInteger::LiteralInteger(Integer* newValue)
  : Expression()
{
  assert(newValue != nullptr);
  value = newValue;
}


LiteralInteger::~LiteralInteger()
{
  delete value;
}


Value* LiteralInteger::evaluate()
{
  int64_t i;
  value->get_as(i);
  return new Integer(i);
}


std::string LiteralInteger::toString()
{
  //return value->toString("0x%x");
  return value->toString();
}


int LiteralInteger::set_break(ObjectBreakTypes bt, ObjectActionTypes at, Expression *expr)
{
  return value ? value->set_break(bt, at, expr) : -1;
}


/*****************************************************************
 * The LiteralFloat class.
 */
LiteralFloat::LiteralFloat(Float* value_)
{
  assert(value_ != nullptr);
  value = value_;
}


LiteralFloat::~LiteralFloat()
{
  delete value;
}


Value* LiteralFloat::evaluate()
{
  double d;
  value->get_as(d);
  return new Float(d);
}


std::string LiteralFloat::toString()
{
  return value->toString();
}


/*****************************************************************
 * The LiteralString class.
 */
LiteralString::LiteralString(String* value_)
{
  value = value_;
}


LiteralString::~LiteralString()
{
  delete value;
}


Value* LiteralString::evaluate()
{
  return new String(value->getVal());
}


std::string LiteralString::toString()
{
  return value->toString();
}


/*****************************************************************
 * The LiteralSymbol class
 *
 * The literal symbol is a thin 'literal' wrapper for the symbol class.
 * The command line parser uses LiteralSymbol whenever an expression
 * encounters a symbol.
 */

LiteralSymbol::LiteralSymbol(gpsimObject *_sym)
{
  sym = dynamic_cast<Value *>(_sym);

  if (!sym) {
    if (_sym) {
      std::string s = "literal symbol '";
      s += _sym->name();
      s += "' does not have a value";
      throw Error(s);

    } else {
      throw Error("NULL pointer to literal symbol");
    }
  }
}


LiteralSymbol::~LiteralSymbol()
{
}


Value *LiteralSymbol::evaluate()
{
    if (sym) {
	if (get_address) {
	    Register *p_Reg = dynamic_cast<Register *>(sym);
	    if (p_Reg) {
		return new Integer(p_Reg->getAddress());
	    }
	}
	return sym->evaluate();
    }
    return nullptr;
}


Value *LiteralSymbol::GetSymbol()
{
  return sym;
}


std::string LiteralSymbol::toString()
{
  if (sym) {
    return sym->name();
  }

  return std::string();
}


int LiteralSymbol::set_break(ObjectBreakTypes bt, ObjectActionTypes at, Expression *expr)
{
  return sym ? sym->set_break(bt, at, expr) : -1;
}


int LiteralSymbol::clear_break()
{
  return sym ? sym->clear_break() : -1;
}


/*****************************************************************
 * The LiteralSymbol class
 */
IndexedSymbol::IndexedSymbol(gpsimObject *pSymbol, ExprList_t *pExprList)
  : m_pExprList(pExprList)
{
  m_pSymbol = dynamic_cast<Value *>(pSymbol);
  assert(m_pSymbol != nullptr);
  assert(pExprList != nullptr);
}


IndexedSymbol::~IndexedSymbol()
{
}


Value* IndexedSymbol::evaluate()
{
  // Indexed symbols with more than one index expression
  // cannot be evaluated
  if (m_pExprList->size() > 1) {
    // Could return an AbstractRange
    throw Error("Indexed variable evaluates to more than one value");
  }

  IIndexedCollection *pIndexedCollection =
    dynamic_cast<IIndexedCollection *>(m_pSymbol);

  if (!pIndexedCollection) {
    throw Error("Cannot index this variable");

  } else {
    Value *pV = m_pExprList->front()->evaluate();
    unsigned int ui = *pV;
    return pIndexedCollection->GetAt(ui).copy();
  }
}


std::string IndexedSymbol::toString()
{
  IIndexedCollection *pIndexedCollection =
    dynamic_cast<IIndexedCollection *>(m_pSymbol);

  if (pIndexedCollection == nullptr) {
    return std::string("The symbol ") + m_pSymbol->name() + " is not an indexed variable";

  } else {
    return pIndexedCollection->toString(m_pExprList);
  }

  return "IndexedSymbol not initialized";
}


/*****************************************************************
 * The RegisterExpression class
 *
 * The literal symbol is a thin 'literal' wrapper for the symbol class.
 * The command line parser uses RegisterExpression whenever an expression
 * encounters a symbol.
 */

RegisterExpression::RegisterExpression(unsigned int uAddress)
  : m_uAddress(uAddress)
{
}


RegisterExpression::~RegisterExpression()
{
}


Value* RegisterExpression::evaluate()
{
  Register *pReg = get_active_cpu()->rma.get_register(m_uAddress);

  if (pReg) {
    return new Integer(pReg->get_value());

  } else {
    static char sFormat[] = "reg(%u) is not a valid register";
    char sBuffer[sizeof(sFormat) + 10];
    snprintf(sBuffer, sizeof(sBuffer), sFormat, m_uAddress);
    throw Error(sBuffer);
  }
}


std::string RegisterExpression::toString()
{
  char sBuffer[10];
  snprintf(sBuffer, sizeof(sBuffer), "%u", m_uAddress);
  return sBuffer;
}
