/*
   Copyright (C) 1998 T. Scott Dattalo

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

#ifndef SRC_IOPORTS_H_
#define SRC_IOPORTS_H_

#include "registers.h"
#include "stimuli.h"

#include <stdio.h>
#include <string>
#include <vector>

class Module;
class PinModule;
class Processor;

///**********************************************************************/
///
/// I/O ports
///
/// An I/O port is collection of I/O pins. For a PIC processor, these
/// are the PORTA, PORTB, etc, registers. gpsim models I/O ports in
/// a similar way it models other registers; there's a base class from
/// which all specific I/O ports are derived. However, I/O ports are
/// special in that they're an interface between a processor's core
/// and the outside world. The requirements vary wildly from one processor
/// to the next; in fact they even vary dynamically within one processor.
/// gpsim attempts to abstract this interface with a set of base classes
/// that are responsible for routing signal state information. These
/// base classes make no attempt to decipher this information, instead
/// this job is left to the peripherals and stimuli connected to the
/// I/O pins and ports.
///
///
/// PinModule
///
/// Here's a general description of gpsim I/O pin design:
///
///    Data
///    Select  ======+
///                +-|-+  Outgoing
///    Source1 ===>| M |  data
///    Source2 ===>| U |=============+
///    SourceN ===>| X |             |
///                +---+             |    +-------+
///    Control                       +===>| IOPIN |
///    Select  ======+                    |       |
///                +-|-+  I/O Pin         |       |
///   Control1 ===>| M |  Direction       |       |<======> Physical
///   Control2 ===>| U |=================>|       |         Interface
///   ControlN ===>| X |                  |       |
///                +---+                  |       |
///    Sink Decode                   +===<|       |
///    Select  ======+               |    +-------+
///                +-|-+   Incoming  |
///    Sink1  <====| D |   data      |
///    Sink2  <====| E |<============|
///    SinkN  <====| C |
///                +---+
///
/// The PinModule models a Processor's I/O Pin. The schematic illustrates
/// the abstract description of the PinModule. Its job is to merge together
/// all of the Processor's peripherals that can control a processor's pin.
/// For example, a UART peripheral may be shared with a general purpose I/O
/// pin. The UART may have a transmit and receive pin and select whether it's
/// in control of the I/O pins. The uart transmit pin and the port's I/O pin
/// can both act as a source for the physical interface. The PinModule
/// arbitrates between the two. Similarly, the UART receive pin can be multiplexed
/// with a register pin. In this case, the PinModule will route signal
/// changes to both devices. Finally, a peripheral like the '622's comparators
/// may overide the output control. The PinModule again arbitrates.
///
///
/// PortModule
///
/// The PortModule is the base class for processor I/O ports. It's essentially
/// a register that contains an array of PinModule's.
///
///  Register               PortModule
///    |-> sfr_register         |
///             |               |
///             \------+--------/
///                    |
///                    +--> PortRegister
///                            |--> PicPortRegister


///------------------------------------------------------------
///
/// SignalControl  - A pure virtual class that defines the interface for
/// a signal control. The I/O Pin Modules will query the source's state
/// via SignalControl. The control is usually used to control the I/O pin
/// direction (i.e. whether it's an input or output...), drive value,
/// pullup state, etc.
/// When a Pin Module is through with the SignalControl, it will call
/// the release() method. This is primarily used to delete the SignalControl
/// objects.

class SignalControl
{
public:
    virtual ~SignalControl();  //// fixme
    virtual char getState() = 0;
    virtual void release() = 0;
};

///------------------------------------------------------------
/// PeripheralSignalSource - A class to interface I/O pins with
/// peripheral outputs.

class PeripheralSignalSource : public SignalControl
{
public:
    explicit PeripheralSignalSource(PinModule *_pin);
    virtual ~PeripheralSignalSource();

    void release() override;

    /// getState - called by the PinModule to determine the source state
    char getState() override;

    /// putState - called by the peripheral to set a new state
    virtual void putState(const char new3State);

    /// toggle - called by the peripheral to toggle the current state.
    virtual void toggle();

private:
    PinModule *m_pin;
    char m_cState;
};

///------------------------------------------------------------
/// PortModule - Manages all of the I/O pins associated with a single
/// port. The PortModule supplies the interface to the I/O pin's. It
/// is designed to handle a group of I/O pins. However, the low level
/// I/O pin processing is handled by PinModule objects contained within
/// the PortModule.

