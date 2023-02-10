/*
   Copyright (C) 2006 Scott Dattalo

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
  i2c.cc

  I2C Module for gpsim

  OVERVIEW:

  The I2C Module is a plugin module for gpsim (the gnupic simulator)
  that implements the I2C protocol.


  BACKGROUND:

  <describe the I2C protocol>


  Application Layer
  -----------------

  The application layer is responsible for interpreting and generating
  packets.

  Physical Layer:
  --------------

  The physical layer consists of two wires that model the I2C bus

    SCL     - Open Collector
    SDA     - Open Collector

  Changes on these lines due to external stimuli will be propogated up
  to the Datalink Layer. Similarly, the Datalink layer will relay changes
  in the application layer to the I/O pins


  Implementation Details
  ----------------------

  The I2C module is designed to be a dynamic library that is loaded
  by gpsim. A typical scenario would be to start gpsim, load a
  processor, load the I2C module and connect the I2C module to the
  processor. The I2C module communicates to the simulator through 3
  different mechanisms. First, the I/O pins of the I2C module are
  manipulated; e.g. the SCL and SDA lines swing low and high. This
  is a low level physical-type interface. Second, gpsim has a callback
  mechanism tied into it's real-time timer. The I2C module will set
  'callback breakpoints' to occur at specific instances of
  time. Finally, the last method of communication is through simulator
  attributes.

  *** FIXME ***

  gpsim is a single-threaded software application. This makes it
  somewhat difficult to implement the state machines in the I2C
  module. So to get around this problem, the I2C module will spawn a
  thread for the application layer. In essence, this thread will allow
  a set of functions to be written that can send and receive byte
  streams. pThread mutexs will ensure that this thread safely
  communicates with the I2C state machines that run in the context of
  the simulator thread.


*/

//------------------------------------------------------------------------
// I2C State Machines
//
// The I2C state machines are responible for the physical layer of the I2CMaster.
// Here is the general timing of an I2C transaction:
//
//    ___    ___ ___ ___ ___ ___ ___ ___           ___ ___ ___ ___ ___ ___ ___ ___ ___    __
// SDA   \__/___X___X___X___X___X___X___\_________/___X___X___X___X___X___X___X___/   \__/
//       S    A7  A6  A5  A4  A3  A2  A1 R/W ACK    D7  D6  D5  D4  D3  D2  D1  D0 /ACK  P
//    ____    _   _   _   _   _   _   _   _   _     _   _   _   _   _   _   _   _   _   ____
// SCL    \__/1\_/2\_/3\_/4\_/5\_/6\_/7\_/8\_/9\___/1\_/2\_/3\_/4\_/5\_/6\_/7\_/8\_/9\_/
//
// S = Start
// P = Stop
// Ai - Address bits
// Di - Data bits
// R/W - Read or Write. A low means write, i.e. transmission from master to slave
// ACK - Acknowledge. The slave holds SDA low to acknowledge the previous transfer
//
// The gpsim I2C controller breaks the transaction down into smaller
// pieces and utilizes state machines for each piece. At the lowest
// level, there are three state machines: Start-bit, Stop-bit, and
// Data-bit. At the next higher level, is the byte transfer state
// machine. And finally, at the highest level is the stream transfer
// state machine.
//
// The start, stop, and data bit state machines use combinations of
// timing events and I/O events as inputs for state transitions. For
// example, a start bit begins with the bus idle (SDA and SCL both
// high) and then with the master driving SDA low. There could be a
// delay between SDA being driven low and it actually falling low
// (e.g. excessive bus capacitamce or bus hardware error). So the
// state machine doesn't make a transition into the start state until
// SDA is actually falls low. Similarly, when a bit is being
// transmitted, the I2C specification requires certain setup and hlod
// times between SDA and SCL. A gpsim cycle callback ensures these
// timings are met and the state machine transitions only after the
// callback triggers.
//
//
//  Start bit State Machine:
//     ____________________
// SDA                     \_____________
//     __________________________________
// SCL
//     idle | sendingStart | Start
//
// The host controller is notified 1uS after SDA goes low.
//
//
//  Data bit transfer State Machine:
//         _____________________________________ __________________________
//  bus SDA_____________________________________X__________________________
//         ____________________                                    ________
//  bus SCL                    \__________________________________/
//         ________________________________ _______________________________
//      SDA________________________________X_______________________________
//         __________                                  ____________________
//      SCL          \________________________________/
//
//          TransferE  TransferA  TransferB  TransferC  TransferD  TransferE
//
// There are 5 states in the bit-transmit state machine:
//
//   TransferA -- SCL has been driven low and we're waiting for it to fall
//   TransferB -- SCL has fallen low. We're now waiting t_HD;DAT before driving SDA
//   TransferC -- SDA has been driven with new data. Now wait t_SU;DAT to release SCL
//   TransferD -- SCL has been released and we're waiting for it to rise
//   TransferE -- SCL has risen high. The bit has been transferred.
//
//
// Stop bit State Machine
// ----------------------
//
//         __________ __________                 _________
//  bus SDA__________X__________\_______________/
//         _______________________________________________
//  bus SCL__________X__________/
//         _____________________         _________________
//      SDA__________X__________\_______/
//         __________ ____________________________________
//      SCL__________X__________/
//
//         any state  Transfer    StopA   StopB  Idle
//                     SDA = 0
//
//  any state -  The stop bit can be initiated at any point. When the
//  request to send a stop bit is made, the state machine is forced
//  into the "transfer a bit state". The bit transferred is a
//  '0'. Once this transition completes, the state machine enters the
//  Stop bit state machine.
//
//  StopA - SCL is High SDA is Low. Waiting t_HD;DAT before driving SDA high
//  StopB - SDA has been released and we're waiting for it to rise.
//  Idle  - SDA has been risen high. The master is notified
//



