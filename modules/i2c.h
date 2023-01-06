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

#ifndef MODULES_I2C_H_
#define MODULES_I2C_H_

#include "../src/gpsim_classes.h"
#include "../src/modules.h"
#include "../src/trigger.h"

class IO_open_collector;

namespace I2C_Module {

//------------------------------------------------------------------------
// I2C Attributes
class  I2C_TxBuffer;
class  I2C_TxReady;
class  I2C_RxBuffer;
class  I2C_RxSequence;
class  I2C_Send7BitAddress;
class  I2C_Stop;
class  I2C_Address;
class  I2C_Debug;
class  I2CMasterModule;


//------------------------------------------------------------------------
// I2C interface
//
// The pure virtual I2C interface class is used by the I2CMaster
// class to establish low-level communications.
class I2CInterface {
public:
  virtual ~I2CInterface()
  {
  }

  static  void *master_interface(void *);
  virtual void run_tests() = 0;
};


//------------------------------------------------------------------------
// I2CMaster
//
// The I2CMaster can either a) simulate a master interface or b) provide support
// for some external code that wishes to serve as a master.


class I2CMaster : public TriggerObject, public Module {
public:
  explicit I2CMaster(const char *_name);
  virtual ~I2CMaster();

  static Module *construct(const char *new_name);
  virtual void create_iopin_map();

  void reset(RESET_TYPE);
  enum eI2CResult {
    eI2CResAck,       // Slave acknowledged last transfer
    eI2CResNack,      // Slave did not acknowledge last transfer
    eI2CResSuccess,   // Last action succeeded
    eI2CResFailed,    // Last action failed.
    eI2CResCollision, //
  };

  /// There are only three things the I2CMaster hardware knows how
  /// to do: send start bits, send stop bits, transfer 8 data bits.
  /// Note that a read operation is performed when the byte '0xff'
  /// is written.

  eI2CResult sendStart();
  eI2CResult sendStop();
  eI2CResult send8BitData(unsigned int data);
  void send7BitAddress(unsigned addr);

  /// rising and falling SCL and SDA edges are captured:

  void new_scl_edge(bool direction);
  void new_sda_edge(bool direction);

  /// Breakpoint stuff
  virtual void callback();
  virtual void callback_print();
  virtual char const * bpName()
  {
    return "i2cmaster";
  }

  /// When the start, stop or transfer state machines then these
  /// functions are called. (Classes derived from this one can
  /// capture this).
  virtual void startCompleted();
  virtual void stopCompleted();
  virtual void transferCompleted();

  void debug();

  /// I/O pins for the I2C bus

  IO_open_collector *m_pSCL;
  IO_open_collector *m_pSDA;

protected:
  void wait_uSec(unsigned int uSec);
  void startIdle();
  bool checkSDA_SCL(bool bSDA, bool bSCL);

  unsigned int m_bitCount;   // Current bit number for either Tx or Rx
  unsigned int m_command;    // Most recent command received from I2C host
  unsigned int m_xfr_data;   // Transmit and receive shift register.
  unsigned int m_TxData;     // Next data byte to transmit
  bool         m_nextBit;    // Next bit to be sent

  uint64_t future_cycle;

  I2CMasterModule *m_pI2CModule;

private:
  enum eI2CMicroState {
    eI2CIdle = 0,

    // Micro states for Start

    eI2CStartA,       // Trying to drive SDA low to begin start
    eI2CStartB,       // Holding Start state  (SDA=0, SCL=1)
    eI2CStartC,       // Holding Start state  (SDA=0, SCL=0)
    eI2CListenToAddress, // Some other master initiated a start.
    eI2CBusy,            // Some other Master has control of the bus.

    // Micro states for Transfer
    eI2CTransferA,       // Transfer- drove SCL low, waiting for it to fall
    eI2CTransferB,       // Transfer- caught SCL low, waiting to update SDA
    eI2CTransferC,       // Transfer- update SDA, wait to drive SCL high
    eI2CTransferD,       // Transfer- drive SCL high, waiting for it to rise
    eI2CTransferE,       // Transfer- caught SCL high, wait before driving low

    // Micro states for Stop
    eI2CStopA,           // Stop - Waiting to drive SDA high.
    eI2CStopB,           // Stop - drove SDA high Waiting for it to go high.

  } m_uState;

  enum eI2CMacroState {
    //eI2CStart,           // In the process of sending a start bit.
    eI2CStop,            // In the process of sending a stop bit
    eI2CTransfer,        // In the prcoess of transferring a byte
    eI2CMaster,          // Bus is idle but we're in control.
    eI2CSlave,           // Bus is idle but we're not in control
    eI2CIdleBus          // Bus is idle and no one owns it

  } m_mState;

  const unsigned int m_MSBmask;   // MSB of transfer mask. Used in data transfers
  bool readBit();
  void setNextMicroState(eI2CMicroState nextState, unsigned int waitTime);
  void setNextMacroState(eI2CMacroState nextState);
  // Define propogation delays.

  // tClkToData = time between when the I2C clock goes
  // low and when the I2C module will update it's data
  // output line.

  unsigned int tClkToData;

  // tClkToSample - time between when the I2C clock goes
  // low and when the I2C module will sample the data
  // line.
  unsigned int tClkToSample;

  /// token - the I2C master interfaces to the I2C client. The
  /// client runs in a different thread. token synchronizes the
  /// two threads.

  // gpsim::Token token; not implemented...

  /// debug stuff
  const char *microStateName(eI2CMicroState);
  const char *macroStateName(eI2CMacroState);

  /// I2C attribute for the Transmit buffer. Writing a byte to
  /// this attribute initiates an I2C transmit.
  I2C_TxBuffer   *mTxByte;

  /// I2C attribute for transmit ready status. 'true' means the module
  /// is ready to transmit.

  I2C_TxReady    *mTxReady;

  /// I2C Receive buffer. This holds the last byte received.

  I2C_RxBuffer   *mRxByte;

  /// I2C Receive Sequence number. This increments everytime a byte is
  /// received.

  I2C_RxSequence *mRxSequence;

  /// I2C 7-bit Address: Writing to here initiates an I2C transaction
  I2C_Send7BitAddress *mSend7BitAddress;

  /// I2C 10-bit Address: Writing to here initiates an I2C transaction
  //I2C_Send10BitAddress *mSend10BitAddress;

  /// I2C Stop. Complete current transfer then issue a STOP bit
  I2C_Stop *mStop;

  /// I2C Module's address
  I2C_Address *mAddress;

  /// I2C debug
  I2C_Debug *mDebug;
};

}  // end of namespace


#endif // MODULES_I2C_H_
