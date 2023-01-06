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

#include "ValueCollections.h"
#include "symbol.h"
#include "registers.h"

#include <list>
#include <string.h>
#include <sstream>

IIndexedCollection::IIndexedCollection(const char *pName,
                                       const char *pDesc,
                                       int iAddressRadix)
  : Value(pName, pDesc)
{
  SetAddressRadix(iAddressRadix);
}


IIndexedCollection::IIndexedCollection(int iAddressRadix)
{
  SetAddressRadix(iAddressRadix);
}


void IIndexedCollection::SetAddressRadix(int iRadix)
{
  m_iAddressRadix = iRadix;

  if (iRadix == 16) {
    strcpy(m_szPrefix, "$");

  } else {
    m_szPrefix[0] = 0;
  }
}


void IIndexedCollection::Set(Value * pValue)
{
  unsigned int  uUpper = GetUpperBound() + 1;

  for (unsigned int uIndex = GetLowerBound(); uIndex < uUpper; uIndex++) {
    SetAt(uIndex, pValue);
  }
}


void IIndexedCollection::SetAt(ExprList_t* pIndexers, Expression *pExpr)
{
    Value * pValue = pExpr->evaluate();

    for (auto &it : *pIndexers)
    {
        Value * pIndex = it->evaluate();
        Integer *pIntIndex = dynamic_cast<Integer*>(pIndex);

        if (pIntIndex)
        {
            SetAt(int(*pIntIndex), pValue);
        }
        else
        {
            AbstractRange *pRange = dynamic_cast<AbstractRange*>(pIndex);

            if (pRange)
            {
                unsigned int uEnd = pRange->get_rightVal() + 1;

                for (unsigned int uIndex = pRange->get_leftVal(); uIndex < uEnd; uIndex++)
                {
                    SetAt(uIndex, pValue);
                }
            }
            else
            {
                Register *pReg = dynamic_cast<Register*>(pIndex);

                if (pReg)
                {
                    SetAt(pReg->getAddress(), pValue);
                }
                else
                {
                    throw Error("indexer not valid");
                }
            }
        }
        delete pIndex;
    }

    delete pValue;
}


char *IIndexedCollection::toString(char *pBuffer, int len)
{
    if (pBuffer && len > 0)
    {
        strncpy(pBuffer, toString().c_str(), len);
        pBuffer[len - 1] = '\0';
    }
    return pBuffer;
}


std::string IIndexedCollection::toString()
{
  int iColumnWidth = 0;
  std::vector<std::string> asIndexes;
  std::vector<std::string> asValue;
  ConsolidateValues(iColumnWidth, asIndexes, asValue);
  return toString(iColumnWidth, asIndexes, asValue);
}


std::string IIndexedCollection::toString(ExprList_t* pIndexerExprs)
{
    try
    {
        if (!pIndexerExprs)
        {
            return toString();
        }
        else
        {
            std::ostringstream sOut;
            for (auto &it : *pIndexerExprs)
            {
                Value * pIndex = it->evaluate();
                AbstractRange *pRange = dynamic_cast<AbstractRange*>(pIndex);

                if (pRange)
                {
                    unsigned int uEnd = pRange->get_rightVal() + 1;

                    for (unsigned int uIndex = pRange->get_leftVal(); uIndex < uEnd; uIndex++)
                    {
                        Value &Value = GetAt(uIndex);
                        sOut << Value.name() << " = " << Value.toString() << '\n';
                    }
                    continue;
                }

                String *pName = dynamic_cast<String*>(pIndex);
                Integer *pInt = pName ?
                        globalSymbolTable().findInteger(pName->getVal()) :
                        dynamic_cast<Integer*>(pIndex);
                Integer temp(0);
                if (pInt == nullptr)
                {
          // This is a temp workaround. I (JR) would expect a register symbol
          // evaluate to an Integer object containing the value of the
          // register. It currently returns an object that is a copy
          // of the register_symbol object.
                    Register *pReg = dynamic_cast<Register*>(pIndex);

                    if (pReg)
                    {
                        int64_t i = pReg->get_value();
                        temp.set(i);
                        pInt = &temp;
                    }
                }

                if (pInt)
                {
                    unsigned int uIndex = (unsigned int)pInt->getVal();

                    if (bIsIndexInRange(uIndex))
                    {
                        Value &Value = GetAt(uIndex);
                        sOut << Value.name() << " = " << Value.toString() << '\n';
                    }
                    else
                    {
                        sOut << "Error: Index " << uIndex << " is out of range\n";
                    }
                }
                else
                {
                    sOut << "Error: The index specified for '"
                        << name() << "' does not contain a valid index.\n";
                }
                delete pIndex;
            }
            return sOut.str();
        }
    }
    catch (const Error &e)
    {
        return e.what();
    }
}


void IIndexedCollection::PushValue(int iFirstIndex, int iCurrentIndex,
                                   Value *pValue,
                                   std::vector<std::string> &asIndexes,
                                   std::vector<std::string> &asValue)
{
    std::ostringstream sIndex;

    if (m_iAddressRadix == 16)
    {
        sIndex << std::hex;
    }

    sIndex << Value::name() << '[' << m_szPrefix << iFirstIndex;

    if (iFirstIndex != iCurrentIndex)
    {
        sIndex << ".." << m_szPrefix << iCurrentIndex;
    }

    sIndex << ']';
    asIndexes.push_back(sIndex.str());
    asValue.push_back(pValue->toString());
}


std::string IIndexedCollection::ElementIndexedName(unsigned int iIndex)
{
    std::ostringstream sIndex;

    if (m_iAddressRadix == 16)
    {
        sIndex << std::hex;
    }

    sIndex << Value::name() << '[' << m_szPrefix << iIndex << ']';
    return sIndex.str();
}


std::string IIndexedCollection::toString(int iColumnWidth,
    std::vector<std::string> &asIndexes,
    std::vector<std::string> &asValue)
{
    std::ostringstream sOut;
    auto itValue = asValue.cbegin();
    auto itElement = asIndexes.cbegin();
    auto itElementEnd = asIndexes.cend();

    // Dump the consolidated element list
    for ( ; itElement != itElementEnd; ++itElement, ++itValue)
    {
        sOut.width(iColumnWidth);
        sOut.setf(std::ios_base::left);
        sOut << *itElement << " = " << *itValue;

        if (itElement + 1 != itElementEnd)
        {
            sOut << '\n';
        }
    }

    return sOut.str();
}


Integer * IIndexedCollection::FindInteger(const char *s)
{
  return globalSymbolTable().findInteger(s);
}
