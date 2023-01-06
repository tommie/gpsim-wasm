/*
   Copyright (C) 2017,2018 Roy R. Rankin

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

// Complimentry Waveform Generated (CWG) module

//#define DEBUG
#if defined(DEBUG)
#include "../config.h"
#define Dprintf(arg) {printf("%s:%d ",__FILE__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

#include <assert.h>
#include <algorithm>
#include <iostream>

#include "cwg.h"
#include "gpsim_time.h"
#include "ioports.h"
#include "processor.h"
#include "stimuli.h"
#include "trace.h"


// Set pin direction
class TristateControl : public SignalControl
{
public:
    TristateControl(CWG *_cwg, PinModule *_pin)
        : m_cwg(_cwg), m_pin(_pin)
    {}
    ~TristateControl() {}

    void set_pin_direction(char _direction)
    {
        direction = _direction;
    }
    char getState() override
    {
        return direction;
    }
    void release() override
    {
        m_cwg->releasePin(m_pin);
    }

private:
    CWG *m_cwg;
    PinModule *m_pin;
    char	direction = 0;
};


class CWGSignalSource : public SignalControl
{
public:
    CWGSignalSource(CWG *_cwg, PinModule *_pin)
        : m_cwg(_cwg), m_pin(_pin), m_state('?')
    {
        assert(m_cwg);
    }
    virtual ~CWGSignalSource() {}

    void setState(char _state)
    {
        m_state = _state;
    }
    char getState() override
    {
        return m_state;
    }
    void release() override
    {
        Dprintf(("CWGSignalSource release\n"));
        m_cwg->releasePinSource(m_pin);
    }

private:
    CWG *m_cwg;
    PinModule *m_pin;
    char m_state;
};


// Report state changes on incoming FLT pin
class FLTSignalSink : public SignalSink
{
public:
    explicit FLTSignalSink(CWG *_cwg)
        : m_cwg(_cwg)
    {
    }

    void setSinkState(char new3State) override
    {
        m_cwg->setState(new3State);
    }
    void release() override
    {
        delete this;
    }

private:
    CWG *m_cwg;
};


CWG::CWG(Processor *pCpu) :
    cwg1con0(this, pCpu, "cwg1con0", "CWG Control Register 0"),
    cwg1con1(this, pCpu, "cwg1con1", "CWG Control Register 1"),
    cwg1con2(this, pCpu, "cwg1con2", "CWG Control Register 2"),
    cwg1dbf(this, pCpu, "cwg1dbf", "CWG Falling Dead-Band Count Register"),
    cwg1dbr(this, pCpu, "cwg1dbr", "CWG Rising Dead-Band Count Register"),
    cpu(pCpu)
{
    std::fill_n(pwm_state, 4, false);
    std::fill_n(clc_state, 4, false);
}


CWG::~CWG()
{
    if (Atri)
    {
        if (pinAactive)
        {
            releasePin(pinA);
        }

        delete Atri;
        delete Asrc;
    }

    if (Btri)
    {
        if (pinBactive)
        {
            releasePin(pinB);
        }

        delete Btri;
        delete Bsrc;
    }
}


void CWG::setState(char state)
{
    Dprintf(("CWG::setState state=%c\n", state));

    if (state == '0' && FLTstate)  	// new low edge
    {
        con2_value |= GxASE;
        cwg1con2.put_value(con2_value);
        autoShutEvent(true);
        active_next_edge = false;

    }
    else if (state == '1' && !FLTstate)
    {
        con2_value &= ~GxASE;
        cwg1con2.put_value(con2_value);
        active_next_edge = true;
    }

    FLTstate = (state != '0');
}


void CWG::set_IOpins(PinModule *p1, PinModule *p2, PinModule *_pinFLT)
{
    pinA = p1;
    pinB = p2;
    pinFLT = _pinFLT;

    if (Atri)
    {
        delete Atri;
        delete Asrc;
    }

    Atri = new TristateControl(this, pinA);
    Asrc = new CWGSignalSource(this, pinA);

    if (Btri)
    {
        delete Btri;
        delete Bsrc;
    }

    Btri = new TristateControl(this, pinB);
    Bsrc = new CWGSignalSource(this, pinB);
}


//------------------------------------------------------------
// setIOpin - reassign the I/O pin associated with one of the
// outputs that go through an APF module on some devices.
void CWG::setIOpin ( PinModule *newPinModule, int arg )
{
    switch ( arg )
    {
      case A_PIN :
        delete Atri;
        delete Asrc;
        Atri = new TristateControl(this, newPinModule);
        Asrc = new CWGSignalSource(this, newPinModule);
        break;

      case B_PIN :
        delete Btri;
        delete Bsrc;
        Btri = new TristateControl(this, newPinModule);
        Bsrc = new CWGSignalSource(this, newPinModule);
        break;
    }
}


void CWG::oeA()
{
    Dprintf(("CWG::oeA() %u %u\n", (con0_value & GxEN), (con0_value & GxOEA)));

    if ((con0_value & GxEN) && (con0_value & GxOEA))
    {
        if (!pinAactive)
        {
            Agui = pinA->getPin().GUIname();
            pinA->getPin().newGUIname("CWGA");
            Atri->set_pin_direction('0');
            pinA->setControl(Atri);
            pinA->setSource(Asrc);
            pinA->updatePinModule();
            pinAactive = true;
            srcAactive = true;
        }

    }
    else if (pinAactive)
    {
        if (Agui.length())
        {
            pinA->getPin().newGUIname(Agui.c_str());

        }
        else
        {
            pinA->getPin().newGUIname(pinA->getPin().name().c_str());
        }

        pinA->setControl(0);
        pinA->setSource(0);
        pinA->updatePinModule();
        pinAactive = false;
        srcAactive = false;
    }
}


void CWG::oeB()
{
    if ((con0_value & GxEN) && (con0_value & GxOEB))
    {
        if (!pinBactive)
        {
            Bgui = pinB->getPin().GUIname();
            pinB->getPin().newGUIname("CWGB");
            Btri->set_pin_direction('0');
            pinB->setControl(Btri);
            pinB->setSource(Bsrc);
            pinB->updatePinModule();
            pinBactive = true;
            srcBactive = true;
        }

    }
    else if (pinBactive)
    {
        if (Bgui.length())
        {
            pinB->getPin().newGUIname(Bgui.c_str());

        }
        else
        {
            pinB->getPin().newGUIname(pinB->getPin().name().c_str());
        }

        pinB->setControl(0);
        pinB->setSource(0);
        pinB->updatePinModule();
        pinBactive = false;
        srcBactive = false;
    }
}


void CWG::cwg_con0(unsigned int value)
{
    unsigned int diff = con0_value ^ value;
    con0_value = value;

    if (diff & GxEN)
    {
        if (diff & GxOEA) oeA();
        if (diff & GxOEB) oeB();

    }
}


void CWG::cwg_con1(unsigned int value)
{
    con1_value = value;
}


void CWG::cwg_con2(unsigned int value)
{
    unsigned int diff = value ^ con2_value;
    con2_value = value;

    if (diff & GxASE)
    {
        if (value & GxASE)
        {
            if (value & GxARSEN)
            {
                active_next_edge = true;
            }

            autoShutEvent(true);

        }
        else
        {
            if (shutdown_active)
            {
                active_next_edge = true;
                autoShutEvent(false);
            }
        }
    }

    if (diff & GxASDFLT)
    {
        enableAutoShutPin(value & GxASDFLT);
    }
}


void CWG::autoShutEvent(bool on)
{
    if (on)
    {
        Dprintf(("CWG::autoShutEvent on A 0x%x\n", con1_value & (GxASDLA0 | GxASDLA1)));

        switch (con1_value & (GxASDLA0 | GxASDLA1))
        {
        case 0:		// to inactive state
            cwg1dbr.new_edge(false, 0.0);
            break;

        case GxASDLA0:   	// pin tristated
            cwg1dbr.kill_callback();
            Atri->set_pin_direction('1');
            pinA->updatePinModule();
            break;

        case GxASDLA1: 		// pin to 0
            cwg1dbr.kill_callback();
            Asrc->setState('0');
            pinA->updatePinModule();
            break;

        case GxASDLA0|GxASDLA1: // pin to 1
            cwg1dbr.kill_callback();
            Asrc->setState('1');
            pinA->updatePinModule();
            break;
        }

        Dprintf(("CWG::autoShutEvent on B 0x%x\n", con1_value & (GxASDLB0 | GxASDLB1)));

        switch (con1_value & (GxASDLB0 | GxASDLB1))
        {
        case 0:		// to inactive state
            cwg1dbf.new_edge(true, 0.0);
            break;

        case GxASDLB0:   	// pin tristated
            cwg1dbf.kill_callback();
            Btri->set_pin_direction('1');
            pinB->updatePinModule();
            break;

        case GxASDLB1: 		// pin to 0
            cwg1dbf.kill_callback();
            Bsrc->setState('0');
            pinB->updatePinModule();
            break;

        case GxASDLB0|GxASDLB1: // pin to 1
            cwg1dbf.kill_callback();
            Bsrc->setState('1');
            pinB->updatePinModule();
            break;
        }

        shutdown_active = true;

    }
    else
    {
        shutdown_active = false;
        Atri->set_pin_direction('0');
        pinA->updatePinModule();
        Btri->set_pin_direction('0');
        pinB->updatePinModule();
    }
}


void CWG::enableAutoShutPin(bool on)
{
    if (on)
    {
        FLTgui = pinFLT->getPin().GUIname();
        pinFLT->getPin().newGUIname("_FLT");

        if (!FLTsink)
        {
            FLTsink = new FLTSignalSink(this);
            pinFLT->addSink(FLTsink);
            FLTstate = pinFLT->getPin().getState();
            Dprintf(("CWG::enableAutoShutPin FLTstate=%x\n", FLTstate));
        }

    }
    else
    {
        if (FLTgui.length())
        {
            pinFLT->getPin().newGUIname(FLTgui.c_str());

        }
        else
        {
            pinFLT->getPin().newGUIname(pinFLT->getPin().name().c_str());
        }

        if (FLTsink)
        {
            pinFLT->removeSink(FLTsink);
            FLTsink->release();
            FLTsink = nullptr;
        }
    }
}


void CWG::releasePin(PinModule *pin)
{
    if (pin)
    {
        Dprintf(("CWG::releasePin %s pinAactive %d pinBactive %d\n", pin->getPin().name().c_str(), pinAactive, pinBactive));
        pin->getPin().newGUIname(pin->getPin().name().c_str());
        pin->setControl(0);

        if (pin == pinA)
        {
            pinAactive = false;
        }

        if (pin == pinB)
        {
            pinBactive = false;
        }
    }
}


void CWG::releasePinSource(PinModule *pin)
{
    Dprintf(("CWG::releasePinSource %p\n", pin));

    if (pin)
    {
        if (pin == pinA)
        {
            srcAactive = false;
        }

        if (pin == pinB)
        {
            srcBactive = false;
        }

        //pin->setSource(0);
    }
}


void CWG::input_source(bool level)
{
    if (level && active_next_edge)
    {
        con2_value &= ~GxASE;
        cwg1con2.put_value(con2_value);
        autoShutEvent(false);
        active_next_edge = false;
    }

    if (!shutdown_active)
    {
        double mult = (con0_value & GxCS0) ? 16e6 / cpu->get_frequency() : 1;
        cwg1dbr.new_edge(level, mult);
        cwg1dbf.new_edge(!level, mult);
    }
}


// Catch pwm output change
void CWG::out_pwm(bool level, char index)
{
    if (index >= 2)
    {
        return;
    }

    if ((level != pwm_state[index - 1])
            && (con0_value & GxEN)
            && ((int)(con1_value & (GxIS0 | GxIS1)) == index - 1))
    {
        Dprintf(("CWG::out_pwm level=%d shutdown_active=%d con2=0x%x\n", level, shutdown_active, con2_value));
        input_source(level);
    }

    pwm_state[index - 1] = level;
}


// catch CLC (Config Logic Cell) output
void CWG::out_CLC(bool level, char index)
{
    assert(index > 1);

    if ((level != clc_state[index - 1])
            && (con0_value & GxEN)
            && ((int)(con1_value & (GxIS0 | GxIS1)) == 3))
    {
        Dprintf(("CWG::out_clc level=%d shutdown_active=%d con2=0x%x\n", level, shutdown_active, con2_value));
        input_source(level);
    }

    clc_state[index - 1] = level;
}


// Catch NCO (Numerically Controlled Oscillator) output
void CWG::out_NCO(bool level)
{
    if ((level != nco_state)
            && (con0_value & GxEN)
            && ((int)(con1_value & (GxIS0 | GxIS1)) == 2))
    {
        Dprintf(("CWG::out_NCO level=%d shutdown_active=%d con2=0x%x\n", level, shutdown_active, con2_value));
        input_source(level);
    }

    nco_state = level;
}


void CWG::set_outA(bool level)
{
    bool invert = con0_value & GxPOLA;
    Dprintf(("CWG::set_outA now=%" PRINTF_GINT64_MODIFIER "d level=%d invert=%d out=%d\n", get_cycles().get(), level, invert, level ^ invert));
    Asrc->setState((level ^ invert) ? '1' : '0');
    pinA->updatePinModule();
}


void CWG::set_outB(bool level)
{
    bool invert = con0_value & GxPOLB;
    Dprintf(("CWG::set_outB now=%" PRINTF_GINT64_MODIFIER "d level=%d invert=%d out=%d\n", get_cycles().get(), level, invert, level ^ invert));
    Bsrc->setState((level ^ invert) ? '1' : '0');
    pinB->updatePinModule();
}


CWG4::CWG4(Processor *pCpu) : CWG(pCpu)
{
    cwg1con1.set_con1_mask(0xf7);
}


// catch PWM (Pulse Width Mod) output
void CWG4::out_pwm(bool level, char index)
{
    if (index >= 4)
    {
        return;
    }

    if ((level != pwm_state[index - 1])
            && (con0_value & GxEN)
            && ((int)(con1_value & (GxIS0 | GxIS1 | GxIS2)) == index - 1))
    {
        Dprintf(("CWG4::out_pwm level=%d shutdown_active=%d con2=0x%x\n", level, shutdown_active, con2_value));
        input_source(level);
    }

    pwm_state[index - 1] = level;
}


// Catch NCO (Numerically Controlled Oscillator) output
void CWG4::out_NCO(bool level)
{
    if ((level != nco_state)
            && (con0_value & GxEN)
            && ((int)(con1_value & (GxIS0 | GxIS1 | GxIS2)) == 6))
    {
        Dprintf(("CWG4::out_NCO level=%d shutdown_active=%d con2=0x%x\n", level, shutdown_active, con2_value));
        input_source(level);
    }

    nco_state = level;
}


CWGxCON0::CWGxCON0(CWG* pt, Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc), pt_cwg(pt), con0_mask(0xf9)
{
}


void CWGxCON0::put(unsigned int new_value)
{
    new_value &= con0_mask;

    if (!(new_value ^ value.get()))
    {
        return;
    }

    trace.raw(write_trace.get() | value.get());
    value.put(new_value);
    pt_cwg->cwg_con0(new_value);
}


CWGxCON1::CWGxCON1(CWG *pt, Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc), pt_cwg(pt), con1_mask(0xf3)
{
}


void CWGxCON1::put(unsigned int new_value)
{
    new_value &= con1_mask;
    unsigned int diff = new_value ^ value.get();

    if (!diff)
    {
        return;
    }

    trace.raw(write_trace.get() | value.get());
    value.put(new_value);
    pt_cwg->cwg_con1(new_value);
}


CWGxCON2::CWGxCON2(CWG *pt, Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc), pt_cwg(pt), con2_mask(0xc3)
{
}


void CWGxCON2::put(unsigned int new_value)
{
    new_value &= con2_mask;
    unsigned int diff = new_value ^ value.get();

    if (!diff)
    {
        return;
    }

    trace.raw(write_trace.get() | value.get());
    value.put(new_value);
    pt_cwg->cwg_con2(new_value);
}


CWGxDBF::CWGxDBF(CWG *pt, Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc), pt_cwg(pt)
{
}


void CWGxDBF::callback()
{
    Dprintf(("CWGxDBF::callback() %" PRINTF_GINT64_MODIFIER "d\n", get_cycles().get()));
    pt_cwg->set_outB(next_level);
    future_cycle = 0;
}


void CWGxDBF::callback_print()
{
    std::cout << "CWGxDBF " << name() << " CallBack ID " << CallBackID << '\n';
}


void CWGxDBF::kill_callback()
{
    if (future_cycle)
    {
        Dprintf(("CWGxDBF::kill_callback() clear future_cycle=%" PRINTF_GINT64_MODIFIER "d\n", future_cycle));
        get_cycles().clear_break(future_cycle);
        future_cycle = 0;
    }
}


void CWGxDBF::new_edge(bool level, double multi)
{
    /* gpsim delay increment is Fosc/4 which is 1/4
       resolution of deadband, so deadband is approximate
    */
    int delay = (value.get() * multi + 2) / 4;
    next_level = level;
    Dprintf(("CWGxDBF::new_edge now=%" PRINTF_GINT64_MODIFIER "d f=%.0f level=%d delay=%d\n", get_cycles().get(), ((Processor *)cpu)->get_frequency(), level, delay));

    if (future_cycle)
    {
        Dprintf(("\t clear future_cycle=%" PRINTF_GINT64_MODIFIER "d\n", future_cycle));
        get_cycles().clear_break(future_cycle);
        future_cycle = 0;
    }

    if (!delay || !level)
    {
        pt_cwg->set_outB(next_level);

    }
    else
    {
        future_cycle = get_cycles().get() + delay;
        get_cycles().set_break(future_cycle, this);
    }
}