class PortModule
{
public:
    explicit PortModule(unsigned int numIopins);
    virtual ~PortModule();

    /// updatePort -- loop through update all I/O pins

    virtual void updatePort();

    /// updatePin -- Update a single I/O pin

    virtual void updatePin(unsigned int iPinNumber);

    /// updatePins -- Update several I/O pins

    virtual void updatePins(unsigned int iPinBitMask);

    /// updateUI -- convey pin state info to a User Interface (e.g. the gui).

    virtual void updateUI();

    /// addPinModule -- supply a pin module at a particular bit position.
    ///      Most of the low level I/O pin related processing will be handled
    ///      here. The PortModule per-pin helper methods below essentially
    ///      call methods in the PinModule to do the dirty work.
    ///      Each bit position can have only one PinModule. If multiple
    ///      modules are added, only the first will be used and the others
    ///      will be ignored.

    void           addPinModule(PinModule *, unsigned int iPinNumber);

    /// addSource -- supply a pin with a source of data. There may

    SignalControl *addSource(SignalControl *, unsigned int iPinNumber);

    /// addControl -- supply a pin with a data direction control

    SignalControl *addControl(SignalControl *, unsigned int iPinNumber);

    /// addPullupControl -- supply a pin with a pullup control

    SignalControl *addPullupControl(SignalControl *, unsigned int iPinNumber);

    /// addSink -- supply a sink to receive info driven on to a pin

    SignalSink    *addSink(SignalSink *, unsigned int iPinNumber);

    /// addPin -- supply an I/O pin. Note, this will create a default pin module
    ///           if one is not already created.

    IOPIN         *addPin(IOPIN *, unsigned int iPinNumber);


    /// getPin -- an I/O pin accessor. This returns the I/O pin at a particular
    ///           bit position.

    IOPIN         *getPin(unsigned int iPinNumber);

    /// operator[] -- PinModule accessor. This returns the pin module at
    ///               a particular bit position.

    PinModule &operator [] (unsigned int pin_number);

    PinModule * getIOpins(unsigned int pin_number);

    // set/get OutputMask which controls bits returned on I/O
    // port register get() call. Used to return 0 for  analog pins
    virtual void setOutputMask(unsigned int OutputMask)
    { mOutputMask = OutputMask; }
    virtual unsigned int getOutputMask()
    { return(mOutputMask); }

protected:
    unsigned int mNumIopins;
    unsigned int mOutputMask;

private:
    /// PinModule -- The array of PinModules that are handled by PortModule.

    std::vector<PinModule *> iopins;
};

///------------------------------------------------------------
/// PinModule - manages the interface to a physical I/O pin. Both
/// simple and complex I/O pins are handled. An example of a simple
/// I/O is one where there is a single data source, data sink and
/// control, like say the GPIO pin on a small PIC. A complex pin
/// is one that is multiplexed with peripherals.
///
/// The parent class 'PinMonitor', allows the PinModule to be
/// registered with the I/O pin. In other words, when the I/O pin
/// changes state, the PinModule will be notified.
#define ANALOG_TABLE_SIZE 3

class PinModule : public PinMonitor
{
public:
    PinModule();
    PinModule(PortModule *, unsigned int _pinNumber, IOPIN *new_pin = nullptr);
    virtual ~PinModule();

    /// updatePinModule -- The low level I/O pin state is resolved here
    /// by examining the direction and state of the I/O pin.

    virtual void updatePinModule();

    /// refreshPinOnUpdate - modal behavior. If set to true, then
    /// a pin's state will always be refreshed whenever the PinModule
    /// is updated. If false, then the pin is updated only if there
    /// is a detected state change.
    void refreshPinOnUpdate(bool bForcedUpdate);

    void setPin(IOPIN *);
    void clrPin() { m_pin = nullptr; }
    void setDefaultSource(SignalControl *);
    void setSource(SignalControl *);
    void setDefaultControl(SignalControl *);
    virtual void setControl(SignalControl *);
    void setPullupControl(SignalControl *);
    void setDefaultPullupControl(SignalControl *);

