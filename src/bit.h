/*
   Copyright (C) 1998-2003 T. Scott Dattalo

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

#ifndef SRC_BIT_h_
#define SRC_BIT_h_

// Function to test the Bit Class:
extern void test_bits();

// Bit
//
// The Bit object is like a bool except that it supports 3-state
// logic.

class RegisterValue;
class Bit
{
public:

  Bit(RegisterValue &rv, unsigned int bit_mask);
  Bit(bool nd = false, bool ni = false)
    : d(nd), i(ni)
  {
  }
  
  inline Bit & operator = (const Bit &bv)
  {
    d = bv.d;
    i = bv.i;
    return *this;
  }
  inline bool operator == (const Bit &bv) const
  {
    return d == bv.d && i == bv.i;
  }
  inline void operator |= (const Bit &bv)
  {
    i = i & bv.i || (i & d) || (bv.d & bv.i);
    d |= bv.d;
  }

  inline void operator &= (const Bit &bv)
  {
    i = (i & bv.i) || (i & (!bv.d)) || ((!d) & bv.i);
    d &= bv.d;
  }

  inline void operator ^= (const Bit &bv) 
  {
    d ^= bv.d;
    i &= bv.i;
  }

  inline Bit operator & (const Bit &r)
  {
    Bit bLv = *this;

    bLv.i = bLv.i & r.i || (r.i & (!r.d)) || ((!bLv.d) & bLv.i);
    bLv.d &= r.d;

    return bLv;
  }

  inline Bit operator | (const Bit &bv)
  {
    Bit bLv = *this;
    bLv |= bv;
    return bLv;
  }
  inline Bit operator ^ (const Bit &bv)
  {
    Bit bLv = *this;
    bLv ^= bv;
    return bLv;
  }
  inline bool isZero()
  {
    return d == false && i == true;
  }
  inline bool isOne()
  {
    return d == true && i == true;
  }
  inline bool isKnown()
  {
    return i == true;
  }
  inline bool isUnKnown()
  {
    return i == false;
  }
  inline void set(bool nd, bool ni)
  {
    d = nd;
    i = ni;
  }

  inline void clear()
  {
    d = false;
    i = true;
  }
  inline void set() 
  {
    d = true;
    i = true;
  }
  inline void setUnknown()
  {
    i = false;
  }

  inline char val()
  {
    return !isKnown() ? '?' : (isOne() ? '1' : '0');
  }

  friend Bit operator ! (Bit b);
  
private:
  bool d;  // The data
  bool i;  // i=true if d is valid
};

inline Bit operator ! (Bit b)
{
  Bit bLv = b;
  bLv.d = !bLv.d;
  return bLv;
}

#endif // SRC_BIT_h_