#include <iostream>
#include <cstdio>

#include <config.h>

#include "../src/stimuli.h"
#include "../src/modules.h"
#include "../src/gpsim_time.h"

#include "../src/ui.h"
#include "../src/value.h"

#include "i2c.h"

//using namespace gpsim;
//using namespace I2C_Module

namespace I2C_Module {

//#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("%5d:%s ",__LINE__,__FUNCTION__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

//static int verbose =1;
#define Vprintf(arg) { if (verbose) {printf("%s:%d ",__FILE__,__LINE__); printf arg;} }


#ifndef PRINTF_INT64_MODIFIER
#ifndef WIN32
#define PRINTF_INT64_MODIFIER "ll"
#endif
#endif

FILE *g_TraceOut = stdout;

// A pointer to the simulator's global defined cycle counter class:
// (there's an issue with DLL's directly accessing globally declared
// objects, so we'll use an accessor function to get a pointer to them
// [or in this case, it])

Cycle_Counter *gcycles = nullptr;

//----------------------------------------

#if 0 // defined but not used
static double get_time()
{
  double t = (double) gcycles->get();
  t /= 2.0;
  double r = floor(t / 1e6);
  t -= r * 1e6;
  return t;
}
#endif


//------------------------------------------------------------------------
#define  isSCLlow()  (m_pSCL->getDrivenState() == false)
#define  isSCLhigh() (m_pSCL->getDrivenState() == true)
#define  isSDAlow()  (m_pSDA->getDrivenState() == false)
#define  isSDAhigh() (m_pSDA->getDrivenState() == true)


//------------------------------------------------------------------------
// I2CPin
//
// An I2C I/O pin is a bidirectional I/O pin that has a strong driver
// for a low state, but a weak driver for the high state. There are
// several ways to model that behavior in gpsim. The way chosen here
// is to derive from the IO_open_collector class and to modify the
// high and low drive output impedances.
//
//

class I2C_PIN : public IO_open_collector {
public:
  I2C_PIN(I2CMaster *pMaster, const char *_name)
    : IO_open_collector(_name), m_pI2Cmaster(pMaster)
  {
    bDrivingState = true;
    bDrivenState = true;
    update_direction(IO_bi_directional::DIR_OUTPUT, true);
    // Set the pullup resistance to 10k ohms:
    set_Zpullup(10e3);
    update_pullup('1',    // Turn on the pullup resistor.
                  false); // Don't update the node. (none is attached).
  }