CWGxDBR::CWGxDBR(CWG *pt, Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc), pt_cwg(pt)
{
}


void CWGxDBR::kill_callback()
{
    if (future_cycle)
    {
        Dprintf(("CWGxDBR::kill_callback() clear future_cycle=%" PRINTF_GINT64_MODIFIER "d\n", future_cycle));
        get_cycles().clear_break(future_cycle);
        future_cycle = 0;
    }
}


void CWGxDBR::new_edge(bool level, double multi)
{
    /* gpsim delay increment is Fosc/4 which is 1/4
       resolution of deadband, so deadband is approximate
    */
    int delay = (value.get() * multi + 2) / 4;
    next_level = level;
    Dprintf(("CWGxDBR::new_edge now=%" PRINTF_GINT64_MODIFIER "d f=%.0f level=%d delay=%d\n", get_cycles().get(), ((Processor *)cpu)->get_frequency(), level, delay));

    if (future_cycle)
    {
        Dprintf(("clear future_cycle=%" PRINTF_GINT64_MODIFIER "d\n", future_cycle));
        get_cycles().clear_break(future_cycle);
        future_cycle = 0;
    }

    if (!delay || !level)
    {
        pt_cwg->set_outA(next_level);

    }
    else
    {
        future_cycle = get_cycles().get() + delay;
        get_cycles().set_break(future_cycle, this);
    }
}


