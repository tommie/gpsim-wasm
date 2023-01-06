/*
   Copyright (C) 2006T. Scott Dattalo

This file is part of the libgpsim_modules library of gpsim

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

/*
  ttl.cc - gpsim's TTL library

*/


#include "../config.h"

#ifdef HAVE_GUI
#include <gtk/gtk.h>
#endif

#include "ttl.h"
#include "../src/stimuli.h"
#include "../src/packages.h"
#include "../src/gpsim_time.h"

#include <iostream>
#include <iomanip>

//#define DEBUG
#if defined(DEBUG)
#include "../config.h"
#define Dprintf(arg) {printf("%s:%d %s ",__FILE__,__LINE__,__FUNCTION__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

namespace TTL {

//------------------------------------------------------------------------
// TTL base class

TTLbase::TTLbase(const char *_name, const char *desc)
  : Module(_name, desc), m_dVdd(0.0), m_bClock(false), m_bEnable(false)
{
}


TTLbase::~TTLbase()
{
}


//------------------------------------------------------------------------
// Some edge-sensitive pins
//
//

//------------------------------------------------------------
// Clock
//
class Clock : public IOPIN {
public:
  Clock(const char *n, TTLbase *pParent);

  virtual void setDrivenState(bool);

private:
  TTLbase *m_pParent;
};


Clock::Clock(const char *n, TTLbase *pParent)
  : IOPIN(n), m_pParent(pParent)
{
}


void Clock::setDrivenState(bool bNewState)
{
  IOPIN::setDrivenState(bNewState);

  if (m_pParent) {
    m_pParent->setClock(bNewState);
  }
}


//------------------------------------------------------------
// Strobe
//
class Strobe : public IOPIN {
public:
  Strobe(const char *n, TTLbase *pParent);

  virtual void setDrivenState(bool);

private:
  TTLbase *m_pParent;
};


Strobe::Strobe(const char *n, TTLbase *pParent)
  : IOPIN(n), m_pParent(pParent)
{
}


void Strobe::setDrivenState(bool bNewState)
{
  IOPIN::setDrivenState(bNewState);

  if (m_pParent) {
    m_pParent->setStrobe(bNewState);
  }
}


//------------------------------------------------------------
// Enable
//
class Enable : public IOPIN {
public:
  Enable(const char *n, TTLbase *pParent);

  virtual void setDrivenState(bool);

private:
  TTLbase *m_pParent;
};


Enable::Enable(const char *n, TTLbase *pParent)
  : IOPIN(n), m_pParent(pParent)
{
}


void Enable::setDrivenState(bool bNewState)
{
  IOPIN::setDrivenState(bNewState);

  if (m_pParent) {
    m_pParent->setEnable(bNewState);
  }
}


//------------------------------------------------------------
// Reset
//
class Reset : public IOPIN {
public:
  Reset(const char *n, TTLbase *pParent);