  void debug()
  {
    std::cout << name()
         << " digital_state=" << (getDrivingState() ? "high" : "low")
         << " Vth=" << get_Vth()
         << " Zth=" << get_Zth()
         << " driving=" << (getDriving() ? "true\n" : "false\n");
  }

  bool isLow()
  {
    return bDrivingState == false && bDrivenState == false;
  }
  bool isHigh()
  {
    return bDrivingState == true  && bDrivenState == true;
  }

  void setDrivingState(bool new_state)
  {
    bDrivingState = new_state;
    //bDrivenState = new_state;

    if (snode) {
      snode->update();
    }
  }

protected:
  // Parent module
  I2CMaster *m_pI2Cmaster;
};


//------------------------------------------------------------------------

class I2C_SDA_PIN : public I2C_PIN {
public:
  I2C_SDA_PIN(I2CMaster *pMaster, const char *_name)
    : I2C_PIN(pMaster, _name)
  {
  }

  virtual void setDrivenState(bool new_dstate)
  {
    bool diff = new_dstate ^ bDrivenState;
    Dprintf(("I2C SDA setDrivenState %d diff %d\n", new_dstate, diff));

    if (m_pI2Cmaster && diff) {
      bDrivenState = new_dstate;
      m_pI2Cmaster->new_sda_edge(new_dstate);
    }
  }
};


//------------------------------------------------------------------------
class I2C_SCL_PIN : public I2C_PIN {
public:
  I2C_SCL_PIN(I2CMaster *pMaster, const char *_name)
    : I2C_PIN(pMaster, _name)
  {
  }

  virtual void setDrivenState(bool new_state)
  {
    bool diff = new_state ^ bDrivenState;
    Dprintf(("I2C SCL setDrivenState %d\n", new_state));

    if (m_pI2Cmaster && diff) {
      bDrivenState = new_state;
      m_pI2Cmaster->new_scl_edge(bDrivenState);
    }
  }
};


//------------------------------------------------------------------------
// I2C attributes
//
// The I2C attributes expose an interface to the I2C module. Each attribute
// can be treated as a variable that a user (or a script) can access.
// There are two C++ methods used for assigning new values. The first
// is the 'set()' method defined in the Value base class. This is the one
// used whenever the user assigns values. The other method is setFromMaster().
// This used whenever the value is changed by the I2C state machine.
//
//------------------------------------------------------------------------


// I2C_TxBuffer
//  Data written to this attribute will get transmitted to the client (TouchPad)
//
class I2C_TxBuffer : public Integer {
  I2CMaster *i2c;

public:
  explicit I2C_TxBuffer(I2CMaster *);

  virtual void set(int64_t);
  void setFromMaster(int64_t);
};


// I2C_TxReady
//  This read-only attribute let's the user know if the TxBuffer is ready
//  to accept another byte.

class I2C_TxReady : public Boolean {
public:
  explicit I2C_TxReady(I2CMaster *);

  virtual void set(bool);
  void setFromMaster(bool);
};


// I2C_RxBuffer
//  Holds the last byte received from the client.

class I2C_RxBuffer : public Integer {
public:
  explicit I2C_RxBuffer(I2CMaster *);

  virtual void set(int64_t);
  void setFromMaster(int64_t);
};


// I2C_RxSequence
//  gpsim attaches a sequence number to every received byte. This
//  attribute allows the user to query that number.

class I2C_RxSequence : public Integer {
public:
  explicit I2C_RxSequence(I2CMaster *);

  virtual void set(int64_t);
  void setFromMaster(int64_t);
};


// I2C_Send7BitAddress
class I2C_Send7BitAddress : public Integer {
  I2CMaster *i2c;

public:
  explicit I2C_Send7BitAddress(I2CMaster *);

  virtual void set(int64_t);
  void setFromMaster(int64_t);
};


// I2C_Stop
class I2C_Stop : public Boolean {
  I2CMaster *i2c;

public:
  explicit I2C_Stop(I2CMaster *);

  virtual void set(bool);
  void setFromMaster(bool);
};


// I2C_Address
class I2C_Address : public Integer {
public:
  explicit I2C_Address(I2CMaster *);

  virtual void set(int64_t);
  // void setFromMaster(int64_t);
};


// I2C_Address
class I2C_Debug : public Integer {
  I2CMaster *i2c;

public:
  explicit I2C_Debug(I2CMaster *);