void CWGxDBR::callback()
{
    Dprintf(("CWGxDBR::callback() %" PRINTF_GINT64_MODIFIER "d\n", get_cycles().get()));
    pt_cwg->set_outA(next_level);
    future_cycle = 0;
}


void CWGxDBR::callback_print()
{
    std::cout << "CWGxDBR " << name() << " CallBack ID " << CallBackID << '\n';
}


class COGSignalSource : public SignalControl
{
public:
    COGSignalSource(COG *_cog, int _index)
        : m_cog(_cog),
          state('?'), index(_index)
    {
        assert(m_cog);
    }
    virtual ~COGSignalSource() { }

    void setState(char m_state) { state = m_state; }
    char getState() override { return state; }
    void release() override { m_cog->releasePins(index); }

private:
    COG *m_cog;
    char state;
    int index;
};

class COGTristate : public SignalControl
{
public:
    COGTristate() {}
    ~COGTristate() {}
    char getState() override { return '0'; }   // set port as output
    void release() override {}
};

class COGSink : public SignalSink
{
public:
    explicit COGSink(COG *_cog)
        : m_cog(_cog)
    {
        assert(_cog);
    }

    void setSinkState(char new3State) override { m_cog->cogx_in(new3State); }
    void release() override {}
private:
    COG *m_cog;
};