    char getControlState();
    char getSourceState();
    char getPullupControlState();
    unsigned int getPinNumber() { return m_pinNumber;}
    void AnalogReq(Register *reg, bool analog, const char *newName);
    // If active control not default, return it
    SignalControl *getActiveControl() {return (m_activeControl == m_defaultControl) ? nullptr : m_activeControl;}
    // If active source not default, return it
    SignalControl *getActiveSource() {return (m_activeSource == m_defaultSource) ? nullptr : m_activeSource;}

    IOPIN &getPin() { return *m_pin;}

    ///
    void setDrivenState(char) override;
    void setDrivingState(char) override;
    void set_nodeVoltage(double) override;
    void putState(char) override;
    void setDirection() override;
    void updateUI() override;

private:
    char          m_cLastControlState;
    char          m_cLastSinkState;
    char          m_cLastSourceState;
    char          m_cLastPullupControlState;

    SignalControl *m_defaultSource,  *m_activeSource;
    SignalControl *m_defaultControl, *m_activeControl;
    SignalControl *m_defaultPullupControl, *m_activePullupControl;

    IOPIN        *m_pin;
    PortModule   *m_port;
    unsigned int  m_pinNumber;
    bool          m_bForcedUpdate;
    Register     *m_analog_reg[ANALOG_TABLE_SIZE + 1];
    bool	  m_analog_active[ANALOG_TABLE_SIZE + 1];
};



///------------------------------------------------------------
class PortRegister : public sfr_register, public PortModule
{
public:
    PortRegister(Module *pCpu, const char *pName, const char *pDesc,
                 unsigned int numIopins, unsigned int enableMask);

    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;
    unsigned int get() override;
    unsigned int get_value() override;
    virtual void putDrive(unsigned int new_drivingValue);
    virtual unsigned int getDriving();
    virtual void setbit(unsigned int bit_number, char new_value);
    virtual void setEnableMask(unsigned int nEnableMask);
    IOPIN        *addPin(IOPIN *, unsigned int iPinNumber);
    IOPIN        *addPin(Module *mod, IOPIN *pin, unsigned int iPinNumber);

    unsigned int getEnableMask()
    {
        return mEnableMask;
    }
    void updateUI() override;

protected:
    unsigned int  mEnableMask;
    unsigned int  drivingValue;
    RegisterValue rvDrivenValue;
};

class PortSink : public SignalSink
{
public:
    PortSink(PortRegister *portReg, unsigned int iobit);

    virtual ~PortSink()
    {
    }

    void setSinkState(char) override;
    void release() override;

private:
    PortRegister *m_PortRegister;
    unsigned int  m_iobit;
};


// Base class to allow changing of IO pins
class apfpin
{
public:
    virtual void setIOpin(PinModule * pin, int arg = 0)
    {
        fprintf(stderr, "unexpected call afpin::setIOpin pin=%p %s arg=%d\n", pin, pin ? pin->getPin().name().c_str():"unknown", arg);
    }
};

class INTsignalSink;
class INTCON;

// class to support INT pin
class INT_pin : public apfpin
{
public:
    INT_pin(Processor *pCpu, INTCON *_intcon, int _intedg_index);

    void setIOpin(PinModule * pin, int arg = 0) override;
    virtual void setState(char new3State);

private:
    Processor *p_cpu;
    INTCON *p_intcon;
    int intedg_index;	// index for get_intedg(index)
    PinModule *m_PinModule;
    INTsignalSink *m_sink;
    int arg;
    bool OldState;
};

// ALTERNATE PIN LOCATIONS register
// set_pins is used to configure operation
class APFCON : public  sfr_register
{
public:
    APFCON(Processor *pCpu, const char *pName, const char *pDesc, unsigned int _mask);

    void put(unsigned int new_value) override;
    void set_pins(unsigned int bit, class apfpin *pt_apfpin, int arg,
                  PinModule *pin_default, PinModule *pin_alt);
    void set_ValidBits(unsigned int _mask) {mValidBits = _mask;}

private:
    unsigned int mValidBits;
    struct dispatch
    {
        class apfpin *pt_apfpin;	// pointer to pin setting function
        int          arg;	        // argument for pin setting function
        PinModule    *pin_default; // pin when bit=0
        PinModule    *pin_alt;	// pin when bit=1
    } dispatch[8];
};


#endif  // SRC_IOPORTS_H_