  virtual void set(int64_t);
  // void setFromMaster(int64_t);
};


//------------------------------------------------------------------------
//
I2CMaster::I2CMaster(const char *_name)
  : TriggerObject(), Module(_name),
    m_bitCount(0), m_command(0),
    m_xfr_data(0),
    m_TxData(0),
    m_nextBit(false),
    future_cycle(0),
    m_pI2CModule(nullptr),
    m_uState(eI2CIdle),
    m_mState(eI2CIdleBus),
    m_MSBmask(1 << 8)
{
  // Transmit and Receive propogation delays
  // The units are simulation cycles.
  tClkToData = 10;
  tClkToSample = 10;
  m_pSCL = new I2C_SCL_PIN(this, "scl");
  addSymbol(m_pSCL);
  m_pSDA = new I2C_SDA_PIN(this, "sda");
  addSymbol(m_pSDA);
  mTxByte     = new I2C_TxBuffer(this);
  mTxReady    = new I2C_TxReady(this);
  mRxByte     = new I2C_RxBuffer(this);
  mRxSequence = new I2C_RxSequence(this);
  mSend7BitAddress = new I2C_Send7BitAddress(this);
  mStop = new I2C_Stop(this);
  mAddress = new I2C_Address(this);
  mDebug = new I2C_Debug(this);
  //initializeAttributes();
  addSymbol(mTxByte);
  addSymbol(mTxReady);
  addSymbol(mRxByte);
  addSymbol(mRxSequence);
  addSymbol(mSend7BitAddress);
  addSymbol(mStop);
  addSymbol(mAddress);
  addSymbol(mDebug);
  // We may also want a logging function...
}


I2CMaster::~I2CMaster()
{
  removeSymbol(mTxByte);
  removeSymbol(mTxReady);
  removeSymbol(mRxByte);
  removeSymbol(mRxSequence);
  removeSymbol(mSend7BitAddress);
  removeSymbol(mStop);
  removeSymbol(mAddress);
  removeSymbol(mDebug);
  removeSymbol(m_pSCL);
  removeSymbol(m_pSDA);
  delete mTxByte;
  delete mTxReady;
  delete mRxByte;
  delete mRxSequence;
  delete mSend7BitAddress;
  delete mStop;
  delete mAddress;
  delete mDebug;
}


void I2CMaster::startIdle()
{
  m_pSDA->setDrivingState(true);
  m_pSCL->setDrivingState(true);
  m_uState = eI2CIdle;
  setNextMacroState(eI2CIdleBus);
}


bool I2CMaster::checkSDA_SCL(bool bSDA, bool bSCL)
{
  Dprintf((" SDA %d--%d  SCL %d--%d\n",
           m_pSDA->getDrivenState(), bSDA,
           m_pSCL->getDrivenState(), bSCL));

  if (m_pSCL->getDrivenState() == bSCL &&
      m_pSDA->getDrivenState() == bSDA) {
    Dprintf((" Match\n"));
    return true;
  }

  startIdle();
  return false;
}


//------------------------------------------------------------------------
// callback() - timer callbacks
//
// The I2C state machines insert wait states and set timeouts. When
// these trigger, control is sent here.
//

void I2CMaster::callback()
{
  Dprintf(("\n"));
  debug();
  // We get here because at some point in the past
  // we made the request to receive control at this point
  // in time. So find out what it was:
  future_cycle = 0;

  switch (m_uState) {
  case eI2CStartB:
    if (checkSDA_SCL(false, true)) {
      setNextMicroState(eI2CStartC, 1000);
      m_pSCL->setDrivingState(false);
      return;
    }

    break;

  case eI2CTransferB:

    // First half of bit transfer where we're driving the clock
    // If SCL high, then there's something clamping it high (that's a
    // bus error).
    // If SCL is low then we'll drive out the new data bit and wait
    // a few uSec before driving SCL high again
    if (isSCLlow()) {
      setNextMicroState(eI2CTransferC, 5);
      m_pSDA->setDrivingState(m_nextBit);
      return;
    }

  case eI2CTransferC:

    // Second half of bit transfer. We've just placed the new data bit
    // onto SDA. SCL should still be low. If it isn't, then there's
    // something clamping it high (and that's a bus error).
    // If SCL is low, then we'll drive it high and set a 1mSec time
    // out. When SCL rises, we'll capture the edge and advance the
    // state machine.
    if (isSCLlow()) {
      setNextMicroState(eI2CTransferD, 1000);
      m_pSCL->setDrivingState(true);
      return;
    }

  case eI2CTransferE:
    if (isSCLhigh()) {
      setNextMicroState(eI2CTransferA, 1000);
      m_pSCL->setDrivingState(false);
      return;
    }

  case eI2CStopA:
    if (checkSDA_SCL(false, true)) {
      setNextMicroState(eI2CStopB, 1000);
      m_pSDA->setDrivingState(true);
    }

  default:
    // Disable the bus
    ;
  }

  // We reach here if either there was a timeout or
  // an error in the state machine.
  startIdle();
}


void I2CMaster::callback_print()
{
  std::cout << "I2CMaster " << CallBackID << '\n';
}


//------------------------------------------------------------------------
// readBit()
//
// Should only be called when SCL is caught rising high.
//
// readBit() shifts the TransferData register left 1 position and
// copies the state of SDA to the LSB position. The MSB shifted out
// becomes the nextBit that will be transmitted (that bit is written
// to SDA later on).
//

bool I2CMaster::readBit()
{
  if (m_bitCount) {
    m_xfr_data <<= 1;
    m_xfr_data |= m_pSDA->getDrivenState() ? 1 : 0;
    m_bitCount--;
    m_nextBit = (m_xfr_data & m_MSBmask) == m_MSBmask;
    Dprintf(("I2CMaster SCL : Rising edge, data in=%u, nextOut=%d bit_count=%u\n",
             m_xfr_data & 1, m_nextBit, m_bitCount));
    return true;
  }

  return false;
}


const char *I2CMaster::microStateName(eI2CMicroState state)
{
  switch (state) {
  case eI2CIdle:
    return "eI2CIdle";

  case eI2CStartA:
    return "eI2CStartA";

  case eI2CStartB:
    return "eI2CStartB";

  case eI2CStartC:
    return "eI2CStartC";

  case eI2CBusy:
    return "eI2CBusy";

  case eI2CTransferA:
    return "eI2CTransferA";

  case eI2CTransferB:
    return "eI2CTransferB";

  case eI2CTransferC:
    return "eI2CTransferC";

  case eI2CTransferD:
    return "eI2CTransferD";

  case eI2CTransferE:
    return "eI2CTransferE";

  case eI2CStopA:
    return "eI2CStopA";

  case eI2CStopB:
    return "eI2CStopB";

  default:
    ;
  }

  return "eI2Cunknown";
}


const char *I2CMaster::macroStateName(eI2CMacroState state)
{
  switch (state) {
  case eI2CStop:
    return "eI2CStop";

  case eI2CTransfer:
    return "eI2CTransfer";

  case eI2CMaster:
    return "eI2CMaster";

  case eI2CSlave:
    return "eI2CSlave";

  case eI2CIdleBus:
    return "eI2CIdleBus";

  default:
    ;
  }

  return "eI2Cunknown";
}


void I2CMaster::debug()
{
#if defined(DEBUG)
  std::cout <<
       " SDA:" << m_pSDA->getDrivenState() <<
       " SCL:" << m_pSCL->getDrivenState() <<
       " microstate:" << microStateName(m_uState) <<
       " macrostate:" << macroStateName(m_mState) <<
       '\n';
  std::cout << " xfr_data:" << std::hex << m_xfr_data
       << " bits:" << m_bitCount
       << " next bit:" << m_nextBit
       << " fc: 0x" << future_cycle
       << '\n';
#endif
}


//------------------------------------------------------------------------
// new_scl_edge ( bool direction )
//
// Handle SCL transitions
//

void I2CMaster::new_scl_edge(bool direction)
{
  int curBusState = m_uState;

  if (verbose) {
    Vprintf(("I2CMaster::new_scl_edge: %d\n", direction));
    debug();
  }

  if (direction) {
    // Rising edge
    switch (m_uState) {
    case eI2CTransferD:
      if (readBit()) {
        setNextMicroState(eI2CTransferE, 5);

      } else if (m_mState == eI2CTransfer) {
        transferCompleted();

      } else if (m_mState == eI2CStop) {
        setNextMicroState(eI2CStopA, 5);
      }

      break;

    case eI2CStopB:

      // Stop is complete.
      if (m_mState == eI2CStop) {
        setNextMacroState(eI2CIdleBus);
        stopCompleted();
      }

    default:
      ;
    }

  }  else  {
    // Falling edge
    Dprintf(("I2CMaster SCL : Falling edge,data=%d\n ", m_nextBit));
    debug();

    switch (m_uState) {
    case eI2CIdle :
      m_pSDA->setDrivingState(true);
      break;

    case eI2CStartC:
    case eI2CStartB:
      setNextMicroState(eI2CTransferC, 1000);
      // If the clock went low because of someone else, still clamp.
      m_pSCL->setDrivingState(false);
      // Start bit has been sent. SDA and SCL are both low.
      // hold the start state for a short moment
      startCompleted();
      break;

    case eI2CTransferA:

      // We're driving the clock and have just managed to pull it
      // low. Now set up to drive the next data bit (if we're reading
      // then the next data bit is a '1').
      if (m_bitCount) {
        setNextMicroState(eI2CTransferB, 5);

      } else {
        transferCompleted();
      }

      break;

    default :
      m_pSDA->setDrivingState(true);     // Release the bus
      break;
    }
  }

  if ((bool)verbose && m_uState != curBusState) {
    Vprintf(("I2C_EE::new_scl_edge() new bus state = %d\n", m_uState));
    debug();
  }
}


//------------------------------------------------------------------------
// new_sda_edge ( bool direction )
//
// Handle SDA transitions
//

void I2CMaster::new_sda_edge(bool direction)
{
  Dprintf(("I2CMaster::new_sda_edge: direction:%d\n", direction));
  debug();

  if (m_pSCL->getDrivenState()) {
    // SCL is high.
    // If SDA just went low, then we caught a start
    // If SDA just went high, then we caught a stop.
    if (direction) {
      // stop bit
      Dprintf(("I2CMaster SDA : Rising edge in SCL high => stop bit\n"));
      m_uState = eI2CIdle;

    } else {
      // start bit
      Dprintf(("I2CMaster SDA : Falling edge in SCL high => start bit\n"));

      if (m_uState != eI2CStartA) {
        Dprintf(("             Another Master has started a transaction\n"));
        // Release SDA and listen to the address
        m_pSDA->setDrivingState(true);
        m_uState = eI2CListenToAddress;

      } else {
        // Wait 1us before notifying the module.
        setNextMicroState(eI2CStartB, 5);
        m_bitCount = 0;
        m_xfr_data = 0;
      }
    }

  } else {
    // SCL is low - SDA changed states. We need to make sure the SDA
    // state matches the SDA state we're driving. If they're different
    // then that means some other I2C device changed the SDA
    // state. We need to wait until SCL changes states before
    // declaring some other master has control.
    //if (direction != m_nextBit)
    //  cout << "Warning SDA unexpectedly changed to: " << direction << endl;
    // Another thing we can do is record the time when SDA changed.
    // This could be useful for recording slave response times.
    // For now, we do nothing:
    ;
  }
}


void I2CMaster::reset(RESET_TYPE )
{
  if (future_cycle) {
    gcycles->clear_break(this);
    future_cycle = 0;
  }

  startIdle();
#if defined(DEBUG)
  std::cout << "I2CMaster reset\n";
  debug();
#endif
}


//------------------------------------------------------------------------
// sendStart
// If the bus is idle, send a start bit.
//

I2CMaster::eI2CResult I2CMaster::sendStart()
{
  if (m_uState == eI2CIdle) {
    // Drive SDA low and set a 1mS timeout
    setNextMicroState(eI2CStartA, 1000);
    m_pSDA->setDrivingState(false);
    return eI2CResSuccess;
  }

  return eI2CResFailed;
}


//------------------------------------------------------------------------
// sendStop() - unconditionally initiate the 'stop' state machine
//
// Depending on the current state of the I2C bus, the stop state
// machine will transition the I2C bus so that a stop bit can be
// sent. In all cases, the bus needs to be in the state where SCL is
// high and SDA is low. The stop bit occurs when SDA is then driven
// high.
//
// There are 4 possibilities:
// SCL=0 SDA=0 - need to drive SCL high
// SCL=0 SDA=1 - drive SDA low followed by SCL high
// SCL=1 SDA=0 - drive SDA high ==> this will be the stop bit
// SCL=1 SDA=1 - drive SCL low followed by SDA low then SCL high.
//

I2CMaster::eI2CResult I2CMaster::sendStop()
{
  // Can't send a stop if we're a slave.
  if (m_mState == eI2CStop || m_mState == eI2CSlave) {
    return eI2CResFailed;
  }

  setNextMacroState(eI2CStop);

  if (isSCLhigh()) { // && m_pSCL->getDrivingState()) {
    if (isSDAhigh()) {
      // SCL is high, SDA high.
      m_bitCount = 0;
      m_xfr_data = 0;
      m_nextBit = false;
      setNextMicroState(eI2CTransferA, 5);
      m_pSCL->setDrivingState(false);

    } else {
      // SCL is high, SDA low -- perfect.
      // Now we only need to drive SDA high and we're done.
      setNextMicroState(eI2CStopA, 5);
    }

  } else {
    if (isSDAhigh()) {
      // SCL low, SDA high
      // We need to drive SDA low followed by SCL high.
      setNextMicroState(eI2CTransferC, 5);
      m_pSDA->setDrivingState(false);

    } else {
      // SCL low, SDA low
      // We need to drive SCL high.
      setNextMicroState(eI2CTransferD, 5);
      m_pSCL->setDrivingState(true);
    }
  }

  return eI2CResSuccess;
}


//------------------------------------------------------------------------
// send8BitData(unsigned int data)
//
// The 'Transfer' state machine is initialized here. A transfer can
// only commence if we've seen a start (or have just completed a
// transfer earlier). SCL should be sitting low.

I2CMaster::eI2CResult I2CMaster::send8BitData(unsigned int data)
{
  Dprintf((" send8BitData:0x%x\n", data));

  if (isSCLlow()) {
    mStop->setFromMaster(false);
    setNextMacroState(eI2CTransfer);
    m_bitCount = 9;             // There are 8 data bits and 1 ACK bit
    m_xfr_data = (data << 1) | 1; // LSB = 1 ==> we'll release SDA
    // during ACK
    m_nextBit = (data & m_MSBmask) == m_MSBmask;
    // We could also remember how long it's been since SCL has gone
    // low and possibly wait a shorter amount of time.
    setNextMicroState(eI2CTransferB, 5);
  }

  return eI2CResFailed;
}


//------------------------------------------------------------------------
// wait_uSec - suspends the I2C state machine.
//
void I2CMaster::wait_uSec(unsigned int uSec)
{
  uint64_t fc = gcycles->get() + (uSec * 2);

  if (future_cycle) {
    gcycles->reassign_break(future_cycle, fc, this);

  } else {
    gcycles->set_break(fc, this);
  }

  future_cycle = fc;
}


//------------------------------------------------------------------------
//
void I2CMaster::setNextMicroState(eI2CMicroState nextState, unsigned int waitTime)
{
  Dprintf((" curr:%s next:%s\n", microStateName(m_uState), microStateName(nextState)));
  m_uState = nextState;
  wait_uSec(waitTime);
}


//------------------------------------------------------------------------
//
void I2CMaster::setNextMacroState(eI2CMacroState nextState)
{
  Dprintf((" curr:%s next:%s\n", macroStateName(m_mState), macroStateName(nextState)));
  m_mState = nextState;
  mStop->setFromMaster(m_mState == eI2CStop);
}


Module *I2CMaster::construct(const char *new_name)
{
  gcycles = &get_cycles();
  I2CMaster *pI2CMaster = new I2CMaster(new_name);
  pI2CMaster->create_iopin_map();
  return pI2CMaster;
}


//------------------------------------------------------------------------

void I2CMaster::send7BitAddress(unsigned int addr)
{
  Dprintf(("\n"));

  if (eI2CResSuccess == sendStart()) {
    mTxByte->setFromMaster(addr << 1);
  }
}


//------------------------------------------------------------------------

void I2CMaster::startCompleted()
{
  Dprintf(("\n"));
  send8BitData(mTxByte->getVal());
}


//------------------------------------------------------------------------
void I2CMaster::stopCompleted()
{
  Dprintf(("\n"));
  mStop->setFromMaster(true);
}


//------------------------------------------------------------------------

void I2CMaster::transferCompleted()
{
  Dprintf(("\n"));
}


//--------------------------------------------------------------
// create_iopin_map
//
//  This is where the information for the I2C Master's package is defined.
// Specifically, the I/O pins of the module are created.

void I2CMaster::create_iopin_map()
{
  // Define the physical package. It has two pins
  create_pkg(2);
  assign_pin(1, m_pSCL);
  assign_pin(2, m_pSDA);
}


//========================================================================
//    I2C Attributes

I2C_TxBuffer::I2C_TxBuffer(I2CMaster *_i2c)
  : Integer("tx", 0, "I2C Transmit Register - byte currently transmitting"), i2c(_i2c)
{
}


void I2C_TxBuffer::set(int64_t i)
{
  i &= 0xff;

  if (i2c) {
    i2c->send8BitData((int)i);
  }

  Integer::set(i);
}


void I2C_TxBuffer::setFromMaster(int64_t i)
{
  Integer::set(i);
}


//########################################
// TxREady - reflects the bus state - if the I2C bus is not owned by another
// master, then TxReady is true
I2C_TxReady::I2C_TxReady(I2CMaster *_i2c)
  : Boolean("tx_ready", false,
            "I2C Transmit Ready - a read-only register that is false only\n"
            "when some other master controls the I2C bus.")
{
}


void I2C_TxReady::set(bool )
{
}


void I2C_TxReady::setFromMaster(bool b)
{
  Boolean::set(b);
}


//########################################
I2C_RxBuffer::I2C_RxBuffer(I2CMaster *_i2c)
  : Integer("rx", 0, "I2C Receive Register - most recently received byte")
{
}


void I2C_RxBuffer::set(int64_t )
{
}


void I2C_RxBuffer::setFromMaster(int64_t i)
{
  Integer::set(i);
}


//########################################
I2C_RxSequence::I2C_RxSequence(I2CMaster *_i2c)
  : Integer("rx_sequence", 0, "I2C Receive Sequence number - increments on each recieved byte")
{
}


void I2C_RxSequence::set(int64_t )
{
}


void I2C_RxSequence::setFromMaster(int64_t i)
{
  Integer::set(i);
}


//########################################
I2C_Send7BitAddress::I2C_Send7BitAddress(I2CMaster *_i2c)
  : Integer("slaveaddr", 0, "I2C slave address - a write to this will send a slave address"), i2c(_i2c)
{
}


void I2C_Send7BitAddress::set(int64_t i)
{
  Dprintf(("setting addr to 0x%lx\n", i));
  Integer::set(i);
  int addr = (int)(i & 0xff);
  i2c->send7BitAddress(addr);
}


void I2C_Send7BitAddress::setFromMaster(int64_t i)
{
  Integer::set(i);
}


//########################################
I2C_Stop::I2C_Stop(I2CMaster *_i2c)
  : Boolean("stop", 0, "I2C stop - transmit a stop bit now"), i2c(_i2c)
{
}


void I2C_Stop::set(bool b)
{
  if (b) {
    Boolean::set(b);
    i2c->sendStop();
  }
}


void I2C_Stop::setFromMaster(bool b)
{
  Boolean::set(b);
}


//########################################
I2C_Address::I2C_Address(I2CMaster *_i2c)
  : Integer("addr", 0, "I2C master address - a write to this sets the master address")
{
}


void I2C_Address::set(int64_t )
{
}


//########################################
I2C_Debug::I2C_Debug(I2CMaster *_i2c)
  : Integer("debug", 0, "I2C debug - a write sets debug verbosity"), i2c(_i2c)
{
}


void I2C_Debug::set(int64_t )
{
  i2c->debug();
}


} // end of namespace I2C_Module