//---------------------------------------------------------------------


COG::COG(Processor *pCpu, const char *pName) :
    cogxcon0(this, pCpu, "cog1con0", "COG Control Register 0"),
    cogxcon1(this, pCpu, "cog1con1", "COG Control Register 1"),
    cogxris(this, pCpu, "cog1ris", "COG Rising Event Input Selection Register"),
    cogxrsim(this, pCpu, "cog1rsim", "COG Rising Event Source Input Mode Registe"),
    cogxfis(this, pCpu, "cog1fis", "COG Falling Event Input Selection Register"),
    cogxfsim(this, pCpu, "cog1fsim", "COG Falling Event Source Input Mode Register"),
    cogxasd0(this, pCpu, "cog1asd0", "COG Auto-shutdown Control Register 0"),
    cogxasd1(this, pCpu, "cog1asd1", "COG Auto-shutdown Control Register 1"),
    cogxstr(this, pCpu, "cog1str", "COG Steering Control Register"),
    cogxdbr(this, pCpu, "cog1dbr", "COG Rising Event Dead-band Count Register"),
    cogxdbf(this, pCpu, "cog1dbf", "COG Falling Event Dead-band Count Register"),
    // RP - @bug - the next two registers are never referenced?
    cogxblkr(this, pCpu, "cog1blkr", "COG Rising Event Blanking Count Register"),
    cogxblkf(this, pCpu, "cog1blkf", "COG Falling Event Blanking Count Register"),
    cogxphr(this, pCpu, "cog1phr", "COG Rising Edge Phase Delay Count Register"),
    cogxphf(this, pCpu, "cog1phf", "COG Falling Edge Phase Delay Count Register"),
    cpu(pCpu), name_str(pName),
    pinIN(nullptr), cogSink(nullptr), set_cycle(0), reset_cycle(0),
    delay_source0(false), delay_source1(false),
    bridge_shutdown(false), input_set(true), input_clear(false),
    full_forward(true), push_pull_level(false)
{
    std::fill_n(m_PinModule, 4, nullptr);
    std::fill_n(m_source, 4, nullptr);
    std::fill_n(source_active, 4, false);
    std::fill_n(active_high, 4, true);
    std::fill_n(steer_ctl, 4, false);

    m_tristate = new COGTristate();
}