  virtual void setDrivenState(bool);

private:
  TTLbase *m_pParent;
};


Reset::Reset(const char *n, TTLbase *pParent)
  : IOPIN(n), m_pParent(pParent)
{
}


void Reset::setDrivenState(bool bNewState)
{
  IOPIN::setDrivenState(bNewState);

  if (m_pParent) {
    m_pParent->setReset(bNewState);
  }
}


//------------------------------------------------------------------------
// TTL377 - Octal Latch
//
//

Module *TTL377::construct(const char *_new_name)
{
  TTL377 *pTTL377 = new TTL377(_new_name);
  pTTL377->new_name(_new_name);
  pTTL377->create_iopin_map();
  return pTTL377;
}


TTL377::TTL377(const char *_name)
  : TTLbase(_name, "TTL377 - Octal Latch")
{
  m_D = new IOPIN *[8];
  m_Q = new IO_bi_directional *[8];
  char pName[4];
  pName[2] = 0;
  int i;

  for (i = 0; i < 8; i++) {
    pName[0] = 'D';
    pName[1] = '0' + i;
    m_D[i] = new IOPIN(pName);
    addSymbol(m_D[i]);
    pName[0] = 'Q';
    m_Q[i] = new IO_bi_directional(pName);
    addSymbol(m_Q[i]);
    m_Q[i]->setDriving(true);
  }

  m_enable = new Enable("E", this);
  addSymbol(m_enable);
  m_clock  = new Clock("CP", this);
  addSymbol(m_clock);
}


TTL377::~TTL377()
{
  for (int i = 0; i < 8; i++) {
    removeSymbol(m_D[i]);
    removeSymbol(m_Q[i]);
  }

  delete [] m_D;
  delete [] m_Q;
  removeSymbol(m_enable);
  removeSymbol(m_clock);
}


void TTL377::setClock(bool bNewClock)
{
  if (bNewClock && !m_bClock && !m_bEnable) {
    update_state();
  }

  m_bClock = bNewClock;
}


void TTL377::setEnable(bool bNewEnable)
{
  m_bEnable = bNewEnable;
}


void TTL377::update_state()
{
  int i;
  bool state[8];

  // Copy the inputs to the outputs through an intermediary to simulate
  // the simultaneous action of the real part.
  for (i = 0; i < 8; i++) {
    state[i] = m_D[i]->getDrivenState();
  }

  for (i = 0; i < 8; i++) {
    m_Q[i]->putState(state[i]);
  }
}


void TTL377::create_iopin_map()
{
  package = new Package(20);
  package->assign_pin(1, m_enable);
  package->assign_pin(2, m_Q[0]);
  package->assign_pin(3, m_D[0]);
  package->assign_pin(4, m_D[1]);
  package->assign_pin(5, m_Q[1]);
  package->assign_pin(6, m_Q[2]);
  package->assign_pin(7, m_D[2]);
  package->assign_pin(8, m_D[3]);
  package->assign_pin(9, m_Q[3]);
  package->assign_pin(11, m_clock);
  package->assign_pin(12, m_Q[4]);
  package->assign_pin(13, m_D[4]);
  package->assign_pin(14, m_D[5]);
  package->assign_pin(15, m_Q[5]);
  package->assign_pin(16, m_Q[6]);
  package->assign_pin(17, m_D[6]);
  package->assign_pin(18, m_D[7]);
  package->assign_pin(19, m_Q[7]);
}


//------------------------------------------------------------------------
// TTL595 - Octal shift register
//
//

Module *TTL595::construct(const char *_new_name)
{
  TTL595 *pTTL595 = new TTL595(_new_name);
  pTTL595->new_name(_new_name);
  pTTL595->create_iopin_map();
  return pTTL595;
}


TTL595::TTL595(const char *_name)
  : TTLbase(_name, "TTL595 - Octal Shift Register"),  m_bStrobe(false), sreg(0)
{
  m_Q = new IO_bi_directional *[8];
  char pName[4] = "Q0";
  int i;

  for (i = 0; i < 8; i++) {
    pName[1] = '0' + i;
    m_Q[i] = new IO_bi_directional(pName);
    addSymbol(m_Q[i]);
    m_Q[i]->setDriving(true);
  }

  m_Ds = new IOPIN("Ds");
  addSymbol(m_Ds);
  m_Qs = new IO_bi_directional("Qs");
  addSymbol(m_Qs);
  m_Qs->setDriving(true);
  m_enable = new Enable("OE", this);
  addSymbol(m_enable);
  m_clock  = new Clock("SCK", this);
  addSymbol(m_clock);
  m_strobe = new Strobe("RCK", this);
  addSymbol(m_strobe);
  m_reset  = new Reset("MR", this);
  addSymbol(m_reset);
}


TTL595::~TTL595()
{
  for (int i = 0; i < 8; i++) {
    removeSymbol(m_Q[i]);
  }

  delete [] m_Q;
  removeSymbol(m_Ds);
  removeSymbol(m_Qs);
  removeSymbol(m_enable);
  removeSymbol(m_clock);
  removeSymbol(m_strobe);
  removeSymbol(m_reset);
}


void TTL595::setClock(bool bNewClock)
{
  // Clock shifts the shift register on rising edge, if MR pin is high
  Dprintf(("bNewClock %d m_bClock %d reset %d Ds %d\n", bNewClock, m_bClock, m_reset->getDrivenState(), m_Ds->getDrivenState()));

  if (bNewClock && !m_bClock && m_reset->getDrivenState()) {
    // Move the shift register left and out.
    sreg <<= 1;

    if (m_Ds->getDrivenState()) {
      sreg |= 0x01;
    }

    Dprintf(("%s sreg=%x now=0x%lx bNewClock %d m_bClock %d\n", name().c_str(), sreg, get_cycles().get(), bNewClock, m_bClock));
    get_cycles().set_break(get_cycles().get() + 1, this);
  }

  m_bClock = bNewClock;
}


void TTL595::setEnable(bool bNewEnable)
{
  // This is the output enable pin on this device
  for (int i = 0; i < 8; i++) {
    m_Q[i]->update_direction(!bNewEnable, true);
  }
}


void TTL595::setReset(bool bNewReset)
{
  if (!bNewReset) {
    sreg = 0;
    m_Qs->putState((sreg & 0x80) != 0);
  }
}


void TTL595::setStrobe(bool bNewStrobe)
{
  Dprintf(("bNewStrobe %d m_bStrobe %d\n", bNewStrobe, m_bStrobe));

  // Strobe copies the contents of the shift register into the outputs
  if (bNewStrobe && !m_bStrobe) {
    update_state();
  }

  m_bStrobe = bNewStrobe;
}


void TTL595::update_state()
{
  for (int i = 0, ss = sreg; i < 8; i++, ss >>= 1) {
    m_Q[i]->putState((bool)(ss & 1));
  }

  Dprintf(("sreg=0x%x\n", sreg));
}


void TTL595::callback_print()
{
  std::cout << "TTL595 " << name() << " CallBack ID 0x" << std::hex << CallBackID << '\n';
}


void TTL595::callback()
{
  Dprintf(("%s %s now=0x%lx\n", __FUNCTION__, name().c_str(), get_cycles().get()));
  m_Qs->putState((sreg & 0x80) != 0);
}


void TTL595::create_iopin_map()
{
  package = new Package(16);
  package->assign_pin(1, m_Q[1]);
  package->assign_pin(2, m_Q[2]);
  package->assign_pin(3, m_Q[3]);
  package->assign_pin(4, m_Q[4]);
  package->assign_pin(5, m_Q[5]);
  package->assign_pin(6, m_Q[6]);
  package->assign_pin(7, m_Q[7]);
  package->assign_pin(9, m_Qs);
  package->assign_pin(10, m_reset);
  package->assign_pin(11, m_clock);
  package->assign_pin(12, m_strobe);
  package->assign_pin(13, m_enable);
  package->assign_pin(14, m_Ds);
  package->assign_pin(15, m_Q[0]);
}


//------------------------------------------------------------------------
// TTL165 - 8-bit parallel to serial shift register
//
//

Module *TTL165::construct(const char *_new_name)
{
  TTL165 *pTTL165 = new TTL165(_new_name);
  pTTL165->new_name(_new_name);
  pTTL165->create_iopin_map();
  return pTTL165;
}


TTL165::TTL165(const char *_name)
  : TTLbase(_name, "TTL165 - PISO Shift Register"),
    m_bStrobe(false), sreg(0)
{
  m_D = new IOPIN *[8];
  char pName[4] = "D0";
  int i;

  for (i = 0; i < 8; i++) {
    pName[1] = '0' + i;
    m_D[i] = new IOPIN(pName);
    addSymbol(m_D[i]);
  }

  m_Ds = new IOPIN("Ds");
  addSymbol(m_Ds);
  m_Q = new IO_bi_directional("Q7");
  addSymbol(m_Q);
  m_Q->setDriving(true);
  m_Qbar = new IO_bi_directional("nQ7");
  addSymbol(m_Qbar);
  m_Qbar->setDriving(true);
  m_enable = new Enable("CE", this);
  addSymbol(m_enable);
  m_clock  = new Clock("CP", this);
  addSymbol(m_clock);
  m_strobe = new Strobe("PL", this);
  addSymbol(m_strobe);
}


TTL165::~TTL165()
{
  for (int i = 0; i < 8; i++) {
    removeSymbol(m_D[i]);
  }

  delete [] m_D;
  removeSymbol(m_Ds);
  removeSymbol(m_Q);
  removeSymbol(m_Qbar);
  removeSymbol(m_enable);
  removeSymbol(m_clock);
  removeSymbol(m_strobe);
}


void TTL165::setClock(bool bNewClock)
{
  // Clock shifts the shift register on rising edge, if CE pin is low and PL is high
  if (bNewClock && !m_bClock && !m_enable->getDrivenState() && m_strobe->getDrivenState()) {
    // Move the shift register left and out.
    sreg <<= 1;

    if (m_Ds->getDrivenState()) {
      sreg |= 0x01;
    }

    m_Q->putState((sreg & 0x80) != 0);
    m_Qbar->putState((sreg & 0x80) == 0);
  }

  m_bClock = bNewClock;
}


void TTL165::setEnable(bool /* bNewEnable */ )
{
  // This is the clock enable pin on this device. No explicit action needed on change.
}


void TTL165::setStrobe(bool bNewStrobe)
{
  // Strobe copies the contents of the parallel inputs into the shift register
  if (bNewStrobe && !m_bStrobe) {
    update_state();
  }

  if (!bNewStrobe) {
    m_Q->putState(m_D[7]->getDrivenState());
    m_Qbar->putState(!m_D[7]->getDrivenState());
  }

  m_bStrobe = bNewStrobe;
}


void TTL165::update_state()
{
  unsigned short ss = 0;

  for (int i = 0; i < 8; i++) {
    if (m_D[i]->getDrivenState()) {
      ss |= 1 << i;
    }
  }

  sreg = ss;
}


void TTL165::create_iopin_map()
{
  package = new Package(16);
  package->assign_pin(1, m_strobe);
  package->assign_pin(2, m_clock);
  package->assign_pin(3, m_D[4]);
  package->assign_pin(4, m_D[5]);
  package->assign_pin(5, m_D[6]);
  package->assign_pin(6, m_D[7]);
  package->assign_pin(7, m_Qbar);
  package->assign_pin(9, m_Q);
  package->assign_pin(10, m_Ds);
  package->assign_pin(11, m_D[0]);
  package->assign_pin(12, m_D[1]);
  package->assign_pin(13, m_D[2]);
  package->assign_pin(14, m_D[3]);
  package->assign_pin(15, m_enable);
}


}
