
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

#include <stdio.h>

#include "bit.h"
#include "registers.h"


Bit::Bit(RegisterValue &rv, unsigned int bit_mask)
{
  d = (rv.get() & bit_mask) ? true : false;
  i = (rv.geti() & bit_mask) ? false : true;
}


static void pr(const char *s, Bit &bv)
{
  printf("%s:%d,%d\n", s, bv.isOne(), bv.isKnown());
}



///-- Class test

void test_bits()
{
  static bool tested = false;
  if (tested)
    return;

  tested = true;

  Bit bOne(true, true);
  pr("one", bOne);
  Bit a = bOne;
  Bit b = bOne;
  Bit c = bOne;
  b.clear();

  pr("a", a);
  pr("b", b);

  for (int i = 0; i < 4; i++) {
    switch (i) {

    case 0:
      puts("Both known");
      a.set(true, true);
      b.set(false, true);
      break;

    case 1:
      puts("a is unknown");
      a.set(true, false);
      b.set(false, true);
      break;
    case 2:
      puts("b is unknown");
      a.set(true, true);
      b.set(false, false);
      break;
    case 3:
      puts("a and b are unknown");
      a.set(true, false);
      b.set(false, false);
      break;
    }

    pr("a", a);
    pr("b", b);

    c = a;
    pr("c=a ->c", c);

    c = b;
    pr("c=b ->c", c);

    c |= a;
    pr("c|=a ->c", c);

    c &= a;
    pr("c&=a ->c", c);

    c |= b;
    pr("c|=b ->c", c);

    c &= b;
    pr("c&=b ->c", c);

    c = b;
    if (c == b)
      pr("c=b ->c", c);
    else
      pr("assignment failed c=b ->c", c);

    c = a;
    if (c == a)
      pr("c=a ->c", c);
    else
      pr("assignment failed c=a ->c", c);

    c = a | a;
    pr("c=a|a ->c", c);

    c = a | b;
    pr("c=a|b ->c", c);

    c = b | a;
    pr("c=b|a ->c", c);

    c = b | b;
    pr("c=b|b ->c", c);

    c = !b;
    pr("c=!b ->c", c);
    pr("     ->b", b);


    c = a & a;
    pr("c=a&a ->c", c);

    c = a & b;
    pr("c=a&b ->c", c);

    c = b & a;
    pr("c=b&a ->c", c);

    c = b & b;
    pr("c=b&b ->c", c);

    c = a & (!a);
    pr("c=a&!a ->c", c);

    c = a & (!b);
    pr("c=a&!b ->c", c);

    c = b & (!a);
    pr("c=b&!a ->c", c);

    c = b & (!b);
    pr("c=b&!b ->c", c);
  }

  Bit d(false, true);
  a.set(false, false);
  b.set(false, false);

  c = a & b & d;
  pr("a", a);
  pr("b", b);
  pr("d", d);
  pr("c=a&b&d ->c", c);

  c = (!a) & (!b);
  pr("c=!a & !b ->c", c);

  a.set(true, false);
  b.set(false, true);
  Bit e = a & (!b);
  pr("a", a);
  pr("b", b);
  pr("e=a & !b ->e", e);

  a.set(false, false);
  b.set(false, true);
  e = a & (!b);
  pr("a", a);
  pr("b", b);
  pr("e=a & !b ->e", e);

  a.set(false, false);
  b.set(true, true);
  e = a & (!b);
  pr("a", a);
  pr("b", b);
  pr("e=a & !b ->e", e);

  a.set(true, false);
  b.set(true, true);
  e = a & (!b);
  pr("a", a);
  pr("b", b);
  pr("e=a & !b ->e", e);
}