COG::~COG()
{
    delete m_tristate;
    delete cogSink;

    for (int i = 0; i < 4; i++)
    {
        delete m_source[i];
    }
}

// set or switch output pin, i = 0-3 for outputs A-D
void COG::setIOpin(PinModule * _pin, int i)
{

    if (i > 3)	// Input_pin
    {
        Dprintf(("COG::setpinIN i=%d pin=%s\n", i, _pin->getPin().name().c_str()));
        // remove cogSink from old input pin
        if (cogSink)
        {
            pinIN->removeSink(cogSink);
            delete cogSink;
            cogSink = nullptr;
        }
        pinIN = _pin;
        set_inputPin();
        return;
    }
    // Disconnect existing output pin
    if (m_PinModule[i] && m_PinModule[i] != _pin && m_source[i])
    {
        m_PinModule[i]->setSource(0);
        m_PinModule[i]->setControl(0);
        delete m_source[i];
        m_source[i] = nullptr;
    }
    m_PinModule[i] = _pin;
    if (_pin)
        set_outputPins();
}

void COG::set_outputPins()
{
    char name[] = "COGA";
    for (int i = 0; i < 4; i++)
    {
        if (cogxcon0.value.get() & GxEN)
        {
            // COG is enabled, setup defines output pins that are not enabled.
            if (m_PinModule[i] && !m_source[i])
            {
                // RP : It's unclear to me why the m_source is erased on disable
                m_source[i] = new COGSignalSource(this, i);
                m_PinModule[i]->setSource(m_source[i]);
                m_PinModule[i]->setControl(m_tristate);
                name[3] = 'A'+i;
                m_PinModule[i]->getPin().newGUIname(name);
                m_PinModule[i]->updatePinModule();
            }
        }
        else
        {
            // COG not enabled, disconnect active output pins
            if (m_PinModule[i] && m_source[i])
            {
                m_PinModule[i]->setSource(0);
                m_PinModule[i]->setControl(0);
                delete m_source[i];
                m_source[i] = nullptr;
                m_PinModule[i]->getPin().newGUIname(m_PinModule[i]->getPin().name().c_str());
            }
        }
    }
}
// connect or disconnect pinIN from COG based on COGxCON1, COGxRIS and COGxFIS
void COG::set_inputPin()
{

    if ((cogxcon0.value.get() & GxEN) &&
            ((cogxris.value.get() & 1) || (cogxfis.value.get() & 1)))
    {
        if (!cogSink)
        {
            cogSink = new COGSink(this);
            pinIN->addSink(cogSink);
            char name[] = "COGIN";
            pinIN->getPin().newGUIname(name);
        }
    }
    else	// pinIN no longer active in COG
    {
        if (cogSink)
        {
            pinIN->removeSink(cogSink);
            delete cogSink;
            cogSink = nullptr;
            pinIN->getPin().newGUIname(pinIN->getPin().name().c_str());
        }

    }
}

void COG::input_event(int index, bool level)
{
    unsigned int mask = 1 << index;
    bool enabled = cogxcon0.value.get() & GxEN;

    if (!enabled)
    {
        input_set = level;
        input_clear = !level;
        return;
    }

    bool set = (cogxris.value.get() & mask);
    bool clear = (cogxfis.value.get() & mask);
    bool change = false;
    if (set && (input_set != level))
    {
        input_set = level;
        if (level)
        {
            change = true;
            drive_bridge(level, FIRST_STATE);
        }
    }
    if (clear && (input_clear != !level))
    {
        input_clear = !level;
        if (!level)
        {
            change = true;
            drive_bridge(level, FIRST_STATE);
        }
    }
    if (change && enabled)
    {
        // prod COG
        if ( cogxcon0.value.get() & GxLD )
        {
            // transfer phase, deadband and blanking registers to double buffer
            phase_val[FALLING] = cogxphf.value.get();
            phase_val[RISING]  = cogxphr.value.get();
            deadband_val[FALLING] = cogxdbf.value.get();
            deadband_val[RISING]  = cogxdbr.value.get();
            // RP - @bug - the next two registers are never referenced?
            blank_val[FALLING] = cogxblkf.value.get();
            blank_val[RISING]  = cogxblkr.value.get();
            // clear the LD bit
            cogxcon0.put ( cogxcon0.value.get() & ~GxLD );
        }
    }
}

