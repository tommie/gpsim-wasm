/*
   Copyright (C) 1998,1999 Scott Dattalo
   Copyright (C) 2014,2022 Roy R. Rankin

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

#include <iostream>
#include <string>

#include "uart.h"
#include <assert.h>     // for assert
#include "gpsim_time.h"
#include "pir.h"        // for PIR
#include "processor.h"  // for Processor
#include "stimuli.h"    // for IOPIN, SignalSink
#include "trace.h"      // for Trace, trace

#define p_cpu ((Processor *)cpu)


//#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("%s:%d-%s() ",__FILE__,__LINE__,__FUNCTION__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

//--------------------------------------------------
//
//--------------------------------------------------
// Drive output for TX pin
class TXSignalSource : public SignalControl
{
public:
    explicit TXSignalSource(_TXSTA *_txsta)
        : m_txsta(_txsta)
    {
        assert(m_txsta);
    }
    ~TXSignalSource() { }
    char getState() override
    {
        return m_txsta->getState();
    }
    void release() override
    {
        m_txsta->releasePin();
    }
private:
    _TXSTA *m_txsta;
};

// Set TX pin to output
class TXSignalControl : public SignalControl
{
public:
    explicit TXSignalControl(_TXSTA *_txsta)
        : m_txsta(_txsta)
    { }
    ~TXSignalControl() { }
    char getState() override { return '0'; }
    void release() override
    {
        m_txsta->releasePin();
    }
private:
    _TXSTA *m_txsta;
};

// set Synchronous DT pin direction
class RCSignalControl : public SignalControl
{
public:
    explicit RCSignalControl(_RCSTA *_rcsta)
        : m_rcsta(_rcsta)
    { }
    ~RCSignalControl() { }
    char getState() override { return '0'; }
    //virtual char getState() { return m_rcsta->getDir(); }
    void release() override
    {
        m_rcsta->releasePin();
    }
private:
    _RCSTA *m_rcsta;
};

// Drive date of DT  pin when transmitting
class RCSignalSource : public SignalControl
{
public:
    explicit RCSignalSource(_RCSTA *_rcsta)
        : m_rcsta(_rcsta)
    {
        assert(m_rcsta);
    }
    ~RCSignalSource() { }
    char getState() override
    {
        return m_rcsta->getState();
    }
    void release() override
    {
        m_rcsta->releasePin();
    }
private:
    _RCSTA *m_rcsta;
};

//--------------------------------------------------
//
//--------------------------------------------------

// Report state changes on incoming RX pin
class RXSignalSink : public SignalSink
{
public:
    explicit RXSignalSink(_RCSTA *_rcsta)
        : m_rcsta(_rcsta)
    {
        assert(_rcsta);
    }

    void setSinkState(char new3State) override { m_rcsta->setState(new3State); }
    void release() override { delete this; }
private:
    _RCSTA *m_rcsta;
};

//--------------------------------------------------
//
//--------------------------------------------------

// Report state changes on incoming Clock pin for Synchronous slave mode
class CLKSignalSink : public SignalSink
{
public:
    explicit CLKSignalSink(_RCSTA *_rcsta)
        : m_rcsta(_rcsta)
    {
        assert(_rcsta);
    }

    void setSinkState(char new3State) override { m_rcsta->clock_edge(new3State); }
    void release() override { delete this; }
private:
    _RCSTA *m_rcsta;
};

//-----------------------------------------------------------
_RCSTA::_RCSTA(Processor *pCpu, const char *pName, const char *pDesc, USART_MODULE *pUSART)
    : sfr_register(pCpu, pName, pDesc),
      rcreg(nullptr), spbrg(nullptr),
      txsta(nullptr), txreg(nullptr), sync_next_clock_edge_high(false),
      rsr(0), bit_count(0), rx_bit(0), sample(0),
      state(_RCSTA::RCSTA_DISABLED), sample_state(0),
      future_cycle(0), last_cycle(0),
      mUSART(pUSART),
      m_PinModule(nullptr), m_sink(nullptr), m_cRxState('?'),
      SourceActive(false), m_control(nullptr), m_source(nullptr), m_cTxState('\0'),
      m_DTdirection('0'), bInvertPin(false),
      old_clock_state(true)
{
    assert(mUSART);
}

_RCSTA::~_RCSTA()
{
    if (SourceActive && m_PinModule)
    {
        m_PinModule->setSource(nullptr);
        m_PinModule->setControl(nullptr);
    }

    delete m_source;
    delete m_control;
}

//-----------------------------------------------------------
_TXSTA::_TXSTA(Processor *pCpu, const char *pName, const char *pDesc, USART_MODULE *pUSART)
    : sfr_register(pCpu, pName, pDesc), txreg(nullptr), spbrg(nullptr),
      rcsta(nullptr), tsr(0), bit_count(0),
      mUSART(pUSART),
      m_PinModule(nullptr),
      m_source(nullptr),
      m_control(nullptr),
      m_clkSink(nullptr),
      SourceActive(false),
      m_cTxState('?'),
      bInvertPin(false)
{
    assert(mUSART);
}

_TXSTA::~_TXSTA()
{
    if (SourceActive && m_PinModule)
    {
        m_PinModule->setSource(nullptr);
        m_PinModule->setControl(nullptr);
    }

    if (m_source) delete m_source;
    if (m_control) delete m_control;
}

//-----------------------------------------------------------
_RCREG::_RCREG(Processor *pCpu, const char *pName, const char *pDesc, USART_MODULE *pUSART)
    : sfr_register(pCpu, pName, pDesc),
      oldest_value(0), fifo_sp(0),  mUSART(pUSART), m_rcsta(nullptr)
{
    assert(mUSART);
}

_TXREG::_TXREG(Processor *pCpu, const char *pName, const char *pDesc, USART_MODULE *pUSART)
    : sfr_register(pCpu, pName, pDesc), m_txsta(nullptr), m_rcsta(nullptr),
      mUSART(pUSART), full(false)
{
    assert(mUSART);
}


_BAUDCON::_BAUDCON(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc),
      txsta(nullptr), rcsta(nullptr)
{
}

_SPBRG::_SPBRG(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc),
      txsta(nullptr), rcsta(nullptr), brgh(nullptr), baudcon(nullptr),
      start_cycle(0), last_cycle(0),
      future_cycle(0), running(false), skip(0)
{
}


_SPBRGH::_SPBRGH(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc), m_spbrg(nullptr)
{
}

//-----------------------------------------------------------
// TXREG - USART Transmit Register
//
// writing to this register causes the PIR1::TXIF bit to clear.
// my reading of the spec is this happens at the end of the next pic
// instruction. If the shift register is empty the bit will then be set
// one pic instruction later. Otherwise the bit is set when the shift
// register empties.  RRR 10/2014


void _TXREG::put(unsigned int new_value)
{

    trace.raw(write_trace.get() | value.get());
    value.put(new_value & 0xff);

    Dprintf(("txreg just got a new value:0x%x\n",new_value));
    assert(m_txsta);
    assert(m_rcsta);

    // The transmit register has data,
    // so clear the TXIF flag

    full = true;
    get_cycles().set_break(get_cycles().get() + 1, this);

    if(m_txsta->bTRMT() && m_txsta->bTXEN())
    {
        // If the transmit buffer is empty and the transmitter is enabled
        // then transmit this new data now...

        get_cycles().set_break(get_cycles().get() + 2, this);
        if (m_txsta->bSYNC())
            m_rcsta->sync_start_transmitting();
        else
            m_txsta->start_transmitting();
    }
    else if(m_txsta->bTRMT() && m_txsta->bSYNC())
    {
        m_txsta->value.put(m_txsta->value.get() & ~ _TXSTA::TRMT);
    }

}

void _TXREG::put_value(unsigned int new_value)
{
    put(new_value);

    update();
}

void _TXREG::callback()
{
    Dprintf(("TXREG callback - time:%" PRINTF_GINT64_MODIFIER "x full %d\n",get_cycles().get(), full));
    if (full)
    {
        mUSART->full();
        full = false;
    }
    else
    {
        mUSART->emptyTX();
    }
}

void _TXREG::callback_print()
{
    std::cout << "TXREG " << name() << " CallBack ID " << CallBackID << '\n';
}

//-----------------------------------------------------------
// TXSTA - setIOpin - assign the I/O pin associated with the
// the transmitter.


void _TXSTA::setIOpin(PinModule *newPinModule)
{
    if (!m_source)
    {
        m_source = new TXSignalSource(this);
        m_control = new TXSignalControl(this);
    }
    else if (m_PinModule)	// disconnect old pin
    {
        disableTXPin();
    }

    m_PinModule = newPinModule;
    if (bTXEN() && rcsta->bSPEN())
    {
        enableTXPin();
    }
}

void _TXSTA::disableTXPin()
{

    if (m_PinModule)
    {

        if (input_as_output)
        {
            IO_bi_directional * m_io = (IO_bi_directional *)&m_PinModule->getPin();

            input_as_output = false;
            m_io->set_VthIn(save_VthIn);
            m_io->set_ZthIn(save_ZthIn);
        }
        if (pullup_input_as_output)
        {
            IO_bi_directional_pu * m_io = (IO_bi_directional_pu *)&m_PinModule->getPin();

            pullup_input_as_output = false;
            m_io->set_Vpullup(save_Vpullup);
            m_io->set_Zpullup(save_Zpullup);
        }
        m_PinModule->setSource(nullptr);
        m_PinModule->setControl(nullptr);
        SourceActive = false;
        m_PinModule->getPin().newGUIname(m_PinModule->getPin().name().c_str());
        if (m_clkSink)
        {
            m_PinModule->removeSink(m_clkSink);
            m_clkSink->release();
            m_clkSink = 0;
        }
        m_PinModule->updatePinModule();
    }
}

void _TXSTA::setCKpin(PinModule *ck_pin)
{
    if (!SourceActive)
        m_PinModule = ck_pin;
}

void _TXSTA::enableTXPin()
{
    assert(m_PinModule);

    if (m_PinModule && !SourceActive)
    {
        char out;
        char reg_no = *(name().c_str() + 2);    // Cope with 16f170x naming
        if ( !isdigit(reg_no) )                 // or more common naming
            reg_no = *(name().c_str() + 5);
        if (bSYNC())
        {
            char ck[4] = "CK";
            if (reg_no)
            {
                ck[2] = reg_no;
                ck[3] = 0;
            }
            m_PinModule->getPin().newGUIname(ck);
            out = '0';
            if (!bCSRC())	  // slave clock
            {
                if (!m_clkSink)
                {
                    m_clkSink = new CLKSignalSink(rcsta);
                    m_PinModule->addSink(m_clkSink);
                    rcsta->set_old_clock_state(m_PinModule->getPin().getState());
                }
                mUSART->emptyTX();
                return;
            }
        }
        else
        {
            char tx[4] = "TX";
            if (reg_no)
            {
                tx[2] = reg_no;
                tx[3] = 0;
            }
            m_PinModule->getPin().newGUIname(tx);
            out = '1';
        }

        // For EUSART set TX as output
        if (mUSART->IsEUSART())
        {
            m_PinModule->setControl(m_control);
            m_PinModule->setSource(m_source);
            SourceActive = true;
        }
        else
        {
            m_PinModule->setSource(m_source);
            SourceActive = true;
            if (use_input_pin_only && (m_PinModule->getControlState() == '0'))  // pin is output
            {
                printf("*** FAIL USART TX pin not input\n");
            }
        }
        putTXState(out);
    }

    mUSART->emptyTX();
}

void _TXSTA::releasePin()
{
    if (m_PinModule && SourceActive)
    {
        m_PinModule->getPin().newGUIname(m_PinModule->getPin().name().c_str());
        m_PinModule->setControl(0);
        SourceActive = false;
    }
}

//-----------------------------------------------------------
// TXSTA - putTXState - update the state of the TX output pin
//

void _TXSTA::putTXState(char newTXState)
{
    m_cTxState = bInvertPin ? newTXState ^ 1 : newTXState;

    // if not EUSART, TX pin is input, bit bang input
    if (!mUSART->IsEUSART())
    {
        if (m_PinModule->getControlState() == '1')	// pin is input
        {
            IO_bi_directional_pu * m_io_pu = (IO_bi_directional_pu *)(&m_PinModule->getPin());
            if (m_io_pu->getPullupStatus())
            {
                if (!pullup_input_as_output)
                {
                    pullup_input_as_output = true;
                    save_Vpullup = m_io_pu->get_Vpullup();
                    save_Zpullup = m_io_pu->get_Zpullup();
                    m_io_pu->set_Zpullup(150.);
                }
                if (m_cTxState == '0')
                    m_io_pu->set_Vpullup(0.5);
                else
                    m_io_pu->set_Vpullup(get_cpu()->get_Vdd());
            }
            else
            {
                IO_bi_directional * m_io = (IO_bi_directional *)&m_PinModule->getPin();

                if (!input_as_output)
                {
                    input_as_output = true;
                    save_VthIn = m_io->get_VthIn();
                    save_ZthIn = m_io->get_ZthIn();
                    m_io->set_ZthIn(150.);
                }
                if (m_cTxState == '0')
                {
                    m_io->set_VthIn(0.5);
                }
                else
                {
                    m_io->set_VthIn(get_cpu()->get_Vdd());
                }

            }
        }
        else    //Pin is output
        {
            if (use_input_pin_only) return;
        }
    }
    if (m_PinModule)
        m_PinModule->updatePinModule();
}

//-----------------------------------------------------------
// TXSTA - Transmit Register Status and Control

void _TXSTA::put_value(unsigned int new_value)
{
    put(new_value);

    update();
}

void _TXSTA::put(unsigned int new_value)
{
    unsigned int old_value = value.get();

    trace.raw(write_trace.get() | value.get());

    if (! mUSART->IsEUSART() )
        new_value &= ~SENDB;      // send break only supported on EUSART

    // The TRMT bit is controlled entirely by hardware.
    // It is high if the TSR has any data.

    //RRRvalue.put((new_value & ~TRMT) | ( (bit_count) ? 0 : TRMT));
    value.put((new_value & ~TRMT) | (old_value & TRMT));

    Dprintf(("%s TXSTA value=0x%x\n",name().c_str(), value.get()));

    if ((old_value ^ value.get()) & TXEN)
    {

        // The TXEN bit has changed states.
        //
        // If transmitter is being enabled and the transmit register
        // has some data that needs to be sent, then start a
        // transmission.
        // If the transmitter is being disabled, then abort any
        // transmission.

        if (value.get() & TXEN)
        {
            Dprintf(("TXSTA - enabling transmitter\n"));
            if (rcsta->bSPEN())
            {

                if (bSYNC() && ! bTRMT() && !rcsta->bSREN() && !rcsta->bCREN())
                {
                    // need to check bTRMT before calling enableTXPin
                    enableTXPin();
                    rcsta->sync_start_transmitting();
                }
                else
                    enableTXPin();
            }
        }
        else
        {
            stop_transmitting();
            mUSART->full();         // Turn off TXIF
            disableTXPin();
        }
    }
}

//------------------------------------------------------------
//
char _TXSTA::getState()
{
    return m_cTxState;
}


// _TXSTA::stop_transmitting()
//
void _TXSTA::stop_transmitting()
{
    Dprintf(("stopping a USART transmission\n"));

    bit_count = 0;
    value.put(value.get() | TRMT);

    // It's not clear from the documentation as to what happens
    // to the TXIF when we are aborting a transmission. According
    // to the documentation, the TXIF is set when the TXEN bit
    // is set. In other words, when the Transmitter is enabled
    // the txreg is emptied (and hence TXIF set). But what happens
    // when TXEN is cleared? Should we clear TXIF too?
    //
    // There is one sentence that says when the transmitter is
    // disabled that the whole transmitter is reset. So I interpret
    // this to mean that the TXIF gets cleared. I could be wrong
    // (and I don't have a way to test it on a real device).
    //
    // Another interpretation is that TXIF retains it state
    // through changing TXEN. However, when SPEN (serial port) is
    // set then the whole usart is reinitialized and TXIF will
    // get set.
    //
    //  txreg->full();   // Clear TXIF
}

void _TXSTA::start_transmitting()
{
    Dprintf(("starting a USART transmission\n"));

    // Build the serial byte that's about to be transmitted.
    // I doubt the pic really does this way, but gpsim builds
    // the entire bit stream including the start bit, 8 data
    // bits, optional 9th data bit and the stop, and places
    // this into the tsr register. But since the contents of
    // the tsr are inaccessible, I guess we'll never know.
    //
    // (BTW, if you look carefully you may puzzle over why
    //  there appear to be 2 stop bits in the packet built
    //  below. Well, it's because the way gpsim implements
    //  the transmit logic. The second stop bit doesn't
    //  actually get transmitted - it merely causes the first
    //  stop bit to get transmitted before the TRMT bit is set.
    //
    //  RRR I believe the above paragraph is a mis-understanding
    //  The tsr register becomes empty, and the TRMT flag goes high,
    //  when we start to transmit the stop bit. Note that transmision
    //  is synchronous with the baud clock, so the start of transmision
    //  of a new character waits for the next callback. This delay maybe,
    //  in fact, the stop bit of the previous transmision,
    //
    //  [Recall that the TRMT bit indicates when the tsr
    //  {transmit shift register} is empty. It's not tied to
    //  an interrupt pin, so the pic application software
    //  most poll this bit.
    //
    //  RRR Also The following is wrong:
    //  This bit is set after the STOP
    //  bit is transmitted.] This is a cheap trick that saves
    //  one comparison in the callback code.)

    // The start bit, which is always low, occupies bit position
    // zero. The next 8 bits come from the txreg.
    assert(txreg);
    if (!txreg)
        return;
    if (value.get() & SENDB)
    {
        transmit_break();
        return;
    }
    tsr = txreg->value.get() << 1;

    // Is this a 9-bit data transmission?
    if (value.get() & TX9)
    {
        // Copy the stop bit and the 9th data bit to the tsr.
        // (See note above for the reason why two stop bits
        // are appended to the packet.)

        tsr |= ( (value.get() & TX9D) ? (3 << 9) : (2 << 9));
        bit_count = 11;  // 1 start, 9 data, 1 stop
    }
    else
    {
        // The stop bit is always high. (See note above
        // for the reason why two stop bits are appended to
        // the packet.)
        tsr |= (1 << 9);
        bit_count = 10;  // 1 start, 8 data, 1 stop
    }


    // Set a callback breakpoint at the next SPBRG edge
    if(cpu)
        get_cycles().set_break(spbrg->get_cpu_cycle(1), this);


    // The TSR now has data, so clear the Transmit Shift
    // Register Status bit.

    trace.raw(write_trace.get() | value.get());
    value.put(value.get() & ~TRMT);

}

void _TXSTA::transmit_break()
{
    Dprintf(("starting a USART sync-break transmission\n"));

    // A sync-break is 13 consecutive low bits and one stop bit. Use the
    // standard transmit logic to achieve this

    if (!txreg)
        return;

    tsr = 1 << 13;

    bit_count = 14;  // 13 break, 1 stop

    // The TSR now has data, so clear the Transmit Shift
    // Register Status bit.

    trace.raw(write_trace.get() | value.get());
    value.put(value.get() & ~TRMT);

    callback();
}

void _TXSTA::transmit_a_bit()
{
    if (bit_count)
    {
        Dprintf(("Transmit bit #%x: bit val:%u time:0x%" PRINTF_GINT64_MODIFIER "x\n",
                 bit_count, (tsr & 1), get_cycles().get()));

        putTXState((tsr & 1) ? '1' : '0');

        tsr >>= 1;

        --bit_count;
    }
}



void _TXSTA::callback()
{
    Dprintf(("TXSTA callback - time:%" PRINTF_GINT64_MODIFIER "x\n", get_cycles().get()));

    transmit_a_bit();

    if (!bit_count)
    {

        value.put(value.get() & ~SENDB);

        // tsr is empty.
        // If there is any more data in the TXREG, then move it to
        // the tsr and continue transmitting other wise set the TRMT bit

        // (See the note above about the 'extra' stop bit that was stuffed
        // into the tsr register.

        if (mUSART->bIsTXempty())
            value.put(value.get() | TRMT);
        else
        {
            start_transmitting();
            mUSART->emptyTX();
        }

    }
    else
    {
        // bit_count is non zero which means there is still
        // data in the tsr that needs to be sent.

        if (cpu)
        {
            get_cycles().set_break(spbrg->get_cpu_cycle(1), this);
        }
    }

}

void _TXSTA::callback_print()
{
    std::cout << "TXSTA " << name() << " CallBack ID " << CallBackID << '\n';
}

//-----------------------------------------------------------
// Receiver portion of the USART
//-----------------------------------------------------------
//
// First RCSTA -- Receiver Control and Status
// The RCSTA class controls the usart reception. The PIC usarts have
// two modes: synchronous and asynchronous.
// Asynchronous reception:
//   Asynchronous reception means that there is no external clock
// available for telling the usart when to sample the data. Sampling
// timing is all based upon the PIC's oscillator. The SPBRG divides
// this clock down to a frequency that's appropriate to the data
// being received. (e.g. 9600 baud defines the rate at which data
// will be sent to the pic - 9600 bits per second.) The start bit,
// which is a high to low transition on the receive line, defines
// when the usart should start sampling the incoming data.
//   The pic usarts sample asynchronous data three times in "approximately
// the middle" of each bit. The data sheet is not exactly specific
// on what's the middle. Consequently, gpsim takes a stab/ educated
// guess on when these three samples are to be taken. Once the
// three samples are taken, then simple majority summing determines
// the sample e.g. if two out of three of the samples are high, then
// then the data bit is considered high.
//
//-----------------------------------------------------------
// RCSTA::put
//
void _RCSTA::put(unsigned int new_value)
{
    unsigned int diff;
    unsigned int readonly = value.get() & (RX9D | OERR | FERR);

    diff = new_value ^ value.get();
    trace.raw(write_trace.get() | value.get());
    assert(txsta);
    assert(txsta->txreg);
    assert(rcreg);
    // If SPEN being turned off, clear all readonly bits
    if (diff & SPEN && !(new_value & SPEN))
    {
        readonly = 0;
        // clear receive stack (and rxif)
        rcreg->pop();
        rcreg->pop();
    }
    // if CREN is being cleared, make sure OERR is clear
    else if (diff & CREN && !(new_value & CREN))
        readonly &= (RX9D | FERR);
    value.put( readonly   |  (new_value & ~(RX9D | OERR | FERR)));

    if (!txsta->bSYNC()) // Asynchronous case
    {
        if (diff & (SPEN | CREN)) // RX status change
        {
            if ((value.get() & (SPEN | CREN)) == SPEN )
            {
                if (txsta->bTXEN()) txsta->enableTXPin();
                spbrg->start();
            }
            else if ((value.get() & (SPEN | CREN)) == (SPEN | CREN))
            {
                enableRCPin();
                if (txsta->bTXEN()) txsta->enableTXPin();
                spbrg->start();
                start_receiving();
                // If the rx line is low, then go ahead and start receiving now.
                if (m_cRxState == '0' || m_cRxState == 'w')
                    receive_start_bit();
                // Clear overrun error when turning on RX
                value.put( value.get() & (~OERR) );
            }
            else 		// RX off, check TX
            {
                if (m_PinModule)
                    m_PinModule->getPin().newGUIname(
                        m_PinModule->getPin().name().c_str());
                stop_receiving();
                state = RCSTA_DISABLED;

                if (bSPEN())	// RX off but TX may still be active
                {
                    if (txsta->bTXEN()) 		//TX output active
                        txsta->enableTXPin();
                    else				// TX off
                        txsta->disableTXPin();
                }
                return;
            }
        }
    }
    else    				// synchronous case
    {

        if (diff & RX9)
        {
            if (bRX9())
                bit_count = 9;
            else
                bit_count = 8;
        }
        if (diff & (SPEN | CREN | SREN )) // RX status change
        {
            // Synchronous transmit (SREN & CREN == 0)
            if ((value.get() & (SPEN | SREN | CREN)) == SPEN)
            {
                enableRCPin(DIR_OUT);
                if (txsta->bTXEN()) txsta->enableTXPin();
                return;
            }
            // Synchronous receive (SREN | CREN != 0)
            else if (value.get() & (SPEN))
            {
                enableRCPin(DIR_IN);
                txsta->enableTXPin();
                rsr = 0;
                if (bRX9())
                    bit_count = 9;
                else
                    bit_count = 8;
                if (txsta->bCSRC()) // Master mode
                {
                    sync_next_clock_edge_high = true;
                    txsta->putTXState('0');  // clock low
                    callback();
                }

                return;
            }
            else	// turn off UART
            {
                if (m_PinModule)
                {
                    m_PinModule->getPin().newGUIname(
                        m_PinModule->getPin().name().c_str());
                    if (m_sink)
                    {
                        m_PinModule->removeSink(m_sink);
                        m_sink->release();
                        m_sink = 0;
                    }
                }
                txsta->disableTXPin();
            }
        }
    }

}

void _RCSTA::enableRCPin(char direction)
{
    if (m_PinModule)
    {
        char reg_no = *(name().c_str() + 5);
        if (txsta->bSYNC()) // Synchronous case
        {
            if (!m_source)
            {
                m_source = new RCSignalSource(this);
                m_control = new RCSignalControl(this);
            }
            if (direction == DIR_OUT)
            {
                m_DTdirection = '0';
                if (SourceActive == false)
                {
                    m_PinModule->setSource(m_source);
                    m_PinModule->setControl(m_control);
                    SourceActive = true;
                }
                putRCState('0');
            }
            else
            {
                m_DTdirection = '1';
                if (SourceActive == true)
                {
                    m_PinModule->setSource(0);
                    m_PinModule->setControl(0);
                    m_PinModule->updatePinModule();
                }
            }
            char dt[4] = "DT";
            dt[2] = reg_no;
            dt[3] = 0;
            m_PinModule->getPin().newGUIname(dt);

        }
        else		// Asynchronous case
        {
            char rx[4] = "RX";
            rx[2] = reg_no;
            rx[3] = 0;
            m_PinModule->getPin().newGUIname(rx);
        }

    }

}

void _RCSTA::disableRCPin()
{
}

void _RCSTA::releasePin()
{
    if (m_PinModule && SourceActive)
    {
        m_PinModule->getPin().newGUIname(m_PinModule->getPin().name().c_str());
        m_PinModule->setControl(0);
        SourceActive = false;
    }
}

void _RCSTA::put_value(unsigned int new_value)
{
    put(new_value);

    update();
}

//-----------------------------------------------------------
// RCSTA - putRCState - update the state of the DTx output pin
//                      only used for Synchronous mode
//

void _RCSTA::putRCState(char newRCState)
{
    bInvertPin = mUSART->baudcon.rxdtp();
    m_cTxState = bInvertPin ? newRCState ^ 1 : newRCState;

    if (m_PinModule)
        m_PinModule->updatePinModule();
}

//-----------------------------------------------------------
// RCSTA - setIOpin - assign the I/O pin associated with the
// the receiver.

void _RCSTA::setIOpin(PinModule *newPinModule)
{
    if (m_sink)
    {
        if (m_PinModule)
        {
            m_PinModule->removeSink(m_sink);
            if (value.get() & SPEN)
                m_PinModule->getPin().newGUIname(m_PinModule->getPin().name().c_str());
        }
    }
    else
        m_sink = new RXSignalSink(this);

    m_PinModule = newPinModule;
    if (m_PinModule)
    {
        m_PinModule->addSink(m_sink);
        old_clock_state = m_PinModule->getPin().getState();
        if (value.get() & SPEN)
            m_PinModule->getPin().newGUIname("RX/DT");
    }

}

//-----------------------------------------------------------
// RCSTA - setState
// This gets called whenever there's a change detected on the RX pin.
// The usart is only interested in those changes when it is waiting
// for the start bit. Otherwise, the rcsta callback function will sample
// the rx pin (if we're receiving).


void _RCSTA::setState(char new_RxState)
{
    Dprintf((" %s setState:%c\n",name().c_str(), new_RxState));

    m_cRxState = new_RxState;

    if ((state == RCSTA_WAITING_FOR_START) && (m_cRxState == '0' || m_cRxState == 'w'))
        receive_start_bit();
}

//  Transmit in synchronous mode
//
void _RCSTA::sync_start_transmitting()
{
    assert(txreg);

    rsr = txreg->value.get();
    if (txsta->bTX9())
    {
        rsr |= (txsta->bTX9D() << 8);
        bit_count = 9;
    }
    else
        bit_count = 8;
    txsta->value.put(txsta->value.get() & ~ _TXSTA::TRMT);
    if (txsta->bCSRC())
    {
        sync_next_clock_edge_high = true;
        txsta->putTXState('0');		// clock low
        callback();
    }
}

void _RCSTA::set_old_clock_state(char new3State)
{
    bool state = (new3State == '1' || new3State == 'W');
    state = mUSART->baudcon.txckp() ? !state : state;
    old_clock_state = state;
}

void _RCSTA::clock_edge(char new3State)
{
    bool state = (new3State == '1' || new3State == 'W');

    // invert clock, if requested
    state = mUSART->baudcon.txckp() ? !state : state;
    if (old_clock_state == state) return;
    old_clock_state = state;
    if (value.get() & SPEN)
    {
        // Transmitting ?
        if ((value.get() & ( CREN | SREN)) == 0)
        {
            if (state)	// clock high, output data
            {
                if (bit_count)
                {
                    putRCState((rsr & 1) ? '1' : '0');
                    rsr >>= 1;
                    bit_count--;
                }
            }
            else
            {
                if(mUSART->bIsTXempty())
                {
                    txsta->value.put(txsta->value.get() | _TXSTA::TRMT);
                }
                else
                {
                    sync_start_transmitting();
                    mUSART->emptyTX();
                }
            }
        }
        else		// receiving
        {
            if (!state) // read data as clock goes low
            {
                bool data = m_PinModule->getPin().getState();
                data = mUSART->baudcon.rxdtp() ? !data : data;

                if (bRX9())
                    rsr |= data << 9;
                else
                    rsr |= data << 8;

                rsr >>= 1;
                if (--bit_count == 0)
                {
                    rcreg->push(rsr);
                    if (bRX9())
                        bit_count = 9;
                    else
                        bit_count = 8;
                    rsr = 0;
                }
            }
        }
    }

}

//-----------------------------------------------------------
// RCSTA::receive_a_bit(unsigned int bit)
//
// A new bit needs to be copied to the the Receive Shift Register.
// If the receiver is receiving data, then this routine will copy
// the incoming bit to the rsr. If this is the last bit, then a
// check will be made to see if we need to set up for the next
// serial byte.
// If this is not the last bit, then the receive state machine.

void _RCSTA::receive_a_bit(unsigned int bit)
{
    // If we're waiting for the start bit and this isn't it then
    // we don't need to look any further
    Dprintf(("%s receive_a_bit state:%u bit:%u time:0x%" PRINTF_GINT64_MODIFIER "x\n",
             name().c_str(), state, bit, get_cycles().get()));

    if( state == RCSTA_MAYBE_START)
    {
        if (bit)
            state = RCSTA_WAITING_FOR_START;
        else
            state = RCSTA_RECEIVING;
        return;
    }
    if (bit_count == 0)
    {
        // we should now have the stop bit
        if (bit)
        {
            // got the stop bit
            // If the rxreg has data from a previous reception then
            // we have a receiver overrun error.
            // cout << "rcsta.rsr is full\n";

            if((value.get() & RX9) == 0)
                rsr >>= 1;

            // Clear any framing error
            value.put(value.get() & (~FERR) );

            // copy the rsr to the fifo
            if(rcreg)
                rcreg->push( rsr & 0x1ff);

            Dprintf(("%s _RCSTA::receive_a_bit received 0x%02X\n",name().c_str(), rsr & 0x1ff));

        }
        else
        {
            //no stop bit; framing error
            value.put(value.get() | FERR);

            // copy the rsr to the fifo
            if(rcreg)
                rcreg->push( rsr & 0x1ff);
        }
        // If we're continuously receiving, then set up for the next byte.
        // FIXME -- may want to set a half bit delay before re-starting...
        if(value.get() & CREN)
            start_receiving();
        else
            state = RCSTA_DISABLED;
        return;
    }


    // Copy the bit into the Receive Shift Register
    if (bit)
        rsr |= 1 << 9;

    //cout << "Receive bit #" << bit_count << ": " << (rsr&(1<<9)) << '\n';

    rsr >>= 1;
    bit_count--;

}

void _RCSTA::stop_receiving()
{
    rsr = 0;
    bit_count = 0;
    state = RCSTA_DISABLED;
}

void _RCSTA::start_receiving()
{
    Dprintf(("%s The USART is starting to receive data\n", name().c_str()));

    rsr = 0;
    sample = 0;

    // Is this a 9-bit data reception?
    bit_count = (value.get() & RX9) ? 9 : 8;

    state = RCSTA_WAITING_FOR_START;
}

void _RCSTA::overrun()
{
    value.put(value.get() | _RCSTA::OERR);
}

void _RCSTA::set_callback_break(unsigned int spbrg_edge)
{
    if (cpu && spbrg)
    {
        unsigned int time_to_event
            = ( spbrg->get_cycles_per_tick() * spbrg_edge ) / TOTAL_SAMPLE_STATES;
        get_cycles().set_break(get_cycles().get() + time_to_event, this);
    }
}

void _RCSTA::receive_start_bit()
{
    Dprintf(("%s USART received a start bit\n", name().c_str()));

    if ((value.get() & (CREN | SREN)) == 0)
    {
        Dprintf(("  but not enabled\n"));
        return;
    }

    if (txsta && (txsta->value.get() & _TXSTA::BRGH))
        set_callback_break(BRGH_FIRST_MID_SAMPLE);
    else
        set_callback_break(BRGL_FIRST_MID_SAMPLE);

    sample = 0;
    sample_state = RCSTA_WAITING_MID1;
    state = RCSTA_MAYBE_START;
}

//------------------------------------------------------------
void _RCSTA::callback()
{
    Dprintf(("RCSTA callback. %s time:0x%" PRINTF_GINT64_MODIFIER "x\n", name().c_str(), get_cycles().get()));

    if (txsta->bSYNC())	// Synchronous mode RX/DT is data, TX/CK is clock
    {
        if (sync_next_clock_edge_high)	// + edge of clock
        {
            sync_next_clock_edge_high = false;
            txsta->putTXState('1');	// Clock high
            // Transmit
            if ((value.get() & (SPEN | SREN | CREN)) == SPEN)
            {
                if (bit_count)
                {
                    putRCState((rsr & 1) ? '1' : '0');
                    rsr >>= 1;
                    bit_count--;
                }
            }
        }
        else	// - clock edge
        {
            sync_next_clock_edge_high = true;
            txsta->putTXState('0');	//clock low
            // Receive Master mode
            if ((value.get() & (SPEN | SREN | CREN)) != SPEN)
            {

                if (value.get() & OERR)
                    return;
                bool data = m_PinModule->getPin().getState();
                data = mUSART->baudcon.rxdtp() ? !data : data;
                if (bRX9())
                    rsr |= data << 9;
                else
                    rsr |= data << 8;
                rsr >>= 1;
                if (--bit_count == 0)
                {
                    rcreg->push(rsr);
                    if (bRX9())
                        bit_count = 9;
                    else
                        bit_count = 8;
                    rsr = 0;
                    value.put(value.get() & ~SREN);
                    if ((value.get() & (SPEN | SREN | CREN)) == SPEN )
                    {
                        enableRCPin(DIR_OUT);
                        return;
                    }
                }
            }
            else		// Transmit, clock low
            {
                if (bit_count == 0 && !mUSART->bIsTXempty())
                {
                    sync_start_transmitting();
                    mUSART->emptyTX();
                    return;
                }
                else if(bit_count == 0 && mUSART->bIsTXempty())
                {
                    txsta->value.put(txsta->value.get() | _TXSTA::TRMT);
                    putRCState('0');
                    return;
                }
            }
        }
        if (cpu && (value.get() & SPEN))
        {
            future_cycle = get_cycles().get() + spbrg->get_cycles_per_tick();
            get_cycles().set_break(future_cycle, this);
        }
    }
    else
    {
        // A bit is sampled 3 times.
        switch (sample_state)
        {
        case RCSTA_WAITING_MID1:
            if (m_cRxState == '1' || m_cRxState == 'W')
                sample++;

            if(txsta && (txsta->value.get() & _TXSTA::BRGH))
                set_callback_break(BRGH_SECOND_MID_SAMPLE - BRGH_FIRST_MID_SAMPLE);
            else
                set_callback_break(BRGL_SECOND_MID_SAMPLE - BRGL_FIRST_MID_SAMPLE);

            sample_state = RCSTA_WAITING_MID2;

            break;

        case RCSTA_WAITING_MID2:
            if (m_cRxState == '1' || m_cRxState == 'W')
                sample++;

            if(txsta && (txsta->value.get() & _TXSTA::BRGH))
                set_callback_break(BRGH_THIRD_MID_SAMPLE - BRGH_SECOND_MID_SAMPLE);
            else
                set_callback_break(BRGL_THIRD_MID_SAMPLE - BRGL_SECOND_MID_SAMPLE);

            sample_state = RCSTA_WAITING_MID3;

            break;

        case RCSTA_WAITING_MID3:
            if (m_cRxState == '1' || m_cRxState == 'W')
                sample++;

            receive_a_bit( (sample >= 2));
            sample = 0;

            // If this wasn't the last bit then go ahead and set a break for the next bit.
            if(state==RCSTA_RECEIVING)
            {
                if(txsta && (txsta->value.get() & _TXSTA::BRGH))
                    set_callback_break(TOTAL_SAMPLE_STATES - (BRGH_THIRD_MID_SAMPLE - BRGH_FIRST_MID_SAMPLE));
                else
                    set_callback_break(TOTAL_SAMPLE_STATES - (BRGL_THIRD_MID_SAMPLE - BRGL_FIRST_MID_SAMPLE));

                sample_state = RCSTA_WAITING_MID1;
            }

            break;

        default:
            //cout << "Error RCSTA callback with bad state\n";
            // The receiver was probably disabled in the middle of a reception.
            ;
        }
    }

}

//-----------------------------------------------------------
void _RCSTA::callback_print()
{
    std::cout << "RCSTA " << name() << " CallBack ID " << CallBackID << '\n';
}

//-----------------------------------------------------------
// RCREG
//
void _RCREG::push(unsigned int new_value)
{
    trace.raw(write_trace.get() | value.get());

    if (fifo_sp >= 2)
    {

        if (m_rcsta)
            m_rcsta->overrun();

        Dprintf(("%s receive overrun\n", name().c_str()));

    }
    else
    {

        Dprintf(("%s pushing uart reception onto rcreg stack, value received=0x%x\n",name().c_str(), new_value));
        fifo_sp++;
        oldest_value = value.get();
        value.put(new_value & 0xff);
        if (m_rcsta)
        {
            unsigned int rcsta = m_rcsta->value.get();
            if (new_value & 0x100)
                rcsta |= _RCSTA::RX9D;
            else
                rcsta &= ~ _RCSTA::RX9D;
            m_rcsta->value.put(rcsta);
        }
    }

    mUSART->set_rcif();
}

void _RCREG::pop()
{
    if (fifo_sp == 0)
        return;

    if (--fifo_sp == 1)
    {
        value.put(oldest_value & 0xff);
        if (m_rcsta)
        {
            unsigned int rcsta = m_rcsta->value.get();
            if (oldest_value & 0x100)
                rcsta |= _RCSTA::RX9D;
            else
                rcsta &= ~ _RCSTA::RX9D;
            m_rcsta->value.put(rcsta);
        }
    }

    if (fifo_sp == 0)
        mUSART->clear_rcif();
}

unsigned int _RCREG::get_value()
{
    return value.get();
}

unsigned int _RCREG::get()
{
    pop();
    trace.raw(read_trace.get() | value.get());
    return value.get();
}




//-----------------------------------------------------------
// SPBRG - Serial Port Baud Rate Generator
//
// The SPBRG is essentially a continuously running programmable
// clock. (Note that this will slow the simulation down if the
// serial port is not used. Perhaps gpsim needs some kind of
// pragma type thing to disable cpu intensive peripherals...)

void _SPBRG::get_next_cycle_break()
{
    future_cycle = last_cycle + get_cycles_per_tick();

    if (cpu)
    {
        if (future_cycle <= get_cycles().get())
        {
            Dprintf(("%s future %" PRINTF_GINT64_MODIFIER "d <= now %" PRINTF_GINT64_MODIFIER "d\n", name().c_str(), future_cycle, get_cycles().get()));
            last_cycle = get_cycles().get();
            future_cycle = last_cycle + get_cycles_per_tick();
        }
        get_cycles().set_break(future_cycle, this);
    }

    //Dprintf(("SPBRG::callback next break at 0x%" PRINTF_GINT64_MODIFIER "x\n",future_cycle));
}

unsigned int _SPBRG::get_cycles_per_tick()
{
    unsigned int cpi = (cpu) ? p_cpu->get_ClockCycles_per_Instruction() : 4;
    unsigned int brgval, cpt, ret;

    if ( baudcon && baudcon->brg16() )
    {
        brgval =  ( brgh ? brgh->value.get() * 256 : 0 ) + value.get();
        cpt = 4;    // hi-speed divisor in 16-bit mode is 4
    }
    else
    {
        brgval = value.get();
        cpt = 16;   // hi-speed divisor in 8-bit mode is 16
    }

    if ( txsta && (txsta->value.get() & _TXSTA::SYNC) )
    {
        // Synchronous mode - divisor is always 4
        // However, code wants two transitions per bit
        // to generate clock for master mode, so use 2
        cpt = 2;
    }
    else
    {
        // Asynchronous mode
        if(txsta && !(txsta->value.get() & _TXSTA::BRGH))
        {
            cpt *= 4;   // lo-speed divisor is 4 times hi-speed
        }
    }

    ret = ((brgval + 1) * cpt) / cpi;
    ret = ret ? ret : 1;
    return ret;
}

void _SPBRG::start()
{
    if (running)
        return;

    if (! skip  || get_cycles().get() >= skip)
    {
        if (cpu)
            last_cycle = get_cycles().get();
        skip = 0;
    }
    running = true;

    start_cycle = last_cycle;

    get_next_cycle_break();

    Dprintf((" SPBRG::start   last_cycle:0x%" PRINTF_GINT64_MODIFIER "x: future_cycle:0x%" PRINTF_GINT64_MODIFIER "x\n",last_cycle,future_cycle));
}

void _SPBRG::put(unsigned int new_value)
{
    trace.raw(write_trace.get() | value.get());
    value.put(new_value);

    Dprintf((" SPBRG value=0x%x\n",value.get()));
    //Prevent updating last_cycle until all current breakpoints have expired
    //Otherwise we see that rx/tx periods get screwed up from now until future_cycle
    future_cycle = last_cycle + get_cycles_per_tick();
    skip = future_cycle;
    Dprintf((" SPBRG value=0x%x skip=0x%" PRINTF_GINT64_MODIFIER "x last=0x%" PRINTF_GINT64_MODIFIER "x cycles/tick=0x%x\n",value.get(), skip, last_cycle, get_cycles_per_tick()));
}

void _SPBRG::put_value(unsigned int new_value)
{
    put(new_value);

    update();
}

void _SPBRGH::put(unsigned int new_value)
{
    trace.raw(write_trace.get() | value.get());
    value.put(new_value);

    if (m_spbrg)
        m_spbrg->set_start_cycle();
}

void _SPBRG::set_start_cycle()
{
    //Prevent updating last_cycle until all current breakpoints have expired
    //Otherwise we see that rx/tx persiods get screwed up from now until future_cycle
    future_cycle = last_cycle + get_cycles_per_tick();
    skip = future_cycle;
}

void _SPBRGH::put_value(unsigned int new_value)
{
    put(new_value);

    update();
}

//--------------------------
//guint64 _SPBRG::get_last_cycle()
//
// Get the cpu cycle corresponding to the last edge of the SPBRG
//

guint64 _SPBRG::get_last_cycle()
{
    // There's a chance that a SPBRG break point exists on the current
    // cpu cycle, but has not yet been serviced.
    if (cpu)
        return (get_cycles().get() == future_cycle) ? future_cycle : last_cycle;
    else
        return 0;
}

//--------------------------
//guint64 _SPBRG::get_cpu_cycle(unsigned int edges_from_now)
//
//  When the SPBRG is enabled, it becomes a free running counter
// that's synchronous with the cpu clock. The frequency of the
// counter depends on the mode of the usart:
//
//  Synchronous mode:
//    baud = cpu frequency / 4 / (spbrg.value + 1)
//
//  Asynchronous mode:
//   high frequency:
//     baud = cpu frequency / 16 / (spbrg.value + 1)
//   low frequency:
//     baud = cpu frequency / 64 / (spbrg.value + 1)
//
// What this routine will do is return the cpu cycle corresponding
// to a (rising) edge of the spbrg clock.

guint64 _SPBRG::get_cpu_cycle(unsigned int edges_from_now)
{
    // There's a chance that a SPBRG break point exists on the current
    // cpu cycle, but has not yet been serviced.
    guint64 cycle = (get_cycles().get() == future_cycle) ? future_cycle : last_cycle;

    return edges_from_now * get_cycles_per_tick() + cycle;
}

void _SPBRG::callback()
{
    if (skip)
    {
        Dprintf((" SPBRG skip=0x%" PRINTF_GINT64_MODIFIER "x, cycle=0x%" PRINTF_GINT64_MODIFIER "x\n", skip, get_cycles().get()));
    }
    if (! skip  || get_cycles().get() >= skip)
    {
        last_cycle = get_cycles().get();
        skip = 0;
    }

    //Dprintf(("SPBRG rollover at cycle:0x%" PRINTF_GINT64_MODIFIER "x\n",last_cycle));

    if ((rcsta && rcsta->bSPEN()) || (txsta && txsta->bTXEN()))
    {
        // If the serial port is enabled, then set another
        // break point for the next clock edge.
        get_next_cycle_break();

    }
    else
    {
        running = false;
    }
}

void _SPBRG::callback_print()
{
    std::cout << "_SPBRG " << name() << " CallBack ID " << CallBackID << '\n';
}

//-----------------------------------------------------------
// TXSTA - Transmit Register Status and Control

void _BAUDCON::put_value(unsigned int new_value)
{
    put(new_value);

    update();
}

void _BAUDCON::put(unsigned int new_value)
{
    unsigned int old_value = value.get();

    trace.raw(write_trace.get() | value.get());

    // The RCIDL bit is controlled entirely by hardware.
    new_value &= ~RCIDL;
    if ( rcsta->rc_is_idle() ) new_value |= RCIDL;

    value.put(new_value);

    Dprintf(("%s BAUDCON value=0x%x\n",name().c_str(), value.get()));


    if ( (old_value ^ value.get()) & TXCKP)
    {

        // The TXCKP bit has changed states.
        //
        txsta->set_pin_pol ((value.get() & TXCKP) ? true : false);
    }
}

//--------------------------------------------------
// member functions for the USART
//--------------------------------------------------
void USART_MODULE::initialize(PIR *_pir,
                              PinModule *tx_pin, PinModule *rx_pin,
                              _TXREG *_txreg, _RCREG *_rcreg)
{
    assert(_txreg && _rcreg);

    pir = _pir;

    spbrg.txsta = &txsta;
    spbrg.rcsta = &rcsta;

    txreg = _txreg;

    txreg->assign_rcsta(&rcsta);
    txreg->assign_txsta(&txsta);

    rcreg = _rcreg;
    rcreg->assign_rcsta(&rcsta);

    txsta.txreg = txreg;
    txsta.rcsta = &rcsta;
    txsta.spbrg = &spbrg;
    txsta.bit_count = 0;
    txsta.setIOpin(tx_pin);

    rcsta.rcreg = rcreg;
    rcsta.spbrg = &spbrg;
    rcsta.txsta = &txsta;
    rcsta.txreg = txreg;
    rcsta.setIOpin(rx_pin);
}

void USART_MODULE::setIOpin(PinModule *pin, int data)
{
    switch (data)
    {
    case TX_PIN:
        txsta.setIOpin(pin);
        break;

    case RX_PIN:
        rcsta.setIOpin(pin);
        break;

    case CK_PIN:
        txsta.setCKpin(pin);
        break;

    }
}

bool USART_MODULE::bIsTXempty()
{
    if (m_txif)
        return m_txif->Get();
    return pir ? pir->get_txif() : true;
}

void USART_MODULE::emptyTX()
{
    Dprintf(("usart::empty - setting TXIF %s\n", txsta.name().c_str()));

    if (txsta.bTXEN())
    {
        if (m_txif)
            m_txif->Trigger();
        else if (pir)
            pir->set_txif();
        else
            assert(pir);
    }
}

void USART_MODULE::full()
{
    Dprintf((" txreg::full - clearing TXIF\n"));
    if (m_txif)
        m_txif->Clear();
    else if(pir)
        pir->clear_txif();
    else
        assert(pir);
}

void USART_MODULE::set_rcif()
{
    Dprintf((" - setting RCIF\n"));
    if (m_rcif)
        m_rcif->Trigger();
    else if(pir)
        pir->set_rcif();
}

void USART_MODULE::clear_rcif()
{
    Dprintf((" - clearing RCIF\n"));
    if (m_rcif)
        m_rcif->Clear();
    else if(pir)
        pir->clear_rcif();
}

//--------------------------------------------------
USART_MODULE::USART_MODULE(Processor *pCpu)
    : txsta(pCpu,"","USART Transmit Status",this),    // Don't set names incase there are two UARTS
      rcsta(pCpu,"","USART Receive Status",this),
      spbrg(pCpu,"","Serial Port Baud Rate Generator"),
      txreg(nullptr), rcreg(nullptr), pir(nullptr),
      spbrgh(pCpu,"spbrgh","Serial Port Baud Rate high byte"),
      baudcon(pCpu,"baudcon","Serial Port Baud Rate Control"),
      is_eusart(false)
{
    baudcon.txsta = &txsta;
    baudcon.rcsta = &rcsta;
}

USART_MODULE::~USART_MODULE()
{
}

void USART_MODULE::mk_rcif_int(PIR *reg, unsigned int bit)
{
    m_rcif = std::unique_ptr<InterruptSource>(new InterruptSource(reg, bit));
}

void USART_MODULE::mk_txif_int(PIR *reg, unsigned int bit)
{
    m_txif = std::unique_ptr<InterruptSource>(new InterruptSource(reg, bit));
}

//--------------------------------------------------
void USART_MODULE::set_eusart(bool is_it)
{
    if (is_it)
    {
        spbrgh.assign_spbrg(&spbrg);
        spbrg.baudcon = &baudcon;
        spbrg.brgh = &spbrgh;
        is_eusart = true;
    }
    else
    {
        spbrgh.assign_spbrg(0);
        spbrg.baudcon = 0;
        spbrg.brgh = 0;
        is_eusart = false;
    }
}