void COG::cogx_in(char new3State)
{
    bool level = (new3State == '1' || new3State == 'W');
    input_event(0, level);
}

// catch Comparator events
void COG::out_Cx(bool level, char index)
{
    if (index > 2) return;

    input_event(index+1, level);
}

// catch CCP events
void COG::out_ccp(bool level, char index)
{
    if (index >= 3) return;
    input_event(index+4, level);
}
// catch pwm events
void COG::out_pwm(bool level, char index)
{
    if (index >= 3) return;
    input_event(index+4, level);
}
// catch clc events
void COG::out_clc(bool level, char index)
{
    if (index) return;
    input_event(3, level);
}


void COG::cog_con0(unsigned int old)
{
    unsigned int new_value = cogxcon0.value.get();

    if ((old ^ new_value) & GxEN)
    {
        set_inputPin();
        set_outputPins();
    }
    if (!(old & GxEN) && ((new_value & GxMD_MASK) & 2))
        full_forward = !((new_value & GxMD_MASK) & 1);
}

void COG::cog_con1(unsigned int new_value)
{
    unsigned int diff = cogxcon1.value.get() ^ new_value;
    if (!diff) return;

    for (int i = 0; i < 4; i++)
    {
        active_high[i] = !(new_value & (1 << i));
    }
}

void COG::cog_asd0(unsigned int new_value, unsigned int old)
{
    if ((old ^ new_value) & GxASE)
    {
        if (new_value & GxASE)  // GxASE high transition
        {
            bridge_shutdown = true;
            if (!auto_shut_src)
                shutdown_bridge();

            auto_shut_src |= GXASE;
        }
        else			// GxASE low transition
        {
            auto_shut_src &= ~GXASE;
            if ((auto_shut_src==0) && !(new_value & GxARSEN))
            {
                bridge_shutdown = false;
                // recovery is postponed until a rising edge
            }
        }
    }
    else if ((old ^ new_value) & 0x37) // changed output settings
    {
        if (bridge_shutdown)
            shutdown_bridge();
    }
}

void COG::cog_str(unsigned int new_value)
{
    unsigned int diff = cogxstr.value.get() ^ new_value;
    // if steer control changed and COG not enabled or not in sync steer mode
    // update steer_ctl
    if ((diff & 0x0f) && (cogxcon0.value.get() & 0x81) != 0x81)
    {
        for(int i = 0; i < 4; i++)
        {
            steer_ctl[i] = new_value & (1 << i);
        }
    }
}

COGxCON0::COGxCON0(COG *pt, Processor *pCpu, const char *pName, const char *pDesc):
    sfr_register(pCpu, pName, pDesc), pt_cog(pt), mask(0xdf)
{
}

void COGxCON0::put(unsigned int new_value)
{
    unsigned int old = value.get();
    new_value &= mask;
    if (!(new_value ^ old)) return;
    trace.raw(write_trace.get() | value.get());
    value.put(new_value);
    pt_cog->cog_con0(old);
}

COGxCON1::COGxCON1(COG *pt, Processor *pCpu, const char *pName, const char *pDesc):
    sfr_register(pCpu, pName, pDesc), pt_cog(pt), mask(0xcf)
{
}

void COGxCON1::put(unsigned int new_value)
{
    new_value &= mask;
    if (!(new_value ^ value.get())) return;
    trace.raw(write_trace.get() | value.get());
    pt_cog->cog_con1(new_value);
    value.put(new_value);
}

COGxRIS::COGxRIS(COG *pt, Processor *pCpu, const char *pName, const char *pDesc):
    sfr_register(pCpu, pName, pDesc), pt_cog(pt), mask(0x7f)
{
}

void COGxRIS::put(unsigned int new_value)
{
    new_value &= mask;
    if (!(new_value ^ value.get())) return;
    trace.raw(write_trace.get() | value.get());
    value.put(new_value);
    pt_cog->set_inputPin();
}

COGxFIS::COGxFIS(COG *pt, Processor *pCpu, const char *pName, const char *pDesc):
    sfr_register(pCpu, pName, pDesc), pt_cog(pt), mask(0x7f)
{
}


void COGxFIS::put(unsigned int new_value)
{
    new_value &= mask;
    if (!(new_value ^ value.get())) return;
    trace.raw(write_trace.get() | value.get());
    value.put(new_value);
    pt_cog->set_inputPin();
}

COGxRSIM::COGxRSIM(COG *pt, Processor *pCpu, const char *pName, const char *pDesc):
    sfr_register(pCpu, pName, pDesc), pt_cog(pt), mask(0x7f)
{
}

COGxFSIM::COGxFSIM(COG *pt, Processor *pCpu, const char *pName, const char *pDesc):
    sfr_register(pCpu, pName, pDesc), pt_cog(pt), mask(0x7f)
{
}


COGxASD1::COGxASD1(COG *pt, Processor *pCpu, const char *pName, const char *pDesc):
    sfr_register(pCpu, pName, pDesc), pt_cog(pt), mask(0x0f)
{
}

COGxSTR::COGxSTR(COG *pt, Processor *pCpu, const char *pName, const char *pDesc):
    sfr_register(pCpu, pName, pDesc), pt_cog(pt), mask(0xff)
{
}

void COGxSTR::put(unsigned int new_value)
{
    new_value &= mask;
    if (!(new_value ^ value.get())) return;
    trace.raw(write_trace.get() | value.get());
    pt_cog->cog_str(new_value);
    value.put(new_value);
}

COGxDBR::COGxDBR(COG *pt, Processor *pCpu, const char *pName, const char *pDesc):
    sfr_register(pCpu, pName, pDesc), pt_cog(pt), mask(0x3f)
{
}

COGxDBF::COGxDBF(COG *pt, Processor *pCpu, const char *pName, const char *pDesc):
    sfr_register(pCpu, pName, pDesc), pt_cog(pt), mask(0x3f)
{
}

COGxASD0::COGxASD0(COG *pt, Processor *pCpu, const char *pName, const char *pDesc):
    sfr_register(pCpu, pName, pDesc), pt_cog(pt), mask(0xfc)
{
}

void COGxASD0::put(unsigned int new_value)
{
    new_value &= mask;
    unsigned int old = value.get();
    if (!(new_value ^ value.get())) return;
    trace.raw(write_trace.get() | old);
    value.put(new_value);
    pt_cog->cog_asd0(new_value, old);
}

COGxBLKR::COGxBLKR(COG *pt, Processor *pCpu, const char *pName, const char *pDesc):
    sfr_register(pCpu, pName, pDesc), pt_cog(pt), mask(0x3f)
{
}

COGxBLKF::COGxBLKF(COG *pt, Processor *pCpu, const char *pName, const char *pDesc):
    sfr_register(pCpu, pName, pDesc), pt_cog(pt), mask(0x3f)
{
}

COGxPHR::COGxPHR(COG *pt, Processor *pCpu, const char *pName, const char *pDesc):
    sfr_register(pCpu, pName, pDesc), pt_cog(pt), mask(0x3f)
{}

COGxPHF::COGxPHF(COG *pt, Processor *pCpu, const char *pName, const char *pDesc):
    sfr_register(pCpu, pName, pDesc), pt_cog(pt), mask(0x3f)
{}


void COG::output_pin(int pin, bool set)
{
    if (m_PinModule[pin])
    {
        Dprintf(("output_pin %d %s active_high=%d set=%d\n", pin, m_PinModule[pin]->getPin().GUIname().c_str(), active_high[pin], set));
        if ( m_source[pin] ) m_source[pin]->setState((active_high[pin] ^ set) ? '0' : '1');
        m_PinModule[pin]->updatePinModule();
    }
}

void COG::callback()
{
    if (phase_cycle == get_cycles().get())
    {
        phase_cycle = 0;
        if (input_set)
            drive_bridge(1, PHASE_STATE);
        else if (input_clear)
            drive_bridge(0, PHASE_STATE);
    }
    if (set_cycle == get_cycles().get())
    {
        set_cycle = 0;
        drive_bridge(input_set, LAST_STATE);
    }
    if (reset_cycle == get_cycles().get())
    {
        drive_bridge(!input_clear, LAST_STATE);
        reset_cycle = 0;
    }
}

//
//  Drive PWM bridge
//
void COG::drive_bridge(int level, int state)
{
    unsigned int con1_reg = cogxcon1.value.get();
    int phase_delay = 0;
    int deadband_delay = 0;
    double clock_div = 1;

    if (auto_shut_src || bridge_shutdown)
        return;

    /*    if (!(asd0_reg & GxASE)) // auto-shutdown active
        {
    	return;
        }
    */

    int clock = (con1_reg & GxCS_MASK) >> GxCS_SHIFT;
    switch(clock)
    {
    case 0:	//Fosc/4
        clock_div = 1;
        break;

    case 1:	//Fosc
        clock_div = 4;
        break;

    case 2:	//HFINTOSC
        clock_div = 16e6 / cpu->get_frequency ();
        break;
    }
    if (state == FIRST_STATE)
    {
        phase_delay = phase_val[level]/clock_div;
    }
    if (phase_delay == 0 && state <= PHASE_STATE)
    {
        if ( level && !(cogxcon1.value.get() & GxRDBS) )
        {
            deadband_delay = deadband_val[RISING]/clock_div;
        }
        if ( !level && !(cogxcon1.value.get() & GxFDBS) )
        {
            deadband_delay = deadband_val[FALLING]/clock_div;
        }
    }
    switch(cogxcon0.value.get() & GxMD_MASK) // bridge mode
    {
    case 0:	// STEERED PWM MODE
    {
        unsigned int str_reg = cogxstr.value.get();
        Dprintf(("STEERED PWM MODE %s cogxstr=0x%x\n", name().c_str(), str_reg));
        for (int i = 0; i < 4; i++)
        {
            if (steer_ctl[i]) // Steering Control bit
                output_pin(i, level);
            else
            {
                // need to output data regardless of active_high
                bool out = str_reg & (1<<(i+4));
                out ^=  active_high[i];
                output_pin(i, out);
            }
        }
    }
    break;

    case 1:	// Synchronous STEERED PWM MODE
    {
        unsigned int str_reg = cogxstr.value.get();
        Dprintf(("STEERED PWM MODE %s cogxstr=0x%x\n", name().c_str(), str_reg));
        for (int i = 0; i < 4; i++)
        {
            if (level) steer_ctl[i] = str_reg & (1<<i);
            if (steer_ctl[i]) // Steering Control bit
                output_pin(i, level);
            else
            {
                // need to output data regardless of active_high
                bool out = str_reg & (1<<(i+4));
                out ^=  active_high[i];
                output_pin(i, out);
            }
        }
    }
    break;

    case 2:	// Full bidge Forward
        Dprintf(("full-bridge %s, forward level=%d\n", name().c_str(), level));
        if (!full_forward)
        {
            if (!level)	// stay with reverse
            {
                output_pin(0, 0);
                output_pin(1, level);
                output_pin(2, 1);
                output_pin(3, 0);
            }
            else	// switch direction
            {
                output_pin(0, 1);
                output_pin(1, 0);
                output_pin(2, 0);
                full_forward = true;
                deadband_delay = deadband_val[RISING]/clock_div;
                if (cogxcon1.value.get() & GxRDBS && deadband_delay)
                {
                    set_cycle = get_cycles().get() + deadband_delay;
                    get_cycles().set_break(set_cycle, this);
                }
                else
                {
                    output_pin(3, level);
                }
            }
        }
        else
        {

            output_pin(0, 1);
            output_pin(1, 0);
            output_pin(2, 0);
            output_pin(3, level);
        }
        break;

    case 3:	// Full bridge reverse
        Dprintf(("full-bridge %s, reverse level=%d\n", name().c_str(), level));
        if (full_forward)
        {
            if (!level)	// stay with forward
            {
                output_pin(0, 1);
                output_pin(1, 0);
                output_pin(2, 0);
                output_pin(3, level);
            }
            else	// switch direction
            {
                output_pin(0, 0);
                output_pin(2, 1);
                output_pin(3, 0);
                full_forward = false;
                deadband_delay = deadband_val[FALLING]/clock_div;
                if (cogxcon1.value.get() & GxFDBS && deadband_delay)
                {
                    set_cycle = get_cycles().get() + deadband_delay;
                    get_cycles().set_break(set_cycle, this);
                }
                else
                {
                    output_pin(1, level);
                }
            }
        }
        else
        {
            output_pin(0, 0);
            output_pin(1, level);
            output_pin(2, 1);
            output_pin(3, 0);
        }
        break;

    case 4:	// Half-Bridge
        Dprintf(("half-bridge %s cycles=%" PRINTF_GINT64_MODIFIER "d, level=%d state=%d phase=%d deadband=%d\n", name().c_str(), get_cycles().get(), level, state, phase_delay, deadband_delay));

        if (phase_delay)
        {
            phase_cycle = get_cycles().get() + phase_delay;
            get_cycles().set_break(phase_cycle, this);
        }
        else if (deadband_delay)
        {
            if (level)
            {
                set_cycle = get_cycles().get() + deadband_delay;
                get_cycles().set_break(set_cycle, this);
                output_pin(1, !level);
                output_pin(3, !level);
            }
            else
            {
                reset_cycle = get_cycles().get() + deadband_delay;
                get_cycles().set_break(reset_cycle, this);
                output_pin(0, level);
                output_pin(2, level);
            }
        }
        else
        {
            output_pin(0, level);
            output_pin(1, !level);
            output_pin(2, level);
            output_pin(3, !level);
        }
        break;

    case 5:	// Push-Pull
        Dprintf(("Push-Pull %s\n", name().c_str()));
        if (level)
        {
            output_pin(0, !push_pull_level);
            output_pin(1, !push_pull_level);
            output_pin(2, push_pull_level);
            output_pin(3, push_pull_level);
            push_pull_level = !push_pull_level;
        }
        break;

    default:
        printf("%s::pwm_match impossible COG bridge mode\n", name().c_str());
        break;
    }
}

//
// Set PWM bridge into shutdown mode
//
void COG::shutdown_bridge()
{
    bridge_shutdown = true;

    unsigned int COGxasd0 = cogxasd0.value.get();
    Dprintf(("cogxasd0=0x%x\n", COGxasd0));

    switch ((COGxasd0 & GxASDBD_MSK) >> GxASDBD_SFT)
    {
    case 3:	// B, D output 1
        output_pin(1, 1);
        output_pin(3, 1);
        break;

    case 2:	// B D output 0
        output_pin(1, 0);
        output_pin(3, 0);
        break;

    case 1:	// B D tristate
        if (m_PinModule[1])  m_PinModule[1]->setControl(m_tristate);
        if (m_PinModule[3])  m_PinModule[3]->setControl(m_tristate);
        break;

    case 0:	// B D inactive state
        break;
    }

    switch ((COGxasd0 & GxASDAC_MSK) >> GxASDAC_SFT)
    {
    case 3:	// A, C output 1
        output_pin(0, 1);
        output_pin(2, 1);
        break;

    case 2:	// A, C output 0
        output_pin(0, 0);
        output_pin(2, 0);
        break;

    case 1:	// A C tristate
        if(m_PinModule[0])  m_PinModule[0]->setControl(m_tristate);
        if(m_PinModule[2])  m_PinModule[2]->setControl(m_tristate);
        break;

    case 0:	// A C inactive state
        break;
    }
    m_PinModule[0]->updatePinModule();
    if (m_PinModule[1]) m_PinModule[1]->updatePinModule();
    if (m_PinModule[2]) m_PinModule[2]->updatePinModule();
    if (m_PinModule[3]) m_PinModule[3]->updatePinModule();
}
