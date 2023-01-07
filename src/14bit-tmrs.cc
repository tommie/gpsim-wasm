/*
/
   Copyright (C) 1998 T. Scott Dattalo
   Copyright (C) 2006,2009,2010,2013,2015,2017,2020 Roy R Rankin

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

#include <assert.h>
#include <stdio.h>
#include <algorithm>
#include <iostream>
#include <string>

#include "14bit-tmrs.h"

#include "a2dconverter.h"
#include "clc.h"
#include "cwg.h"
#include "gpsim_interface.h"
#include "gpsim_time.h"
#include "pir.h"
#include "zcd.h"
#include "at.h"
#include "processor.h"
#include "ssp.h"
#include "stimuli.h"
#include "trace.h"
#include "ui.h"
#include "value.h"
#include "comparator.h"
#include <bitset>


//
// 14bit-tmrs.cc
//
// Timer 1&2  modules for the 14bit core pic processors.
//
#define DEBUG 0
#if DEBUG & 1
#define RRprint(arg) {fprintf(stderr, "%s:%d %s ",__FILE__,__LINE__, __FUNCTION__); fprintf arg;}
#else
#define RRprint(arg) {}
#endif
#if DEBUG & 2
#define Dprintf(arg) {printf("%s:%d %s ",__FILE__,__LINE__, __FUNCTION__); printf arg; }
#else
#define Dprintf(arg) {}
#endif


//--------------------------------------------------
// CCPRL
//--------------------------------------------------

CCPRL::CCPRL(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc)
{
}


bool CCPRL::test_compare_mode()
{
    return tmrl && ccpcon && ccpcon->test_compare_mode();
}


void CCPRL::put(unsigned int new_value)
{
    trace.raw(write_trace.get() | value.get());
    value.put(new_value);

    if (test_compare_mode())
    {
        start_compare_mode();  // Actually, re-start with new capture value.
    }
}


void CCPRL::capture_tmr()
{
    tmrl->get_low_and_high();
    trace.raw(write_trace.get() | value.get());
    value.put(tmrl->value.get());
    trace.raw(ccprh->write_trace.get() | ccprh->value.get());
    ccprh->value.put(tmrl->tmrh->value.get());

    if (verbose & 4)
    {
        int c = value.get() + 256 * ccprh->value.get();
        std::cout << name() << " CCPRL captured: tmr=" << c << '\n';
    }
}


void CCPRL::start_compare_mode(CCPCON *ref)
{
    int capture_value = value.get() + 256 * ccprh->value.get();

    if (verbose & 4)
    {
        std::cout << name() << " start compare mode with capture value = " << capture_value << '\n';
    }

    if (ref)
    {
        ccpcon = ref;
    }

    RRprint((stderr, "CCPRL::start_compare_mode %s ccpcon=%s\n", name().c_str(), ccpcon?ccpcon->name().c_str():"UNDEF"));
    if (ccpcon)
    {
        tmrl->set_compare_event(capture_value, ccpcon);

    }
    else
    {
        std::cout << name() << " CPRL: Attempting to set a compare callback with no CCPCON\n";
    }
}


void CCPRL::stop_compare_mode()
{
    // If this CCP is in the compare mode, then change to non-compare and cancel
    // the tmr breakpoint.
    if (test_compare_mode())
    {
        tmrl->clear_compare_event(ccpcon);
    }

    ccpcon = nullptr;
}


void CCPRL::start_pwm_mode()
{
    //std::out << name() << " CCPRL: starting pwm mode\n";
    ccprh->pwm_mode = true;
}


void CCPRL::stop_pwm_mode()
{
    //std::out << name() << " CCPRL: stopping pwm mode\n";
    ccprh->pwm_mode = false;
}


//--------------------------------------------------
// assign_tmr - assign a new timer to the capture compare module
//
// This was created for the 18f family where it's possible to dynamically
// choose which clock is captured during an event.
//
void CCPRL::assign_tmr(TMRL *ptmr)
{
    if (ptmr)
    {
        std::cout << name() << " Reassigning CCPRL clock source\n";
        tmrl = ptmr;
    }
}


//--------------------------------------------------
// CCPRH
//--------------------------------------------------

CCPRH::CCPRH(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc)
{
}


// put_value allows PWM code to put data
void CCPRH::put_value(unsigned int new_value)
{
    trace.raw(write_trace.get() | value.get());
    value.put(new_value);
}


void CCPRH::put(unsigned int new_value)
{
    //std::cout << name() << " CCPRH put \n";
    if (pwm_mode == false)   // In pwm_mode, CCPRH is a read-only register.
    {
        put_value(new_value);

        if (ccprl && ccprl->test_compare_mode())
        {
            ccprl->start_compare_mode();  // Actually, re-start with new capture value.
        }
    }
}
// Like CCPRH except is writable in PWM mod
void CCPRH_HLT::put(unsigned int new_value)
{
    //std::cout << name() << " CCPRH put \n";
    put_value(new_value);

    if (ccprl && ccprl->test_compare_mode())
    {
      ccprl->start_compare_mode();  // Actually, re-start with new capture value.
    }
}


unsigned int CCPRH::get()
{
    //std::cout << name() << " CCPRH get\n";
    trace.raw(read_trace.get() | value.get());
    return value.get();
}


//--------------------------------------------------
//
//--------------------------------------------------
class CCPSignalSource : public SignalControl
{
public:
    CCPSignalSource(CCPCON *_ccp, int _index)
        : m_ccp(_ccp), state('?'), index(_index)
    {
        assert(m_ccp);
    }
    virtual ~CCPSignalSource() {}

    void setState(char m_state)
    {
        state = m_state;
    }
    char getState() override
    {
        return state;
    }
    void release() override
    {
        m_ccp->releasePins(index);
    }

private:
    CCPCON *m_ccp;
    char state;
    int index;
};


//--------------------------------------------------
//
//--------------------------------------------------

class CCPSignalSink : public SignalSink
{
public:
    CCPSignalSink(CCPCON *_ccp, int _index)
        : m_ccp(_ccp), index(_index)
    {
        assert(_ccp);
    }

    virtual ~CCPSignalSink() {}

    void release() override
    {
        m_ccp->releaseSink();
    }
    void setSinkState(char new3State) override
    {
	unsigned int current = new3State == '1' || new3State == 'W';
	RRprint((stderr, "setSinkState current=%d last=%d\n", current, last));
	if (current != last)
            m_ccp->new_edge(current);
	last = current;
    }

private:
    CCPCON 	*m_ccp;
    unsigned int last = 2;	// initial value = 2, normally 0 or 1
    int 	index;
};


class Tristate : public SignalControl
{
public:
    Tristate() {}

    char getState() override
    {
        return '1';  // set port to high impedance by setting it to input
    }
    void release() override { }
};


//--------------------------------------------------
// CCPCON
//--------------------------------------------------
CCPCON::CCPCON(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc), index('\0'), m_cog(nullptr), m_cwg(nullptr),
      m_cOutputState('?')
{
    std::fill_n(m_clc, 4, nullptr);
    std::fill_n(m_PinModule, 5, nullptr);
    std::fill_n(m_source, 5, nullptr);
    std::fill_n(source_active, 5, false);

    for (int i = 0; i < 5; i++)
	m_source[i] = new CCPSignalSource(this, i);

    mValidBits = 0x3f;
}


CCPCON::~CCPCON()
{
    if (m_Interrupt)
    {
	delete m_Interrupt;
	m_Interrupt = nullptr;
    }

    for (int i = 0; i < 5; i++)
    {
        if (m_source[i])
        {
            if (source_active[i] && m_PinModule[i])
            {
                m_PinModule[i]->setSource(0);
            }

            delete m_source[i];
	    m_source[i] = nullptr;
	    source_active[i] = false;
        }
    }

// Doing this causes core dumps RRR
//    delete m_tristate;
//    m_tristate = nullptr;

    if (m_PinModule[input_pin()] && m_sink && m_bInputEnabled)
    {
        m_PinModule[input_pin()]->removeSink(m_sink);
    }

    delete m_sink;
}


void CCPCON::setIOpin(PinModule *pin, int pin_slot)
{
    if (pin && pin->getPin())
    {
	if (pin_slot > CCP_IN_PIN)
	{
	    fprintf(stderr, "*** ERROR CCPCON::setIOpin invalid pin_slot=%d\n", pin_slot);
	     assert(pin_slot > CCP_IN_PIN);
        }
	// Create source for pin_slot output pin
        if (!m_source[pin_slot])
        {
            m_source[pin_slot] = new CCPSignalSource(this, pin_slot);
        }
	// Where input and output use different pins input_pin() should
	// return CCP_IN_PIN. Where only input or output are used at any time,
	// input_pin() can return CCP_PIN
	if ((unsigned int)pin_slot == input_pin())
	{
	    RRprint((stderr, "Input %s pin_slot=%d m_PinModule[pin_slot]=%p\n", name().c_str(), pin_slot, m_PinModule[pin_slot]));
	    // if m_PinModule is defined, move sink if new pin
	    if (m_PinModule[pin_slot])
	    {
		RRprint((stderr, "%p %p m_bInputEnabled=%d\n", m_PinModule[pin_slot], pin, m_bInputEnabled));
	        if (m_PinModule[pin_slot] != pin && m_bInputEnabled)
                {
                    m_PinModule[pin_slot]->removeSink(m_sink);
                    pin->addSink(m_sink);
		}
	    }
	    else  // create sink for input pin
	    {
		if (!m_sink)
		{
		    m_sink = new CCPSignalSink(this, 0);
		    m_tristate = new Tristate();
		    RRprint((stderr, "pin=%s m_tristat=%p\n", pin->getPin()->name().c_str(), m_tristate));
		}
		RRprint((stderr, "m_sink=%p m_bInputEnabled=%d\n", m_sink, m_bInputEnabled));
		if (m_bInputEnabled)
                    pin->addSink(m_sink);
	    }
	}

        m_PinModule[pin_slot] = pin;
    }
    else
    {
	if (m_PinModule[pin_slot] && source_active[pin_slot])
        {
	    m_PinModule[pin_slot]->setSource(0);
	    source_active[pin_slot] = false;
	}
	if (m_source[pin_slot])
	{
            delete m_source[pin_slot];
            m_source[pin_slot] = nullptr;
	}
	m_PinModule[pin_slot] = nullptr;
    }
}

// remove ccpcon for pwm_mode and release output pins
void CCPCON::stop_pwm()
{
    if (!is_pwm())
    {
	if (tmr2)
    	    tmr2->stop_pwm(address);
	for(int i=0; i<4; i++)
	{
            if (source_active[i])
            {
		config_output(i, false, m_bInputEnabled);
	    }
	}
    }
}


// EPWM has four outputs PWM 1
void CCPCON::setIOpin(PinModule *p1, PinModule *p2, PinModule *p3, PinModule *p4)
{
    Dprintf(("%s::setIOpin %s %s %s %s\n", name().c_str(),
             (p1 && p1->getPin()) ? p1->getPin()->name().c_str() : "unknown",
             (p2 && p2->getPin()) ? p2->getPin()->name().c_str() : "unknown",
             (p3 && p3->getPin()) ? p3->getPin()->name().c_str() : "unknown",
             (p4 && p4->getPin()) ? p4->getPin()->name().c_str() : "unknown"
            ));

    if (p1 && !p1->getPin())
    {
        Dprintf(("FIXME %s::setIOpin called where p1 has unassigned pin\n", name().c_str()));
        return;
    }

    setIOpin(p1, CCP_PIN);
    if (p2) setIOpin(p2, PxB_PIN);
    if (p3) setIOpin(p3, PxC_PIN);
    if (p4) setIOpin(p4, PxD_PIN);
}


void CCPCON::setCrosslinks(CCPRL *_ccprl, PIR *_pir, unsigned int _mask,
                           TMR2 *_tmr2, ECCPAS *_eccpas)
{
    ccprl = _ccprl;
    pir = _pir;
    tmr2 = _tmr2;
    eccpas = _eccpas;
    pir_mask = _mask;
}


void CCPCON::setADCON(ADCON0 *_adcon0)
{
    adcon0 = _adcon0;
}


char CCPCON::getState()
{
    return m_bOutputEnabled ?  m_cOutputState : '?';
}


void CCPCON::new_edge(unsigned int level)
{
    Dprintf(("%s::new_edge() level=%u\n", name().c_str(), level));

    switch (value.get() & (CCPM3 | CCPM2 | CCPM1 | CCPM0))
    {
    case ALL_OFF0:
    case ALL_OFF1:
    case ALL_OFF2:
    case ALL_OFF3:
        Dprintf(("--CCPCON not enabled\n"));
        return;

    case CAP_FALLING_EDGE:
        if (level == 0 && ccprl)
        {
            ccprl->capture_tmr();
	    if (m_Interrupt)
		m_Interrupt->Trigger();
	    else if (pir)
                pir->set_mask(pir_mask);
	    if (ccp_output_server)
	    {
		ccp_output_server->send_data(true, 0);
		ccp_output_server->send_data(false, 0);
	    }
            Dprintf(("--CCPCON caught falling edge\n"));
        }

        break;

    case CAP_RISING_EDGE:
        if (level && ccprl)
        {
            ccprl->capture_tmr();
	    if (m_Interrupt)
		m_Interrupt->Trigger();
	    else if (pir)
                pir->set_mask(pir_mask);
	    if (ccp_output_server)
	    {
		ccp_output_server->send_data(true, 0);
		ccp_output_server->send_data(false, 0);
	    }
            Dprintf(("--CCPCON caught rising edge\n"));
        }

        break;

    case CAP_RISING_EDGE4:
        if (level && ++edge_cnt >= edges)
        {
            ccprl->capture_tmr();
	    if (m_Interrupt)
		m_Interrupt->Trigger();
	    else if (pir)
                pir->set_mask(pir_mask);
	    if (ccp_output_server)
	    {
		ccp_output_server->send_data(true, 0);
		ccp_output_server->send_data(false, 0);
	    }
            edge_cnt = 0;
            Dprintf(("--CCPCON caught 4th rising edge\n"));
        }

        //else std::cout << name() << " Saw rising edge, but skipped\n";
        break;

    case CAP_RISING_EDGE16:
        if (level && ++edge_cnt >= edges)
        {
            ccprl->capture_tmr();
	    if (m_Interrupt)
		m_Interrupt->Trigger();
	    else if (pir)
                pir->set_mask(pir_mask);
	    if (ccp_output_server)
	    {
		ccp_output_server->send_data(true, 0);
		ccp_output_server->send_data(false, 0);
	    }
            edge_cnt = 0;
            Dprintf(("--CCPCON caught 4th rising edge\n"));
        }

        //else std::cout << name() << " Saw rising edge, but skipped\n";
        break;

    case COM_SET_OUT:
    case COM_CLEAR_OUT:
    case COM_INTERRUPT:
    case COM_TRIGGER:
    case PWM0:
    case PWM1:
    case PWM2:
    case PWM3:
        //std::cout << name() << " CCPCON is set up as an output\n";
        return;
    }
}


void CCPCON::compare_match()
{
    Dprintf(("%s::compare_match()\n", name().c_str()));

    switch (value.get() & (CCPM3 | CCPM2 | CCPM1 | CCPM0))
    {
    case ALL_OFF0:
    case ALL_OFF1:
    case ALL_OFF2:
    case ALL_OFF3:
        Dprintf(("-- CCPCON not enabled\n"));
        return;

    case CAP_FALLING_EDGE:
    case CAP_RISING_EDGE:
    case CAP_RISING_EDGE4:
    case CAP_RISING_EDGE16:
        Dprintf(("-- CCPCON is programmed for capture. bug?\n"));
        break;

    case COM_SET_OUT:
        m_cOutputState = '1';
        m_source[0]->setState('1');
        m_PinModule[0]->updatePinModule();

	if (m_Interrupt)
	    m_Interrupt->Trigger();
	else if (pir)
            pir->set_mask(pir_mask);

	if (ccp_output_server)
	{
	    ccp_output_server->send_data(true, 0);
	}

        Dprintf(("-- CCPCON setting compare output to 1\n"));
        break;

    case COM_CLEAR_OUT:
        m_cOutputState = '0';
        m_source[0]->setState('0');
        m_PinModule[0]->updatePinModule();

	if (m_Interrupt)
	    m_Interrupt->Trigger();
	else if (pir)
            pir->set_mask(pir_mask);
	if (ccp_output_server)
	{
	    ccp_output_server->send_data(false, 0);
	}

        Dprintf(("-- CCPCON setting compare output to 0\n"));
        break;

    case COM_INTERRUPT:
	if (m_Interrupt)
	    m_Interrupt->Trigger();
	else if (pir)
            pir->set_mask(pir_mask);

        Dprintf(("-- CCPCON setting interrupt\n"));
        break;

    case COM_TRIGGER:
        if (ccprl)
        {
            ccprl->tmrl->clear_timer();
        }

	if (m_Interrupt)
	    m_Interrupt->Trigger();
	else if (pir)
            pir->set_mask(pir_mask);

        if (adcon0)
        {
            adcon0->start_conversion();
        }

        Dprintf(("-- CCPCON triggering an A/D conversion ccprl %p\n", ccprl));
        break;

    case PWM0:
    case PWM1:
    case PWM2:
    case PWM3:
        //std::cout << name() << " CCPCON is set up as an output\n";
        return;
    }
}


// handle dead-band delay in half-bridge mode
void CCPCON::callback()
{
    if (delay_source0)
    {
        m_source[0]->setState('1');
        m_PinModule[0]->updatePinModule();
        delay_source0 = false;
    }

    if (delay_source1)
    {
        m_source[1]->setState('1');
        m_PinModule[1]->updatePinModule();
        delay_source1 = false;
    }

    if (pulse_clear)
    {
	ccp_out(false, false);
	future_cycle = 0;
	pulse_clear = false;
    }
}


void CCPCON::releaseSink()
{
    delete m_sink;
    m_sink = nullptr;
}


void CCPCON::releasePins(int i)
{
    source_active[i] = false;
}


void CCPCON::simple_pwm_output(int level)
{
   m_cOutputState = level ? '1' : '0';
   m_source[0]->setState(level ? '1' : '0');
   m_PinModule[0]->setSource(m_source[0]);
   source_active[0] = true;
   m_PinModule[0]->updatePinModule();
   RRprint((stderr, "CCPCON::simple_pwm_output %s level=%d\n", name().c_str(), level));
}
void CCPCON::pwm_match(int level)
{
    unsigned int new_value = value.get();
    // return if not pwm mode
    if ((new_value & 0x0c) != 0x0c)
	return;

    RRprint((stderr, "CCPCON::pwm_match %s level=%d\n", name().c_str(), level));
    Dprintf(("%s::pwm_match() level=%d pwm1con = %p value=0x%x now=0x%" PRINTF_GINT64_MODIFIER "x\n", name().c_str(), level, pwm1con, new_value, get_cycles().get()));

    // if the level is 1, then tmr2 == pr2 and the pwm cycle
    // is starting over. In which case, we need to update the duty
    // cycle by reading ccprl and the ccp X & Y and caching them
    // in ccprh's pwm slave register.
    if (level == 1)
    {
        // Auto shutdown comes off at start of PWM if ECCPASE clear
        if (bridge_shutdown && (!eccpas || !(eccpas->get_value() & ECCPAS::ECCPASE)))
        {
            Dprintf(("bridge_shutdown=%d eccpas=%p ECCPAS=%x\n", bridge_shutdown, eccpas,
                     eccpas ? eccpas->get_value() & ECCPAS::ECCPASE : 0));

            for (int i = 0; i < 4; i++)
            {
                if (m_PinModule[i])
                {
                    m_PinModule[i]->setControl(0); //restore default pin direction
                    source_active[i] = false;
                    m_PinModule[i]->updatePinModule();
                }
            }

            bridge_shutdown = false;
        }

        tmr2->pwm_dc(pwm_duty_cycle(), address);
	ccprl2ccprh();
    }

    if (!pwm1con)   // simple PWM
    {
        if (bridge_shutdown == false)   // some processors use shutdown and simple PWM
        {
	RRprint((stderr, "pwm_match simple PwM TMR2 == duty cycle level=%d now=%ld\n", level, get_cycles().get()));
	    simple_pwm_output(level);

            if (level && !pwm_duty_cycle())   // if duty cycle == 0 output stays low
            {
		simple_pwm_output(0);
            }

            //std::cout << name() << " iopin should change\n";
        }

    }
    else  	// EPWM
    {
        if (!bridge_shutdown)
        {
            RRprint((stderr, "CCPCON::pwm_match call drive_bridge level=%d new_value=0x%x\n", level, new_value));
            drive_bridge(level, new_value);
        }
    }
}


//
//  Drive PWM bridge
//
void CCPCON::drive_bridge(int level, int new_value)
{
    unsigned int pstrcon_value;

    // pstrcon allows port steering for "single output"
    // if it is not defined, just use the first port
    if (pstrcon)
    {
        pstrcon_value = pstrcon->value.get();

    }
    else
    {
        pstrcon_value = 1;
    }

    int pwm_width;
    int delay = pwm1con->value.get() & ~PWM1CON::PRSEN;
    bool active_high[4];

    switch (new_value & (CCPM3 | CCPM2 | CCPM1 | CCPM0))
    {
    case PWM0:	//P1A, P1C, P1B, P1D active high
        active_high[0] = true;
        active_high[1] = true;
        active_high[2] = true;
        active_high[3] = true;
        break;

    case PWM1:	// P1A P1C active high P1B P1D active low
        active_high[0] = true;
        active_high[1] = false;
        active_high[2] = true;
        active_high[3] = false;
        break;

    case PWM2: 	// P1A P1C active low P1B P1D active high
        active_high[0] = false;
        active_high[1] = true;
        active_high[2] = false;
        active_high[3] = true;
        break;

    case PWM3:	// //P1A, P1C, P1B, P1D active low
        active_high[0] = false;
        active_high[1] = false;
        active_high[2] = false;
        active_high[3] = false;
        break;

    default:
        std::cout << name() << " not pwm mode. bug?\n";
        return;
        break;
    }

    switch ((new_value & (P1M1 | P1M0)) >> 6)   // ECCP bridge mode
    {
    case 0:	// Single
        Dprintf(("Single bridge %s pstrcon=0x%x\n", name().c_str(), pstrcon_value));

        for (int i = 0; i < 4; i++)
        {
            if (pstrcon_value & (1 << i) && m_PinModule[i])
            {
                m_PinModule[i]->setSource(m_source[i]);
                source_active[i] = true;

                // follow level except where duty cycle = 0
                if (level && pwm_duty_cycle())
                {
                    m_source[i]->setState(active_high[i] ? '1' : '0');
                }
                else
                {
                    m_source[i]->setState(active_high[i] ? '0' : '1');
                }

                m_PinModule[i]->updatePinModule();

            }
            else if (m_PinModule[i])
            {
                m_PinModule[i]->setSource(0);
                source_active[i] = false;
            }
        }

        break;

    case 2:	// Half-Bridge
        Dprintf(("half-bridge %s\n", name().c_str()));
        m_PinModule[0]->setSource(m_source[0]);
        source_active[0] = true;
        m_PinModule[1]->setSource(m_source[1]);
        source_active[1] = true;

        if (m_PinModule[2])
        {
            m_PinModule[2]->setSource(0);
            source_active[2] = false;
        }

        if (m_PinModule[3])
        {
            m_PinModule[3]->setSource(0);
            source_active[3] = false;
        }

        delay_source0 = false;
        delay_source1 = false;
        // FIXME need to add deadband
        // follow level except where duty cycle = 0
        pwm_width = level ?
                    pwm_duty_cycle() :
                    ((tmr2->pr2->value.get() + 1) * 4) - pwm_duty_cycle();

  	RRprint((stderr, "    pwm_duty_cycle()=%d pwm_width=%d flag=%d delay=%d\n", pwm_duty_cycle(), pwm_width, (level ^ active_high[0]), delay));
        if (!(level ^ active_high[0]) && pwm_duty_cycle())
        {
            // No delay, change state
            if (delay == 0)
            {
                m_source[0]->setState('1');

            }
            else if (delay < pwm_width)     // there is a delay
            {
                future_cycle = get_cycles().get() + delay;
                get_cycles().set_break(future_cycle, this);
                delay_source0 = true;
            }

        }
        else
        {
            m_source[0]->setState('0');
        }

        if (!(level ^ active_high[1]) && pwm_duty_cycle())
        {
            m_source[1]->setState('0');

        }
        else
        {
            // No delay, change state
            if (delay == 0)
            {
                m_source[1]->setState('1');

            }
            else if (delay < pwm_width)     // there is a delay
            {
                future_cycle = get_cycles().get() + delay;
                get_cycles().set_break(future_cycle, this);
                delay_source1 = true;
            }
        }

        m_PinModule[0]->updatePinModule();
        m_PinModule[1]->updatePinModule();
        break;

    case 1:	// Full bidge Forward
        Dprintf(("full-bridge %s, forward\n", name().c_str()));

         m_source[0]->setState(active_high[0] ? '1' : '0');
        if (m_PinModule[0])
        {
	    if (!source_active[0])
	    {
                m_PinModule[0]->setSource(m_source[0]);
                source_active[0] = true;
	    }
            // P1A High (if active high)
            m_PinModule[0]->updatePinModule();
        }

        if (m_PinModule[1])
        {
            m_PinModule[1]->setSource(m_source[1]);
            source_active[1] = true;
            // P1B, P1C low (if active high)
            m_source[1]->setState(active_high[1] ? '0' : '1');
            m_PinModule[1]->updatePinModule();
        }

        if (m_PinModule[2])
        {
            m_PinModule[2]->setSource(m_source[2]);
            source_active[2] = true;
            // P1B, P1C low (if active high)
            m_source[2]->setState(active_high[2] ? '0' : '1');
            m_PinModule[2]->updatePinModule();
        }

        if (m_PinModule[3])
        {
            m_PinModule[3]->setSource(m_source[3]);
            source_active[3] = true;

            // P1D toggles
            RRprint((stderr, "full-bridge level=%d pwm_duty_cycle()=%d active_high[3]=%d\n", level, pwm_duty_cycle(), active_high[3]));
	    // level == 1 (tmr2 = pr2+1) and there is a duty cycle,
	    // set output as per active_high. If duty cycle = 0
	    // leave output at !active_high
            if (level && pwm_duty_cycle())
            {
                m_source[3]->setState(active_high[3] ? '1' : '0');

            }
            else
            {
                m_source[3]->setState(active_high[3] ? '0' : '1');
            }

            m_PinModule[3]->updatePinModule();
        }

        break;

    case 3:	// Full bridge reverse
        Dprintf(("full-bridge reverse %s\n", name().c_str()));

        if (m_PinModule[0])
        {
            m_PinModule[0]->setSource(m_source[0]);
            source_active[0] = true;
            // P1A, P1D low (if active high)
            m_source[0]->setState(active_high[0] ? '0' : '1');
            m_PinModule[0]->updatePinModule();
        }

        if (m_PinModule[1])
        {
            m_PinModule[1]->setSource(m_source[1]);
            source_active[1] = true;

            // P1B toggles
            if (level && pwm_duty_cycle())
            {
                m_source[1]->setState(active_high[1] ? '1' : '0');

            }
            else
            {
                m_source[1]->setState(active_high[1] ? '0' : '1');
            }

            m_PinModule[1]->updatePinModule();
        }

        if (m_PinModule[2])
        {
            m_PinModule[2]->setSource(m_source[2]);
            source_active[2] = true;
            // P1C High (if active high)
            m_source[2]->setState(active_high[2] ? '1' : '0');
            m_PinModule[2]->updatePinModule();
        }

        if (m_PinModule[3])
        {
            m_PinModule[3]->setSource(m_source[3]);
            source_active[3] = true;
            // P1A, P1D low (if active high)
            m_source[3]->setState(active_high[3] ? '0' : '1');
            m_PinModule[3]->updatePinModule();
        }

        break;

    default:
        printf("%s::pwm_match impossible ECCP bridge mode\n", name().c_str());
        break;
    }
}


//
// Set PWM bridge into shutdown mode
//
void CCPCON::shutdown_bridge(int eccpas)
{
    bridge_shutdown = true;
    Dprintf(("eccpas=0x%x\n", eccpas));
    RRprint((stderr, "CCPCON::shutdown_bridge eccpas=0x%x bd=%d ac=%d\n",
	eccpas,
	eccpas & (ECCPAS::PSSBD0 | ECCPAS::PSSBD1),
	(eccpas & (ECCPAS::PSSAC0 | ECCPAS::PSSAC1)) >> 2));
    RRprint((stderr, "CCPCON::shutdown_bridge eccpas=0x%x Maskbd=%x Maskac=%x\n", eccpas, (ECCPAS::PSSBD0 | ECCPAS::PSSBD1), (ECCPAS::PSSAC0 | ECCPAS::PSSAC1)));

    switch (eccpas & (ECCPAS::PSSBD0 | ECCPAS::PSSBD1))
    {
    case 0:	// B D output 0
        if (m_source[1])
        {
            m_source[1]->setState('0');
        }

        if (m_source[3])
        {
            m_source[3]->setState('0');
        }

        break;

    case 1:	// B, D output 1
	RRprint((stderr, "m_source[1]=%p %d\n", m_source[1], source_active[1]));
	RRprint((stderr, "m_source[3]=%p %d\n", m_source[3], source_active[3]));
        if (m_source[1])
        {
            m_source[1]->setState('1');
        }

        if (m_source[3])
        {
	    if (!source_active[3])
	    {
		RRprint((stderr, "p3=%s\n", m_PinModule[3]->getPin()->name().c_str()));
                m_PinModule[3]->setSource(m_source[3]);
                source_active[3] = true;
	    }
            m_source[3]->setState('1');
        }

        break;

    default:	// Tristate B & D
        if (m_PinModule[1])
        {
            m_PinModule[1]->setControl(m_tristate);
        }

        if (m_PinModule[3])
        {
            m_PinModule[3]->setControl(m_tristate);
        }

        break;
    }

    switch ((eccpas & (ECCPAS::PSSAC0 | ECCPAS::PSSAC1)) >> 2)
    {
    case 0:	// A, C output 0
        m_source[0]->setState('0');

        if (m_source[2])
        {
            m_source[2]->setState('0');
        }

        break;

    case 1:	// A, C output 1
        m_source[0]->setState('1');

        if (m_source[2])
        {
	    if (!source_active[2])
	    {
		RRprint((stderr, "p2=%s\n", m_PinModule[2]->getPin()->name().c_str()));
                m_PinModule[2]->setSource(m_source[2]);
                source_active[2] = true;
	    }
            m_source[2]->setState('1');
        }

        break;

    default:	// Tristate A & C
        m_PinModule[0]->setControl(m_tristate);

        if (m_PinModule[2])
        {
            m_PinModule[2]->setControl(m_tristate);
        }

        break;
    }

    m_PinModule[0]->updatePinModule();

    if (m_PinModule[1])
    {
        m_PinModule[1]->updatePinModule();
    }

    if (m_PinModule[2])
    {
        m_PinModule[2]->updatePinModule();
    }

    if (m_PinModule[3])
    {
        m_PinModule[3]->updatePinModule();
    }
}


void CCPCON::put(unsigned int new_value)
{
    unsigned int old_value = value.get();
    new_value &= mValidBits;
    Dprintf(("%s::put() new_value=0x%x\n", name().c_str(), new_value));
    trace.raw(write_trace.get() | value.get());
    value.put(new_value);

    unsigned int mode = new_value & MODE_MASK;

    if (!ccprl || !tmr2)
    {
        return;
    }

    // Return if no change other than possibly the duty cycle
    if (((new_value ^ old_value) & ~(CCPY | CCPX)) == 0)
    {
        return;
    }

    switch (mode)
    {
    case ALL_OFF0:
    case ALL_OFF1:
    case ALL_OFF2:
    case ALL_OFF3:
        if ((old_value & PWM_MASK) == PWM_MASK)
        {
	    if (ccprl)
	        ccprl->stop_pwm_mode();
	    stop_pwm();
        }
        if (ccprl)
        {
            ccprl->stop_compare_mode();
        }
	config_output(0, false, false);
        break;

    case CAP_FALLING_EDGE:
    case CAP_RISING_EDGE:
    case CAP_RISING_EDGE4:
    case CAP_RISING_EDGE16:
	capture_start(mode, old_value);
        break;

    case COM_SET_OUT:
    case COM_CLEAR_OUT:
    case COM_INTERRUPT:
    case COM_TRIGGER:
	compare_start(mode, old_value);
        break;

    case PWM0:
    case PWM1:
    case PWM2:
    case PWM3:
        ccprl->stop_compare_mode();
        /* do this when TMR2 == PR2
              ccprl->start_pwm_mode();
              tmr2->pwm_dc( pwm_duty_cycle(), address);
        */
        tmr2->pwm_dc( pwm_duty_cycle(), address);
	config_output(0, true, false);
        m_cOutputState = '0';

        if ((old_value & P1M0) && (new_value & P1M0))   // old and new full-bridge
        {
            // need to adjust timer if P1M1 also changed
            Dprintf(("full bridge repeat old=0x%x new=%x\n", old_value, new_value));

        }
        else
        {
 	    RRprint((stderr, "CCPCON::put %s call update\n", name().c_str()));
 	    tmr2->update();
        }

        pwm_match(2);
        return;
        break;
    }

}


bool CCPCON::test_compare_mode()
{
    switch (value.get() & MODE_MASK)
    {
    case COM_SET_OUT:
    case COM_CLEAR_OUT:
    case COM_INTERRUPT:
    case COM_TRIGGER:
        return true;
        break;

    default:
	return false;
	break;
    }

    return false;
}

void CCPCON::config_output(unsigned int i, bool newOut, bool newIn)
{
    // Output pin has changed state
    RRprint((stderr, "CCPCON::config_output %s i=%d newOut=%d m_bOutputEnabled=%d %p source=%p\n",name().c_str(), i, newOut, m_bOutputEnabled, m_PinModule[i], m_source[i]));
    if ((newOut != m_bOutputEnabled) && m_PinModule[i])
    {
        if (newOut)
        {
	    std::string pin_name = name().substr(0, 4);
	    m_PinModule[i]->getPin()->newGUIname(pin_name.c_str());
            m_PinModule[i]->setSource(m_source[i]);
            source_active[i] = true;
	    m_PinModule[i]->updatePinModule();
        }
        else
        {
	    if ((i != input_pin()) || !newIn)
	        m_PinModule[i]->getPin()->newGUIname("");
            m_PinModule[i]->setSource(0);
            m_source[i]->setState('?');
            source_active[i] = false;
	    m_PinModule[i]->updatePinModule();
        }
	m_bOutputEnabled = newOut;
    }
    if ((newIn != m_bInputEnabled) && m_PinModule[input_pin()])
    {
        in_pin_active(newIn);
	m_PinModule[input_pin()]->updatePinModule();
    }
}
void CCPCON::in_pin_active(bool on_off)
{
    if (!m_PinModule[input_pin()])
	return;

    RRprint((stderr, "CCPCON::in_pin_active %s on_off=%d m_bInputEnabled=%d\n", name().c_str(), on_off, m_bInputEnabled));

    if (on_off && !m_bInputEnabled)
    {
        std::string pin_name = name().substr(0, 4);
	pin_name += "in";
	RRprint((stderr, "CCPCON::in_pin_active pin_name=%s\n", pin_name.c_str()));
	m_PinModule[input_pin()]->getPin()->newGUIname(pin_name.c_str());
	m_PinModule[input_pin()]->addSink(m_sink);
	m_bInputEnabled = true;
    }
    else if (!on_off && m_bInputEnabled)
    {
	if (input_pin() != 0 || !m_bOutputEnabled)
	    m_PinModule[input_pin()]->getPin()->newGUIname("");

         m_PinModule[input_pin()]->removeSink(m_sink);
	 m_bInputEnabled = false;
    }
}
void CCPCON::compare_start(unsigned int mode, unsigned int old_value)
{
    RRprint((stderr, "CCPCON::compare_start %s mode=0x%x\n", name().c_str(), mode));
    if ((old_value & PWM_MASK) == PWM_MASK)
    {
	ccprl->stop_pwm_mode();
	stop_pwm();
    }
    ccprl->start_compare_mode(this);
    if (mode == COM_INTERRUPT)
        config_output(0, false, false);
    else
        config_output(0, true, false);
}

void CCPCON::capture_start(unsigned int mode, unsigned int old_value)
{
    RRprint((stderr, "CCPCON::capture_start %s mode=0x%x \n", name().c_str(), mode));
    if ((old_value & PWM_MASK) == PWM_MASK)
    {
	ccprl->stop_pwm_mode();
	stop_pwm();
    }
    config_output(0, true, true);
    ccprl->stop_compare_mode();
    switch(mode)
    {
    case CAP_RISE_OR_FALL:
    case CAP_FALLING_EDGE:
    case CAP_RISING_EDGE:
        edges = 1;
        break;

    case CAP_RISING_EDGE4:
        edges = 4;
        break;

    case CAP_RISING_EDGE16:
	edges = 16;
        break;
    };
}
DATA_SERVER *CCPCON::get_ccp_server()
{
    if (ccp_output_server == nullptr)
	ccp_output_server = new DATA_SERVER(DATA_SERVER::CCP);
    return ccp_output_server;
}


// CCPCON has FMT bit related to duty cycle
void CCPCON_FMT::put(unsigned int new_value)
{
    unsigned int old_value = value.get();
    new_value = (new_value & mValidBits) | (old_value & ~mValidBits);
    unsigned int diff = (new_value ^ old_value) & mValidBits;
    // register unchanged, return
    if (!diff)
	return;
    trace.raw(write_trace.get() | value.get());
    value.put(new_value);
    // ONLY FMT change, nothing to do, return
    if (!(diff & ~FMT))
	return;

    unsigned int mode = (new_value & MODE_MASK);

    RRprint((stderr, "CCPCON_FMT::put new_value = 0x%x enable=%d\n", new_value, ((new_value & EN) && mode)));
    // module is enabled
    if ((new_value & EN) && mode)
    {
	switch(new_value & PWM_MASK)
	{
	case 0xc:
	    ccp_pwm();
	    break;

	case 0x8:
	    compare_start(mode, old_value);
	    break;

	case 0x4:
	    capture_start(mode, old_value);
	    break;

	case 0x0:
	    if (mode == 3)
		capture_start(mode, old_value);
	    else if (mode > 0)
	    {
		compare_start(mode, old_value);
	    }
	    break;


	}
	if ((new_value & PWM_MASK) == PWM_MASK)
	{
	}
    }
    else	// module is disabled
    {

    RRprint((stderr, "CCPCON_FMT::put in disabled new_value = 0x%x enable=%d\n", new_value, ((new_value & EN) && mode)));
   	ccprl->stop_compare_mode();
        stop_pwm();
	config_output(0, false, false);
	value.put(value.get() & ~CCP_OUT);
    }

    // ??? what happend if module changed while ccp is on?

}

void CCPCON_FMT::ccp_pwm()
{
    ccprl->stop_compare_mode();
      /* do this when TMR2 == PR2
        ccprl->start_pwm_mode();
        tmr2->pwm_dc( pwm_duty_cycle(), address);
      */
    RRprint((stderr, "\tCCPCON_FMT::ccp_pwm duty_cycle=%d address=0x%x\n", pwm_duty_cycle(), address));
    tmr2->pwm_dc( pwm_duty_cycle(), address);
    m_cOutputState = '0';
    config_output(0, true, false);
    pwm_match(1);
}

void CCPCON_FMT::new_edge(unsigned int level)
{
    unsigned int mode = value.get() & MODE_MASK;
    RRprint((stderr, "CCPCON_FMT::new_edge %s state=%d mode=%d\n", name().c_str(), level, mode));

    if (!(value.get() & EN))
	return;			// do nothing if ccp not enbled

    if (!ccprl)
    {
	fprintf(stderr, "%s ccprl not defined\n", name().c_str());
	assert(ccprl);
    }

    switch(mode)
    {
    case CAP_FALLING_EDGE:
	if (!level)
	    capture_output();
	break;

    case CAP_RISE_OR_FALL:	// triggers on either edge
	capture_output();
	break;

    case CAP_RISING_EDGE:
    case CAP_RISING_EDGE4:
    case CAP_RISING_EDGE16:
	RRprint((stderr, "CCPCON_FMT::new_edge level=%d edge_cnt=%d egdes=%d\n", level, edge_cnt, edges));
        if (level && ++edge_cnt >= edges)
	{
	    capture_output();
	    edge_cnt = 0;
        }
    default:
	break;
    }
}

void CCPCON_FMT::capture_output()
{
    ccp_out(true, true);
    ccprl->capture_tmr();
    future_cycle = get_cycles().get() + 1;
    get_cycles().set_break(future_cycle, this);
    pulse_clear = true;
}

void CCPCON_FMT::ccp_out(bool state, bool interrupt)
{
        m_cOutputState = state?'1':'0';
        if (state)
	    value.put(value.get() | CCP_OUT);
	else
	    value.put(value.get() & ~CCP_OUT);

	// May not be defined by PPS
	if (m_PinModule[0])
	{
            m_source[0]->setState(state?'1':'0');
            m_PinModule[0]->updatePinModule();
	}

	RRprint((stderr, "CCPCON_FMT::ccp_out state=%d interrupt=%d m_Interrupt=%p pir=%p\n", state, interrupt, m_Interrupt, pir));
	if (interrupt)
	{
            if (m_Interrupt)
                m_Interrupt->Trigger();
            else if (pir)
                pir->set_mask(pir_mask);
	}
}

void CCPCON_FMT::compare_match()
{
    unsigned int mode = value.get() & MODE_MASK;
    RRprint((stderr, "CCPCON_FMT::compare_match() %s mode=0x%x\n", name().c_str(), mode));

    switch(mode)
    {
    case COM_SET_OUT:
	ccp_out(true, true);
        Dprintf(("-- CCPCON setting compare output to 1\n"));
        break;

    case COM_CLEAR_OUT:
	ccp_out(false, true);
        Dprintf(("-- CCPCON setting compare output to 0\n"));
        break;

    case COM_TOG:
	ccp_out(!(value.get() & CCP_OUT), true);
	break;

    case COM_TOG_CLR:
        if (ccprl)
            ccprl->tmrl->clear_timer();
	ccp_out(!(value.get() & CCP_OUT), true);
	break;

    case COM_PULSE_CLR:
        if (ccprl)
            ccprl->tmrl->clear_timer();
    case COM_PULSE:
	ccp_out(true, true);
	RRprint((stderr, "CCPCON_FMT::compare_match COM_PULSE future_cycle=%ld\n", future_cycle));
        future_cycle = get_cycles().get() + 1;
	get_cycles().set_break(future_cycle, this);
	pulse_clear = true;
	break;

    default:
	break;
    }
}
void CCPCON_FMT::compare_start(unsigned int mode, unsigned int old_value)
{
    RRprint((stderr, "CCPCON_FMT::compare_start %s mode=0x%x\n", name().c_str(), mode));
    if ((old_value & PWM_MASK) == PWM_MASK)
    {
	ccprl->stop_pwm_mode();
	stop_pwm();
    }
    ccprl->start_compare_mode(this);
    config_output(0, true, false);
    // If Pulse, zero output
    if (mode == COM_PULSE || mode == COM_PULSE_CLR)
    {
	ccp_out(false, false);
    }
}

// Combine ccprh and ccprl to get duty cycle
unsigned int CCPCON_FMT::pwm_duty_cycle()
{

    unsigned int ret;

    if (value.get() & FMT)
	ret = (ccprl->ccprh->value.get() << 2) + ((ccprl->value.get() >> 6) & 3);
    else
	ret =  ((ccprl->ccprh->value.get() & 3) << 8) + ccprl->value.get();

    RRprint((stderr, "CCPCON_FMT::pwm_duty_cycle() %s FMT=%d ret=%d \n", name().c_str(), (bool)(value.get() & FMT), ret));
    return ret;
}

void CCPCON_FMT::simple_pwm_output(int level)
{
   m_cOutputState = level ? '1' : '0';
   RRprint((stderr, "CCPCON_FMT::simple_pwm_output m_source[0]=%p m_PinModule[0]=%p\n",m_source[0], m_PinModule[0]));
   if (m_PinModule[0])
   {
       m_source[0]->setState(level ? '1' : '0');
       m_PinModule[0]->setSource(m_source[0]);
       source_active[0] = true;
       m_PinModule[0]->updatePinModule();
   }

   if (level)
	value.put(value.get() | CCP_OUT);
   else
	value.put(value.get() & ~CCP_OUT);
   RRprint((stderr, "CCPCON_FMT::simple_pwm_output %s pin=%p level=%d m_Interrupt=%p\n", name().c_str(), m_PinModule[0], level, m_Interrupt));
   if (m_Interrupt && level)
	m_Interrupt->Trigger();
   if (ccp_output_server)
	ccp_output_server->send_data(level, 0);
}

void CCPCON_FMT::new_capture_src(unsigned int new_value)
{
    RRprint((stderr, "CCPCON_FMT::new_capture_src %d old=%d\n", new_value, capture_input));
    capture_input = new_value;
}


class CCP_CLC_RECEIVER : public DATA_RECEIVER
{
public:
    explicit CCP_CLC_RECEIVER(CCPxCAP *_pt_ccp, const char *_name) :
        DATA_RECEIVER(_name), pt_ccp(_pt_ccp)
    {}
    virtual ~CCP_CLC_RECEIVER(){}
    void rcv_data(int v1, int v2) override;

private:
    CCPxCAP *pt_ccp;

};

void CCP_CLC_RECEIVER::rcv_data(int v1, int v2)
{
    pt_ccp->clc_data_ccp(v1, v2 & ~DATA_SERVER::SERV_MASK);
}

CCPxCAP::CCPxCAP(Processor *pCpu, const char *pName, const char *pDesc, CCPCON_FMT *_ccp_fmt)
    : sfr_register(pCpu, pName, pDesc), ccp_fmt(_ccp_fmt)
{

    pt_clc_receiver = new CCP_CLC_RECEIVER(this, pName);
}

CCPxCAP::~CCPxCAP()
{
    delete pt_clc_receiver;
}


void CCPxCAP::put(unsigned int new_value)
{
    unsigned int old = value.get();
    new_value &= 0x07;
    if (!(new_value ^ old))
	return;
    RRprint((stderr, "%s CCPxCAP::put new=%d old=%d\n", name().c_str(), new_value, old));
    ccp_fmt->new_capture_src(new_value);
    trace.raw(write_trace.get() | old);
    value.put(new_value);
    switch(old)
    {
    case CCPxPin:
	ccp_fmt->in_pin_active(false);
	break;

    case C1_out:
	get_cm_data_server()->detach_data(pt_clc_receiver);
	break;

    case C2_out:
	get_cm_data_server()->detach_data(pt_clc_receiver);
	break;

    case IOC_int:
	break;

    case LC1_out:
	get_clc_data_server(0)->detach_data(pt_clc_receiver);
	break;

    case LC2_out:
	get_clc_data_server(1)->detach_data(pt_clc_receiver);
	break;

    }
    switch(new_value)
    {
    case CCPxPin:
	ccp_fmt->in_pin_active(true);
	break;

    case C1_out:
	get_cm_data_server()->attach_data(pt_clc_receiver);
	break;

    case C2_out:
	get_cm_data_server()->attach_data(pt_clc_receiver);
	break;

    case IOC_int:
	break;

    case LC1_out:
	RRprint((stderr, "\tpt_clc_receiver=%p server=%p\n", pt_clc_receiver, get_clc_data_server(0)));
        get_clc_data_server(0)->attach_data(pt_clc_receiver);

	break;

    case LC2_out:
        get_clc_data_server(1)->attach_data(pt_clc_receiver);
	break;
    };
}
DATA_SERVER * CCPxCAP::get_clc_data_server(unsigned int n_clc)
{
    CLC_BASE *pt_clc = ccp_fmt->m_clc[n_clc];
    if (!pt_clc)
    {
        fprintf(stderr, "***ERROR CCPxCAP:get_cm_data_server m_clc[%u] not defined\n", n_clc);
        assert(pt_clc);
    }
    return pt_clc->get_CLC_data_server();
}

DATA_SERVER * CCPxCAP::get_cm_data_server()
{
    if (!pt_cm)
    {
        fprintf(stderr, "***ERROR CCPxCAP:get_cm_data_server pt_cm not defined\n");
        assert(pt_cm);
    }

    return pt_cm->get_CM_data_server();
}
void CCPxCAP::clc_data_ccp(bool state, unsigned int n_clc)
{
    RRprint((stderr, "CCPxCAP::clc_data_ccp state=%d n_clc=%d\n", state, n_clc));
    if (value.get() == LC1_out && n_clc == 0)
	ccp_fmt->new_edge(state);
    if (value.get() == LC2_out && n_clc == 1)
	ccp_fmt->new_edge(state);
}

PWMxCON::PWMxCON(Processor *pCpu, const char *pName, const char *pDesc, char _index)
    : CCPCON(pCpu, pName, pDesc), index(_index)
{
    mValidBits = 0xd0;

    std::fill_n(m_clc, 4, nullptr);
}
PWMxCON::~PWMxCON()
{
    if (pwmx_output_server)
	delete pwmx_output_server;
}


void PWMxCON::put(unsigned int new_value)
{
    new_value &= mValidBits;
    put_value(new_value);
}


void PWMxCON::put_value(unsigned int new_value)
{
    unsigned int diff = value.get() ^ new_value;
    Dprintf(("PWMxCON::put %s new 0x%x diff 0x%x\n", name().c_str(), new_value, diff));

    if (!diff)
    {
        return;
    }

    trace.raw(write_trace.get() | value.get());
    value.put(new_value);

    if (diff & PWMxEN)
    {
        if (new_value & PWMxEN)   // Turn on PWM
        {
	    RRprint((stderr, "PWMxEN %s %s address=0x%x duty_cycle=%d\n", name().c_str(), tmr2->name().c_str(), address, pwm_duty_cycle()));
	    tmr2->pwm_dc(pwm_duty_cycle(), address);
	    config_output(0, true, false);
            pwm_match(0);

        }
        else  		// Turn off PWM
        {
	    config_output(0, false, false);
            tmr2->stop_pwm(address);
        }
    }
}


/*
 * level == 0 duty cycle match
 * level == 1 tmr2 == PR2
 * level == 2
 */
void PWMxCON::pwm_match(int level)
{
    unsigned int reg = value.get();

    if (!(reg & PWMxEN))
    {
        return;
    }

    Dprintf(("%s::pwm_match() level=%d now=%" PRINTF_GINT64_MODIFIER "d\n", name().c_str(), level, get_cycles().get()));
//    printf("%s PWMxCON::pwm_match() level=%d now=%" PRINTF_GINT64_MODIFIER "d %s=0x%x\n", name().c_str(), level, get_cycles().get(), tmr2->name().c_str(), tmr2->value.get());

    if (level == 1)
    {
	// update pwm_mode and duty_cycle
        tmr2->pwm_dc(pwm_duty_cycle(), address);

        if (!pwm_duty_cycle())   // if duty cycle == 0 output stays low
        {
            level = 0;
        }
    }

    if (reg & PWMxPOL)  	// inverse output
    {
        level = level ? 0 : 1;
        Dprintf(("%s::pwm_match() invert output to %d\n", name().c_str(), level));
    }

    if (level)
    {
        reg |= PWMxOUT;

    }
    else
    {
        reg &= ~PWMxOUT;
    }

    Dprintf(("%s::pwm_match() reg 0x%x old 0x%x\n", name().c_str(), reg, value.get()));

    if (reg != value.get())
    {
        put_value(reg);
    }

    // send output to Comp output Generator
    if (m_cog)
        m_cog->out_pwm(level, index);
    // send output to Comp wave Gen
    if (m_cwg)
    {
        m_cwg->out_pwm(level, index);
    }

    // sent output to Config Logic Cell
    for (int i = 0; i < 4; i++)
    {
        if (m_clc[i])
        {
            m_clc[i]->out_pwm(level, index);
        }
    }

    m_source[0]->setState(level ? '1' : '0');
    if (outPinEnabled())
    {
        m_cOutputState = level ? '1' : '0';
        m_PinModule[0]->setSource(m_source[0]);
        m_PinModule[0]->updatePinModule();
        source_active[0] = true;
        Dprintf(("PWMxOE level %c\n", m_cOutputState));
    }
}

DATA_SERVER *PWMxCON::get_pwmx_server()
{
    if (pwmx_output_server == nullptr)
	pwmx_output_server = new DATA_SERVER(DATA_SERVER::PWM);
    return pwmx_output_server;
}

TRISCCP::TRISCCP(Processor *pCpu, const char *pName, const char * /* pDesc */ )
    : sfr_register(pCpu, pName), first(true)
{
}


void TRISCCP::put(unsigned int new_value)
{
    if (first)
    {
        first = false;
        std::cout << name() << " not implemented, if required, file feature request\n";
    }

    trace.raw(write_trace.get() | value.get());
    value.put(new_value);
}


DATACCP::DATACCP(Processor *pCpu, const char *pName, const char * /* pDesc */ )
    : sfr_register(pCpu, pName), first(true)
{
}


void DATACCP::put(unsigned int new_value)
{
    if (first)
    {
        first = false;
        std::cout << name() << " not implemented, if required, file feature request\n";
    }

    trace.raw(write_trace.get() | value.get());
    value.put(new_value);
}


class TMR1_Interface : public Interface
{
public:
    explicit TMR1_Interface(TMRL *_tmr1)
        : Interface((void **)_tmr1), tmr1(_tmr1)
    {
    }
    void SimulationHasStopped(void * /* object */ ) override
    {
        tmr1->current_value();
    }
    void Update(void *object) override
    {
        SimulationHasStopped(object);
    }

private:
    TMRL *tmr1;
};


// Attribute for frequency of external Timer1 oscillator
class TMR1_Freq_Attribute : public Float
{
public:
    TMR1_Freq_Attribute(Processor * _cpu, double freq, const char *name = "tmr1_freq");

    void set(double d) override;
    double get_freq();

private:
    Processor * cpu;
};


TMR1_Freq_Attribute::TMR1_Freq_Attribute(Processor * _cpu, double freq, const char *name)
    : Float(name, freq, " Tmr oscillator frequency."),
      cpu(_cpu)
{
}


double TMR1_Freq_Attribute::get_freq()
{
    double d;
    Float::get_as(d);
    return d;
}


void TMR1_Freq_Attribute::set(double d)
{
    Float::set(d);
}


//--------------------------------------------------
// T1CON
//--------------------------------------------------
T1CON::T1CON(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc), cpu(pCpu)
{
    char freq_name[] = "tmr1_freq";

    if (*(pName + 1) >= '1' && *(pName + 1) <= '9')
    {
        freq_name[3] = *(pName + 1);
    }

    cpu->addSymbol(freq_attribute = new TMR1_Freq_Attribute(pCpu, 32768.0, freq_name));
}


T1CON::~T1CON()
{
    cpu->removeSymbol(freq_attribute);
    delete freq_attribute;
}


void T1CON::put(unsigned int new_value)
{
    trace.raw(write_trace.get() | value.get());
    unsigned int diff = value.get() ^ new_value;
    value.put(new_value);

    if (!tmrl)
    {
        return;
    }

    // First, check the tmr1 clock source bit to see if we are  changing from
    // internal to external (or vice versa) clocks.
    if (diff & (TMR1CS | T1OSCEN))
    {
        tmrl->new_clock_source();
    }

    if (diff & TMR1ON)
    {
        tmrl->on_or_off(value.get() & TMR1ON);
    }
    else  if (diff & (T1CKPS0 | T1CKPS1 | TMR1GE | T1GINV))
    {
        tmrl->update();
    }
}


unsigned int T1CON::get()
{
    trace.raw(read_trace.get() | value.get());
    return value.get();
}


unsigned int T1CON::get_prescale()
{
    return (value.get() & (T1CKPS0 | T1CKPS1)) >> 4;
}


double T1CON::t1osc()
{
    return (value.get() & T1OSCEN) ? freq_attribute->get_freq() : 0.0;
}


//--------------------------------------------------
//
//--------------------------------------------------

//
//  Signal T1GCon on change of state of Gate pin
//
class T1GCon_GateSignalSink : public SignalSink
{
public:
    explicit T1GCon_GateSignalSink(T1GCON *_t1gcon)
        : m_t1gcon(_t1gcon)
    {
        assert(_t1gcon);
    }
    virtual ~T1GCon_GateSignalSink()
    {
    }

    void release() override
    {
        delete this;
    }
    void setSinkState(char new3State) override
    {
	Dprintf(("setSinkState %s new3State=%c\n", m_t1gcon->name().c_str(), new3State));
        m_t1gcon->PIN_gate(new3State == '1' || new3State == 'W');
    }

private:
    T1GCON *m_t1gcon;
};


T1GCON::T1GCON(Processor *pCpu, const char *pName, const char *pDesc, T1CON_G *_t1con_g)
    : sfr_register(pCpu, pName, pDesc), write_mask(0xfb), t1con_g(_t1con_g)
{
}


T1GCON::~T1GCON()
{
    if (m_Interrupt)
    {
        m_Interrupt->release();
    }
}


bool T1GCON::tmr1_isON()
{
    if (t1con_g)
    {
        return t1con_g->get_tmr1on();
    }

    if (tmrl->t1con)
    {
        return tmrl->t1con->get_tmr1on();
    }

    std::cerr << name() << " Error T1GCON::tmr1_isON get_tmr1on() not found\n";
    return false;
}


void T1GCON::put(unsigned int new_value)
{
    unsigned int old_value = value.get();
    new_value = (new_value & write_mask) | (old_value & ~write_mask);
    unsigned int diff = new_value ^ old_value;
    bool t1ggo = new_value & T1GGO;
    assert(m_Interrupt);
    assert(tmrl);

    RRprint((stderr, "T1GCON::put %s new_value=0x%x old_value=0x%x diff=0x%x\n", name().c_str(), new_value, old_value, diff));

    if (!diff)
    {
        return;
    }

    trace.raw(write_trace.get() | value.get());
    value.put(new_value);

    if (diff & (T1GSS1 | T1GSS0 | T1GPOL | TMR1GE))
    {
        switch (new_value & (T1GSS1 | T1GSS0))
        {
        case 0:
            new_gate(PIN_gate_state);
            break;

        case 1:
            new_gate(T0_gate_state);
            break;

        case 2:
            new_gate(CM1_gate_state);
            break;

        case 3:
            new_gate(CM2_gate_state);
            break;
        }

        // Dont't allow gate change to clear new T1GG0
        if ((diff & T1GGO) && t1ggo)
        {
            value.put(value.get() | T1GGO);
        }
    }

    // Change of single pulse mode
    if ( diff & T1GSPM )
    {
        wait_trigger = false;
    }

    // Single pulse mode "GO" enabled
    if ((diff & T1GGO) && ((value.get() & (T1GGO | T1GSPM))==(T1GGO | T1GSPM)))
    {
	wait_trigger = true;
        // Make sure T1GVAL is off but don't change t1g_in_val
        if (value.get() & T1GVAL)
        {
            value.put(value.get() & ~T1GVAL);
            tmrl->IO_gate(false);
        }
    }

    // Toggle mode change
    if (diff & T1GTM)
    {
        if ((value.get() & T1GTM))   // T1GTM going high, set t1g_in to 0
        {
            if (value.get() & T1GVAL)
            {
                value.put(value.get() & ~(T1GVAL));
                m_Interrupt->Trigger();
            }
            t1g_in_val = false;
            tmrl->IO_gate(false); // Counting should be stopped
        }
    }

    tmrl->update();
}


void T1GCON::setGatepin(PinModule *pin)
{
    if (pin != gate_pin)
    {
        if (sink)
        {
            gate_pin->removeSink(sink);

        }
        else
        {
            sink = new T1GCon_GateSignalSink(this);
        }

        gate_pin = pin;
        Dprintf(("T1GCON::setGatepin %s %s\n", name().c_str(), pin->getPin()->name().c_str()));
        pin->addSink(sink);
    }
}


// The following 4 functions are called on a state change.
// They pass the state to new_gate if that input is selected.
void T1GCON::PIN_gate(bool state)
{
    PIN_gate_state = state;

    if ((value.get() & (T1GSS0 | T1GSS1)) == 0)
    {
        new_gate(state);
    }
}


void T1GCON::T0_gate(bool state)
{
    T0_gate_state = state;

    if ((value.get() & (T1GSS0 | T1GSS1)) == 1)
    {
        new_gate(state);
    }
}


// T[246] = PR[246]
// overloads T0_gate_state
void T1GCON::T2_gate(bool state)
{
    T0_gate_state = state;

    if ((value.get() & (T1GSS0 | T1GSS1)) == 1)
    {
        new_gate(state);
    }
}


void T1GCON::CM1_gate(bool state)
{
    CM1_gate_state = state;

    if ((value.get() & (T1GSS0 | T1GSS1)) == 2)
    {
        new_gate(state);
    }
}


void T1GCON::CM2_gate(bool state)
{
    CM2_gate_state = state;

  RRprint((stderr, "T1GCON::CM2_gate state=%d\n", state));

    if ((value.get() & (T1GSS0 | T1GSS1)) == 3)
    {
        new_gate(state);
    }
}


void T1GCON::new_gate(bool state)
{
    Dprintf (( "T1GCON::new_gate(%d)\n", state ));
    bool t1g_in = (state ^ !get_t1GPOL());
    unsigned int reg_value = value.get();

    if (t1g_in == last_t1g_in)  // && (t1g_in == t1g_val))   // no state change, do nothing
	return;

    last_t1g_in = t1g_in;

    Dprintf (( "      -  in %c   TM=%d  SPM=%d  inval=%d   val=%d\n",
                t1g_in?'/':'\\', (reg_value&T1GTM)?1:0, (reg_value&T1GSPM)?1:0,
                t1g_in_val, (reg_value&T1GVAL) ));
    if ( reg_value & T1GTM )  // Toggle mode
    {
        if ( !tmr1_isON() )    // timer turned off
	{
            Dprintf (( "     - TMR off, quit\n" ));
            t1g_in_val = false;
            return;
	}
        else if ( t1g_in )    // valid rising edge
	{
            t1g_in_val = ! t1g_in_val;		// t1g_in_val changes state
	}
        else
        {
            Dprintf (( "     - falling edge, no toggle\n" ));
	    return;
        }
    }
    else  	// Gate directly in control
    {
        t1g_in_val = t1g_in;
    }

    if (reg_value & T1GSPM) 	// Single pulse mode
    {
        if (!(reg_value & T1GGO))  // do nothing if T1GGO clear
        {
            Dprintf (( "     - SPM not armed\n" ));
            return;
        }
        if ( wait_trigger )
        {
            // The first leading edge after the code sets GO/DONE should
            // start the timer but a trailing edge does nothing
            if ( t1g_in_val )
            {
                Dprintf (( "     - SPM armed\n" ));
	    wait_trigger = false;
	}
        }
        else if (!t1g_in_val)     // End of gate
	{
            Dprintf (( "     - SPM done\n" ));
            reg_value &= ~T1GGO;  //set done
        }
    }

    if (t1g_in_val)
    {
        reg_value |= T1GVAL;
    }
    else
    {
        if (reg_value & T1GVAL) 	// interrupt on T1GVAL negative edge
        {
            m_Interrupt->Trigger();
        }

        reg_value &= ~T1GVAL;
    }

    value.put(reg_value);
    tmrl->IO_gate(t1g_in_val);
}

void T1GCON::on_or_off(int new_state)
{
    // We don't (yet) care much whether the timer is being turned on or off
    (void)new_state;
    t1g_in_val = false;     // reset the flip-flop
}




//--------------------------------------------------
// T1CON_G
//--------------------------------------------------
T1CON_G::T1CON_G(Processor *pCpu, const char *pName, const char *pDesc)
//: sfr_register(pCpu, pName, pDesc),
    : T1CON(pCpu, pName, pDesc),
      t1gcon(pCpu, "t1gcon", "TM1 Gate Control Register", this)
{
}


T1CON_G::~T1CON_G()
{
}


void T1CON_G::put(unsigned int new_value)
{
    trace.raw(write_trace.get() | value.get());
    unsigned int diff = value.get() ^ new_value;
    value.put(new_value);

    if (!tmrl)
    {
        return;
    }

    // First, check the tmr1 clock source bit to see if we are  changing from
    // internal to external (or vice versa) clocks.
    if (diff & (TMR1CS0 | TMR1CS1 | T1OSCEN))
    {
        tmrl->new_clock_source();
    }

    if (diff & TMR1ON)
    {
        tmrl->on_or_off(value.get() & TMR1ON);
        t1gcon.on_or_off(value.get() & TMR1ON);
    }
    else  if (diff & (T1CKPS0 | T1CKPS1))
    {
        tmrl->update();
    }
}


// If Cap. sensing oscillator T1 clock source, pass to T1
void T1CON_G::t1_cap_increment()
{
    if (get_tmr1cs() == 3)  	// T1 input Cap. sensing oscillator
    {
        tmrl->increment();
    }
}


//--------------------------------------------------
// member functions for the TMRH base class
//--------------------------------------------------
TMRH::TMRH(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc)
{
    value.put(0);
}


void TMRH::put(unsigned int new_value)
{
    trace.raw(write_trace.get() | value.get());

    if (!tmrl)
    {
        value.put(new_value & 0xff);
        return;
    }

    tmrl->set_ext_scale();
    value.put(new_value & 0xff);
    tmrl->synchronized_cycle = get_cycles().get();
    tmrl->last_cycle = tmrl->synchronized_cycle
                       - (int64_t)((tmrl->value.get() + (value.get() << 8)
                                   * tmrl->prescale * tmrl->ext_scale) + 0.5);

    if (tmrl->t1con->get_tmr1on())
    {
        tmrl->update();
    }
}


unsigned int TMRH::get()
{
    trace.raw(read_trace.get() | value.get());
    return get_value();
}


// For the gui and CLI
unsigned int TMRH::get_value()
{
    // If the TMR1 is being read immediately after being written, then
    // it hasn't had enough time to synchronize with the PIC's clock.
    if (get_cycles().get() <= tmrl->synchronized_cycle)
    {
        return value.get();
    }

    // If the TMR is not running then return.
    if (!tmrl->t1con->get_tmr1on())
    {
        return value.get();
    }

    tmrl->current_value();
    return value.get();
}


//--------------------------------------------------
//
//--------------------------------------------------

class TMRl_GateSignalSink : public SignalSink
{
public:
    explicit TMRl_GateSignalSink(TMRL *_tmr1l)
        : m_tmr1l(_tmr1l)
    {
        assert(_tmr1l);
    }

    void release() override
    {
        delete this;
    }
    void setSinkState(char new3State) override
    {
        m_tmr1l->IO_gate(new3State == '1' || new3State == 'W');
    }

private:
    TMRL *m_tmr1l;
};


//--------------------------------------------------
// trivial class to represent a compare event reference
//--------------------------------------------------

class TMR1CapComRef
{
public:
    TMR1CapComRef(CCPCON * c, unsigned int v) : ccpcon(c), value(v) {}

    TMR1CapComRef * next = nullptr;

    CCPCON * ccpcon;
    unsigned int value;
};


//--------------------------------------------------
// member functions for the TMRL base class
//--------------------------------------------------
TMRL::TMRL(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc),
      m_cState('?'), m_compare_GateState(true),
      m_io_GateState(true), m_t1gss(true)
{
    value.put(0);
    prescale_counter = prescale = 1;
    break_value = 0x10000;
    ext_scale = 1.0;

    std::fill_n(m_clc, 4, nullptr);
}


TMRL::~TMRL()
{
    if (m_Interrupt)
    {
        m_Interrupt->release();
    }
    if (tmr135_overflow_server)
	delete tmr135_overflow_server;

    delete tmr1_interface;
}


/*
 * If we are similating an external RTC crystal for timer1,
 * compute scale factor between crsytal speed and processor
 * instruction cycle rate
 *
 * If tmr1cs = 1 Fosc is 4 x normal speed so reduce ticks by 1/4
 */
void TMRL::set_ext_scale()
{
    current_value();

    ext_scale = 1.0;

    if (t1con->get_t1oscen()  && (t1con->get_tmr1cs() == 2))   // external clock
    {
        ext_scale = get_cycles().instruction_cps() /
                    t1con->freq_attribute->get_freq();

    }
    else if (t1con->get_tmr1cs() == 1)     // Fosc
    {
        ext_scale = 0.25;

    }
    else if ((t1con->get_tmr1cs() == 3) && have_lfintosc)
    {
	ext_scale = get_cycles().instruction_cps() / 31000.;
    }

    if (future_cycle)
    {
        last_cycle = get_cycles().get()
                     - (int64_t)(value_16bit * (prescale * ext_scale) + 0.5);
    }
}


void TMRL::release()
{
}


void TMRL::setIOpin(PinModule *extClkSource, int /* arg */ )
{
    Dprintf(("%s::setIOpin %s\n", name().c_str(), extClkSource ? extClkSource->getPin()->name().c_str() : ""));

    if (extClkSource)
    {
        extClkSource->addSink(this);
    }
}


void TMRL::setSinkState(char new3State)
{
    if (new3State != m_cState)
    {
        m_cState = new3State;

        if (m_bExtClkEnabled && (m_cState == '1' || m_cState == 'W'))
        {
            increment();
        }
    }
}


void TMRL::set_compare_event(unsigned int value, CCPCON *host)
{
    TMR1CapComRef * event = compare_queue;

    if (host)
    {
        while (event)
        {
            if (event->ccpcon == host)
            {
                event->value = value;
                update();
                return;
            }

            event = event->next;
        }

        event = new TMR1CapComRef(host, value);
        event->next = compare_queue;
        compare_queue = event;
        update();

    }
    else
    {
        std::cout << name() << " TMRL::set_compare_event called with no CAPCOM\n";
    }
}


void TMRL::clear_compare_event(CCPCON *host)
{
    TMR1CapComRef * event = compare_queue;
    TMR1CapComRef * * eptr = &compare_queue;

    while (event)
    {
        if (event->ccpcon == host)
        {
            *eptr = event->next;
            delete event;
            update();
            return;
        }

        eptr = &event->next;
        event = event->next;
    }
}


void TMRL::setGatepin(PinModule *extGateSource)
{
    Dprintf(("TMRL::setGatepin\n"));

    if (extGateSource)
    {
        extGateSource->addSink(new TMRl_GateSignalSink(this));
    }
}


void TMRL::set_T1GSS(bool arg)
{
    m_t1gss = arg;

    if (m_t1gss)
    {
        IO_gate(m_io_GateState);

    }
    else
    {
        compare_gate(m_compare_GateState);
    }
}


void TMRL::compare_gate(bool state)
{
    m_compare_GateState = state;

    if (!m_t1gss && m_GateState != state)
    {
        m_GateState = state;
        Dprintf(("TMRL::compare_gate state %d tmr1GE=%d\n", state, t1con->get_tmr1GE()));

        if (t1con->get_tmr1GE())
        {
            update();
        }
    }
}


void TMRL::IO_gate(bool state)
{
    m_io_GateState = state;

    if (m_t1gss && (m_GateState != state))
    {
        m_GateState = state;
        Dprintf(("TMRL::IO_gate %s state %d 1GE=%d\n", name().c_str(), state, t1con->get_tmr1GE()));

        if (t1con->get_tmr1GE())
        {
            update();
        }
    }
}


//------------------------------------------------------------
// setInterruptSource()
//
// This Timer can be an interrupt source. When the interrupt
// needs to be generated, then the InterruptSource object will
// direct the interrupt to where it needs to go (usually this
// is the Peripheral Interrupt Register).

void TMRL::setInterruptSource(InterruptSource *_int)
{
    m_Interrupt = _int;
}


void TMRL::increment()
{
    Dprintf(("TMRL increment because of external clock\n"));

    if (--prescale_counter == 0)
    {
        prescale_counter = prescale;

        // In synchronous counter mode prescaler works but rest of tmr1 does not
        if (t1con->get_t1sync() == 0 && m_sleeping)
        {
            return;
        }

        // prescaler works but rest of timer turned off
        if (!t1con->get_tmr1on())
        {
            return;
        }

        // If TMRH/TMRL have been manually changed, we'll want to
        // get the up-to-date values;
        trace.raw(write_trace.get() | value.get());
        current_value();
        value_16bit = 0xffff & (value_16bit + 1);
        tmrh->value.put((value_16bit >> 8) & 0xff);
        value.put(value_16bit & 0xff);

        if (value_16bit == 0 && m_Interrupt)
        {
            if (verbose & 4)
            {
                std::cout << name() << " TMRL:increment interrupt now=" << std::dec << get_cycles().get() << " value_16bit "  << value_16bit << '\n';
            }

            m_Interrupt->Trigger();

	    if (tmr135_overflow_server)
	    {
	        tmr135_overflow_server->send_data(true, tmr_number);
	    }

        }
    }
}

DATA_SERVER *TMRL::get_tmr135_server()
{
    if (tmr135_overflow_server == nullptr)
    {
        tmr135_overflow_server = new DATA_SERVER(DATA_SERVER::TMR1);
	tmr_number = name().c_str()[3] - '0';
    }
    return tmr135_overflow_server;
}


void TMRL::on_or_off(int new_state)
{
    RRprint((stderr, "*** %s::on_or_off on=%d now=%ld\n", name().c_str(), new_state, get_cycles().get()));
    if (new_state)
    {
        Dprintf(("%s is being turned on\n", name().c_str()));
        // turn on the timer
        // Effective last cycle
        // Compute the "effective last cycle", i.e. the cycle
        // at which TMR1 was last 0 had it always been counting.
        last_cycle = (int64_t)(get_cycles().get() -
                              (value.get() + (tmrh->value.get() << 8)) * prescale * ext_scale + 0.5);
        update();
    }
    else
    {
        Dprintf(("%s is being turned off\n", name().c_str()));
        // turn off the timer and save the current value
        current_value();

        if (future_cycle)
        {
            get_cycles().clear_break(this);
            future_cycle = 0;
        }
    }
}


//
// If anything has changed to affect when the next TMR1 break point
// will occur, this routine will make sure the break point is moved
// correctly.
//

void TMRL::update()
{
    Dprintf(("TMR1 %s update now=0x%" PRINTF_GINT64_MODIFIER "x\n", name().c_str(), get_cycles().get()));
    // if t1con->get_t1GINV() is false, timer can run if m_GateState == 0
    bool gate = t1con->get_t1GINV() ? m_GateState : !m_GateState;
    Dprintf(("TMRL::update gate %s %d GateState %d inv %d get_tmr1on %x tmr1GE %x tmr1cs %x t1oscen %x\n", name().c_str(), gate, m_GateState, t1con->get_t1GINV(), t1con->get_tmr1on(), t1con->get_tmr1GE(), t1con->get_tmr1cs(), t1con->get_t1oscen()));
    RRprint((stderr, "*** %s::update now=%ld gate  %d GateState %d inv %d get_tmr1on %x tmr1GE %x tmr1cs %x t1oscen %x\n", name().c_str(), get_cycles().get(), gate, m_GateState, t1con->get_t1GINV(), t1con->get_tmr1on(), t1con->get_tmr1GE(), t1con->get_tmr1cs(), t1con->get_t1oscen()));

    /* When tmr1 is on, and t1con->get_tmr1GE() is true,
       gate == 1 allows timer to run, gate == 0 stops timer.
       However, if t1con->get_tmr1GE() is false gate has no
       effect on timer running or not.
    */
    if (t1con->get_tmr1on() && (t1con->get_tmr1GE() ? gate : true))
    {
        switch (t1con->get_tmr1cs())
        {
        case 0:	// internal clock Fosc/4
            if (verbose & 0x4)
            {
                std::cout << name() << " Tmr1 Internal clock\n";
            }

            break;

        case 1:	// internal clock Fosc
            break;

        case 2:	// External clock
            if (t1con->get_t1oscen())  	// External clock enabled
            {
                /*
                 external timer1 clock runs off a crystal which is typically
                 32768 Hz and is independant on the instruction clock, but
                 gpsim runs on the instruction clock. Ext_scale is the ratio
                 of these two clocks so the breakpoint can be adjusted to be
                 triggered at the correct time.
                */
                if (verbose & 0x4)
                {
                    std::cout << name() << " Tmr1 External clock\n";
                }

            }
            else  		// External stimuli(pin)
            {
                prescale = 1 << t1con->get_prescale();
                prescale_counter = prescale;
                set_ext_scale();
                return;
            }

            break;

        case 3:			// Cap. sensing oscillator
	    if (!have_lfintosc)
	    {
            	prescale = 1 << t1con->get_prescale();
            	prescale_counter = prescale;
            	set_ext_scale();
            	return;
	    }
            break;

        default:
            std::cout << name() << " TMR1SC reserved value " << t1con->get_tmr1cs() << '\n';
            break;
        }

        set_ext_scale();
        // Note, unlike TMR0, anytime something is written to TMRL, the
        // prescaler is unaffected on the P18 processors. However, it is
        // reset on the p16f88 processor, which is how the current code
        // works. This only effects the external drive mode.
        prescale = 1 << t1con->get_prescale();
        prescale_counter = prescale;

        if (verbose & 0x4)
        {
            std::cout << name() << " TMRL: Current prescale " << prescale << ", ext scale " << ext_scale << '\n';
        }

        //  synchronized_cycle = cycles.get() + 2;
        synchronized_cycle = get_cycles().get();
        last_cycle = synchronized_cycle
                     - (int64_t)(value_16bit * (prescale * ext_scale) + 0.5);
        break_value = 0x10000;  // Assume that a rollover will happen first.

        for (TMR1CapComRef * event = compare_queue; event; event = event->next)
        {
            if (verbose & 0x4)
            {
                std::cout << name() << " compare mode on " << event->ccpcon << ", value = " << event->value << '\n';
            }

            if (event->value > value_16bit && event->value < break_value)
            {
                // A compare interrupt is going to happen before the timer
                // will rollover.
                break_value = event->value;
            }
        }

        if (verbose & 0x4)
        {
            std::cout << name() << " TMR1 now at " << value_16bit << ", next event at " << break_value << '\n';
        }

        uint64_t fc = get_cycles().get()
                     + (uint64_t)((break_value - value_16bit) * prescale * ext_scale);

        if (future_cycle)
        {
            get_cycles().reassign_break(future_cycle, fc, this);

        }
        else
        {
            get_cycles().set_break(fc, this);
        }

        future_cycle = fc;

        // Setup to update for GUI breaks
        if (tmr1_interface == nullptr)
        {
            tmr1_interface = new TMR1_Interface(this);
            get_interface().prepend_interface(tmr1_interface);
        }

    }
    else
    {
        // turn off the timer and save the current value
        if (future_cycle)
        {
            current_value();
            get_cycles().clear_break(this);
            future_cycle = 0;
        }
    }
}


void TMRL::put(unsigned int new_value)
{
    set_ext_scale();
    trace.raw(write_trace.get() | value.get());
    value.put(new_value & 0xff);

    if (!tmrh || !t1con)
    {
        return;
    }

    synchronized_cycle = get_cycles().get();
    last_cycle = synchronized_cycle - (int64_t)((value.get()
                 + (tmrh->value.get() << 8)) * prescale * ext_scale + 0.5);
    current_value();

    if (t1con->get_tmr1on())
    {
        update();
    }
}


unsigned int TMRL::get()
{
    trace.raw(read_trace.get() | value.get());
    return get_value();
}


// For the gui and CLI
unsigned int TMRL::get_value()
{
    // If the TMRL is being read immediately after being written, then
    // it hasn't had enough time to synchronize with the PIC's clock.
    if (get_cycles().get() <= synchronized_cycle)
    {
        return value.get();
    }

    // If TMRL is not on, then return the current value
    if (!t1con->get_tmr1on())
    {
        return value.get();
    }

    current_value();
    return value.get();
}


//%%%FIXME%%% inline this
// if break inactive (future_cycle == 0), just read the TMR1H and TMR1L
// registers otherwise compute what the register should be and then
// update TMR1H and TMR1L.
// RP: Using future_cycle here is not strictly right. What we really want is
// the condition "TMR1 is running on a GPSIM-generated clock" (as opposed
// to being off, or externally clocked by a stimulus). The presence of a
// breakpoint is _usually_ a good indication of this, but not while we're
// actually processing that breakpoint. For the time being, we work around
// this by calling current_value "redundantly" in callback()
//
void TMRL::current_value()
{
    if (!tmrh)
    {
        return;
    }

    if (future_cycle == 0)
    {
        value_16bit = tmrh->value.get() * 256 + value.get();
    }
    else
    {
        value_16bit = (uint64_t)((get_cycles().get() - last_cycle) /
                                (prescale * ext_scale));

        if (value_16bit > 0x10000)
        {
            std::cerr << name() << " overflow TMRL " << value_16bit << '\n';
        }

        value.put(value_16bit & 0xff);
        tmrh->value.put((value_16bit >> 8) & 0xff);
    }
}


unsigned int TMRL::get_low_and_high()
{
    // If the TMRL is being read immediately after being written, then
    // it hasn't had enough time to synchronize with the PIC's clock.
    if (get_cycles().get() <= synchronized_cycle)
    {
        return value.get();
    }

    current_value();
    trace.raw(read_trace.get() | value.get());
    trace.raw(tmrh->read_trace.get() | tmrh->value.get());
    return value_16bit;
}


// set m_bExtClkEnable is tmr1 is being clocked by an external stimulus
void TMRL::new_clock_source()
{
    m_bExtClkEnabled = false;
    current_value();

    switch (t1con->get_tmr1cs())
    {
    case 0:	// Fosc/4
        if (verbose & 0x4)
        {
            std::cout << name() << " Tmr1 Fosc/4 \n";
        }

        put(value.get());
        break;

    case 1:	// Fosc
        if (verbose & 0x4)
        {
            std::cout << name() << " Tmr1 Fosc \n";
        }

        put(value.get());
        break;

    case 2:	// External pin or crystal
        if (t1con->get_t1oscen())  	// External crystal, simulate
        {
            if (verbose & 0x4)
            {
                std::cout << name() << " Tmr1 External Crystal\n";
            }

            put(value.get());    // let TMRL::put() set a cycle counter break point

        }
        else  	// external pin
        {
            if (verbose & 0x4)
            {
                std::cout << name() << " Tmr1 External Stimuli\n";
            }

            if (future_cycle)
            {
                // Compute value_16bit with old prescale and ext_scale
                current_value();
                get_cycles().clear_break(this);
                future_cycle = 0;
            }

            prescale = 1 << t1con->get_prescale();
            prescale_counter = prescale;
            set_ext_scale();
            m_bExtClkEnabled = true;
        }

        break;

    case 3:	// Capacitor sense oscillator
        if (verbose & 0x4)
        {
            std::cout << name() << " Tmr1 Cap. sensing oscillator\n";
        }

        if (future_cycle)
        {
            // Compute value_16bit with old prescale and ext_scale
            current_value();
            get_cycles().clear_break(this);
            future_cycle = 0;
        }

        prescale = 1 << t1con->get_prescale();
        prescale_counter = prescale;
        set_ext_scale();
        break;
    }
}


//
// clear_timer - This is called by either the CCP or PWM modules to
// reset the timer to zero. This is rather easy since the current TMR
// value is always referenced to the cpu cycle counter.
//

void TMRL::clear_timer()
{
    synchronized_cycle = get_cycles().get();
    last_cycle = synchronized_cycle;
    value.put(0);
    tmrh->value.put(0);

    if (verbose & 0x4)
    {
        std::cout << name() << " TMR1 has been cleared\n";
    }
}


// TMRL callback is called when the cycle counter hits the break point that
// was set in TMRL::put. The cycle counter will clear the break point, so
// we don't need to worry about it. At this point, TMRL is rolling over.

void TMRL::callback()
{
    if (verbose & 4)
    {
        std::cout << name() << " TMRL::callback\n";
    }

    // If TMRL is being clocked by the external clock, then at some point
    // the simulate code must have switched from the internal clock to
    // external clock. The cycle break point was still set, so just ignore it.
    if ((t1con->get_tmr1cs() == 2) && ! t1con->get_t1oscen())
    {
        if (verbose & 4)
        {
            std::cout << name() << " TMRL:callback No oscillator\n";
        }

        value.put(0);
        tmrh->value.put(0);
        future_cycle = 0;  // indicates that TMRL no longer has a break point
        return;
    }

    current_value();      // Because this relies on future_cycle, we must call it before clearing that
    future_cycle = 0;     // indicate that there's no break currently set

    if (break_value < 0x10000)
    {
        // The break was due to a "compare"
        if (value_16bit != break_value)
        {
            std::cout << name() << " TMR1 compare break: value=" << value_16bit << " but break_value=" << break_value << '\n';
        }

        if (verbose & 4)
        {
            std::cout << name() << " TMR1 break due to compare "  << std::hex << get_cycles().get() << '\n';
        }

        for (TMR1CapComRef * event = compare_queue; event; event = event->next)
        {
            if (event->value == break_value)
            {
                // This CCP channel has a compare at this time
                event->ccpcon->compare_match();
            }
        }

    }
    else
    {
        // The break was due to a roll-over

        //std::cout << name() << " TMRL rollover: " << hex << cycles.get() << '\n';
        if (m_Interrupt)
        {
            m_Interrupt->Trigger();
        }


        if (tmr135_overflow_server)
	{
	    tmr135_overflow_server->send_data(true, tmr_number);
	}

        // Reset the timer to 0.
        synchronized_cycle = get_cycles().get();
        last_cycle = synchronized_cycle;
        value.put(0);
        tmrh->value.put(0);
    }

    update();
}


//---------------------------

void TMRL::callback_print()
{
    std::cout << name() << " TMRL CallBack ID " << CallBackID << '\n';
}


//---------------------------

void TMRL::sleep()
{
    m_sleeping = true;
    Dprintf(("TMRL::sleep t1sysc %d\n", t1con->get_t1sync()));
    // If tmr1 is running off Fosc/4 or Fosc this assumes Fosc stops during sleep

    if (t1con->get_tmr1on() && t1con->get_tmr1cs() != 2)
    {
        if (future_cycle)
        {
            current_value();
            get_cycles().clear_break(this);
            future_cycle = 0;
        }
    }
}


//---------------------------

void TMRL::wake()
{
    m_sleeping = false;
    Dprintf(("TMRL::wake\n"));

    if (t1con->get_tmr1on() && t1con->get_tmr1cs() != 2)
    {
        update();
    }
}


//--------------------------------------------------
// member functions for the PR2 base class
//--------------------------------------------------

PR2::PR2(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc)
{
}


void PR2::put(unsigned int new_value)
{
    trace.raw(write_trace.get() | value.get());
    Dprintf(("PR2:: put %x\n", new_value));

    if (value.get() != new_value)
    {
        if (tmr2)
        {
            tmr2->new_pr2(new_value);
        }

        value.put(new_value);
    }
    else
    {
        value.put(new_value);
    }
}


//--------------------------------------------------
// member functions for the T2CON base class
//--------------------------------------------------

T2CON::T2CON(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc)
{
}


void T2CON::put(unsigned int new_value)
{
    trace.raw(write_trace.get() | value.get());
    unsigned int diff = value.get() ^ new_value;
    value.put(new_value);

    RRprint((stderr, "T2CON::put %s new_value=0x%x tmr2=%p %x\n", name().c_str(), new_value, tmr2, diff));

    if (tmr2)
    {

        if (diff & TMR2ON)
        {
            tmr2->on_or_off((bool)(value.get() & TMR2ON));
        }
	else if (diff)
            tmr2->new_pre_post_scale();
    }
}


void T2CON_128::put(unsigned int new_value)
{
    trace.raw(write_trace.get() | value.get());
    unsigned int diff = value.get() ^ new_value;
    value.put(new_value);

    RRprint((stderr, "T2CON_128::put  new_value=0x%x %s diff=%x on=%d pt_hlt=%p\n", new_value, name().c_str(), diff, (bool)(new_value & TMR2ON), pt_hlt));

    if (pt_hlt)
    {
 	if (diff & TMR2ON)
	{
	    const char *reg_name = name().c_str();
	    tmr2->tmr_number = reg_name[1];
            tmr2->on_or_off((bool)(value.get() & TMR2ON));
	}
	else if (diff)
	    pt_hlt->new_pre_post_scale();
    }
    else if (tmr2)
    {

        if (diff & TMR2ON)
        {
            tmr2->on_or_off((bool)(value.get() & TMR2ON));
        }
	else if (diff)
            tmr2->new_pre_post_scale();
    }
    else
	assert(tmr2);
}

unsigned int T2CON_128::get_pre_scale()
{
    unsigned int ckps = (value.get() & CKPS_mask) >> CKPS_shift;
    return 1<<ckps;
}
unsigned int T2CON_128::get_post_scale()
{
    return (value.get() & OUTPS_mask)+1;
}


//--------------------------------------------------
// member functions for the TMR2 base class
//--------------------------------------------------
// Catch stopped simulation and update value of TMR2
class TMR2_Interface : public Interface
{
public:
    explicit TMR2_Interface(TMR2 *_tmr2)
        : Interface((void **)_tmr2), tmr2(_tmr2)
    {
    }
    void SimulationHasStopped(void * /* object */ ) override
    {
        tmr2->current_value();
    }
    void Update(void * /* object */ ) override
    {
        tmr2->current_value();
    }

private:
    TMR2 *tmr2;
};


TMR2::TMR2(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc),
      update_state(TMR2_ANY_PWM_UPDATE | TMR2_PR2_UPDATE), prescale(1)
{
    ssp_module[0] = ssp_module[1] = nullptr;
    value.put(0);

    tmr_number = pName[3];

    std::fill_n(duty_cycle, MAX_PWM_CHANS, 0);
    std::fill_n(m_clc, 4, nullptr);
    std::fill_n(ccp, MAX_PWM_CHANS, nullptr);
}


TMR2::~TMR2()
{
    if (m_Interrupt)
    {
        m_Interrupt->release();
    }
    if (tmr246_server)
	delete tmr246_server;

    delete tmr2_interface;
}


void TMR2::callback_print()
{
    std::cout << name() << " TMR2 CallBack ID " << CallBackID << '\n';
}



// Catch output from CLC module
void TMR2::out_clc(bool level, char index)
{
    assert(0);
}
void TMR2::increment()
{
    if(!running || !enabled) // counting only works if t2 is on
	return;
    // if pulsing output, turn output off
#ifdef RRR
    if (tmrx_clkcon)
    {
	get_tmr246_server()->send_data(true, tmr_number-'0');
	get_tmr246_server()->send_data(false, tmr_number-'0');
    }
#endif
    if (++prescale_counter >=  prescale)
    {
	unsigned int tmr2_val = value.get()+1;

        value.put(tmr2_val);
    RRprint((stderr, "TMR2::increment %s tmr2_val=%d break+value=%d prescale_counter=%d\n", name().c_str(), tmr2_val, break_value, prescale_counter));
	prescale_counter = 0;
        if (tmr2_val == break_value)
	{
	    new_t2_edge();
	    if (last_update&TMR2_PR2_UPDATE)
		value.put(0);
//	    else
//		value.put(++tmr2_val);
	    break_value = next_break();
   RRprint((stderr, "\t%s new_break=%d last_update=0x%x\n", name().c_str(), break_value, last_update));
	}
/*
	else
	{
	    value.put(++tmr2_val);
	}
*/

    }
}

bool TMR2::add_ccp(CCPCON * _ccp)
{
    int cc;

    for (cc = 0; cc < MAX_PWM_CHANS; cc++)
    {
        if (ccp[cc] == 0 || ccp[cc] == _ccp)
        {
            ccp[cc] = _ccp;
            return true;
        }
    }

    return false;
}


bool TMR2::rm_ccp(CCPCON * _ccp)
{
    int cc;

    for (cc = 0; cc < MAX_PWM_CHANS; cc++)
    {
        if (ccp[cc] == _ccp)
        {
            ccp[cc] = 0;
            return true;
        }
    }

    return false;
}

void TMR2::reset_value(bool on)
{
    if (!running)
	return;

     RRprint((stderr, "%s::reset_value(%d) delta=%d resid=%d flags=0x%x\n", name().c_str(), on, (int)(get_cycles().get() - zero_cycle), (int)(get_cycles().get() - zero_cycle) % prescale, last_update));
    value.put(0);

    if (on)	// go into reset mode
    {
	last_update |= TMR2_RESET;
	uint64_t fc = get_cycles().get() + 2;
	if (future_cycle)
	    get_cycles().reassign_break(future_cycle, fc, this);
	else
	    get_cycles().set_break(fc, this);
        future_cycle = fc;
    }
    else
    {
	last_update &= ~TMR2_RESET;
	last_update |= TMR2_PAUSE;
	uint64_t fc = get_cycles().get() + 2;
	if (future_cycle)
	    get_cycles().reassign_break(future_cycle, fc, this);
	else
	    get_cycles().set_break(fc, this);
        future_cycle = fc;
    }
}
// Reset TMRx to zero and pause for two clock cycles
void TMR2::reset_edge()
{
    RRprint((stderr, "%s::reset_edge running=%d delta=%d resid=%d\n", name().c_str(),running, (int)(get_cycles().get() - zero_cycle), (int)(get_cycles().get() - zero_cycle) % prescale));
    if (!running)
	running = true;
//	return;


    value.put(0);
    prescale_counter = 0;

    count_from_zero();
}

bool TMR2::count_from_zero()
{
	new_pre_post_scale();
	RRprint((stderr, "TMR2::count_from_zero %s now=%ld value=%d prescale_count=%d\n", name().c_str(), get_cycles().get(), value.get(), prescale_counter));
        zero_cycle = get_cycles().get() - value.get() * prescale - prescale_counter;
	break_value = next_break();
	RRprint((stderr, "\tTMR2::count_from_zero break_value=%d zero_cycle=%ld last_update=0x%x\n", break_value, zero_cycle, last_update));
    	if (last_update & TMR2_ANY_PWM_UPDATE)
        {
            int modeMask = TMR2_PWM1_UPDATE;
            for (int cc = 0; cc < MAX_PWM_CHANS && last_update; cc++)
            {
                if (last_update & modeMask)
                {
                    if (ccp[cc])        // shouldn't be needed
                        ccp[cc]->pwm_match(1);
            	}
                modeMask <<= 1;
            }
	    if (use_clk)
	    {
		RRprint((stderr, "now=%ld zero_cycle=%ld adjust=%d prescale_counter=%d ", get_cycles().get(), zero_cycle, value.get() * prescale, prescale_counter));
                zero_cycle = get_cycles().get() - value.get() * prescale - prescale_counter;
		RRprint((stderr, " new zero_cycle=%ld fc=%ld\n", zero_cycle, zero_cycle + break_value));
		uint64_t fc = zero_cycle + break_value;
		RRprint((stderr, "\tfc=%ld now=%ld\n", fc, get_cycles().get()));
		assert(fc > get_cycles().get());
		if (future_cycle)
		{
                    get_cycles().reassign_break(future_cycle, fc, this);
                    future_cycle = fc;
		}
		else
		{
		    future_cycle = fc;
		    get_cycles().set_break(future_cycle, this);
		}
		return(true);
	     }
          }
    return(false);
}


void TMR2::on_or_off(int new_state)
{
    prescale = t2con->get_pre_scale();
    post_scale = t2con->get_post_scale();
    RRprint((stderr, "TMR2::on_or_off on=%d %s=0x%x running=%d->%d enabled=%d now=%ld last=%ld prescale=%d postscale=%d future_cycle=%ld\n", new_state, name().c_str(), value.get(), running, new_state, enabled, get_cycles().get(), zero_cycle, prescale, post_scale, future_cycle));

    running = new_state;
    if (!running || !enabled)
    {
	RRprint((stderr, "\tstop clock now=%ld ", get_cycles().get()));

	if (future_cycle)
	{
	     unsigned int delta = ((get_cycles().get() - zero_cycle)/clk_ratio) + 0.5;
	     value.put(delta/prescale);
	     prescale_counter = delta % prescale;
	RRprint((stderr, "\tzero_cycle=%ld new_value=%d prescale_counter=%d",  zero_cycle, value.get(), prescale_counter));
	     zero_cycle = 0;
             get_cycles().clear_break(this);
             future_cycle = 0;
	}
	RRprint((stderr, "\n"))
	return;
    }

    // turn on timer
    if (new_state)
    {
        Dprintf(("TMR2 is being turned on pwm_mode=%d\n", pwm_mode));
        // turn on the timer
        // Effective last cycle
        // Compute the "effective last cycle", i.e. the cycle
        // at which TMR2 was last 0 had it always been counting.
        zero_cycle = get_cycles().get() - (value.get() * prescale  + prescale_counter) * clk_ratio;
	RRprint((stderr, " DEBUG zero_cycle=%ld now=%ld %s=%d prescale_counter=%d\n", zero_cycle, get_cycles().get(), name().c_str(), value.get(), prescale_counter));

        if (use_clk)
        {
	    break_value = next_break();
	RRprint((stderr, "\tbreak_value=0x%x last_update=0x%x value=%d counter=%d\n", break_value, last_update, value.get(), prescale_counter));
//	    if (value.get() == 0 && prescale_counter == 0)
	    if (last_update & TMR2_ANY_PWM_UPDATE)
	    {
    		if (tx_hlt)
    		{
		    unsigned int mode = (tx_hlt->value.get() & 0b11111);
		    if ((mode >= 0b01000 && mode <= 0b01111) ||
		        ( mode >= 0b10001 && mode <= 0b10011) ||
			 mode == 0b10110 || mode == 0b10111
		    )
		    {
        		for (int cc = 0; cc < MAX_PWM_CHANS; cc++)
        		{
            		    if (ccp[cc] &&  ccp[cc]->is_pwm())
            		    {
                		ccp[cc]->pwm_match(1);
            		    }
        		}
		    }
		}

	    }
	    RRprint((stderr, "\tTMR2::on_or_off  %s=%d prescale_counter=%d now = %ld zero_cycle=%ld\n", name().c_str(), value.get(), prescale_counter, get_cycles().get(), zero_cycle));
            update();

            if (tmr2_interface == nullptr)
            {
                tmr2_interface = new TMR2_Interface(this);
                get_interface().prepend_interface(tmr2_interface);
            }
	}
	else
	{
	    prescale_counter = 0;
	    break_value = next_break();
        }
	running = true;

    }
    // Turn off timer
    else
    {
	running = false;
        Dprintf(("TMR2 is being turned off\n"));
  	if (use_clk)
        {
             // turn off the timer and save the current value
             prescale_counter = (get_cycles().get()-zero_cycle)%prescale;
     	RRprint((stderr, "\t%s off value=%d delta=%d prescale_counter=%d last=%ld\n", name().c_str(), value.get(), (int)(get_cycles().get()-zero_cycle), prescale_counter, zero_cycle));
             current_value();
             if (future_cycle)
             {
                 get_cycles().clear_break(this);
         	 future_cycle = 0;
             }
	}
    }
}



//
// pwm_dc - set PWM duty cycle
//
//

void TMR2::pwm_dc(unsigned int dc, unsigned int ccp_address)
{
    int modeMask = TMR2_PWM1_UPDATE;
    int cc;

    for (cc = 0; cc < MAX_PWM_CHANS; cc++)
    {
        if (ccp[cc] && (ccp_address == ccp[cc]->address) && ccp[cc]->is_pwm())
        {
            Dprintf(("TMR2::pwm_dc duty cycle 0x%x ccp_address 0x%x %s\n", dc, ccp_address, ccp[cc]->name().c_str()));
            duty_cycle[cc] = dc;
            pwm_mode |= modeMask;
            return;
        }

        modeMask <<= 1;
    }

    std::cout << name() << ": error bad ccpxcon address while in pwm_dc()\n";
    std::cout << "    ccp_address = " << ccp_address << " expected one of";

    for (cc = 0; cc < MAX_PWM_CHANS; cc++)
    {
        if (ccp[cc])
        {
            std::cout << " " << ccp[cc]->address;
        }
    }

    std::cout << '\n';
}


//
// stop_pwm
//

void TMR2::stop_pwm(unsigned int ccp_address)
{
    int modeMask = TMR2_PWM1_UPDATE;
    int old_pwm = pwm_mode;

    for (int cc = 0; cc < MAX_PWM_CHANS; cc++)
    {
        if (ccp[cc] && (ccp_address == ccp[cc]->address))
        {
            // std::cout << name() << " TMR2:  stopping pwm mode with ccp" << cc+1 << ".\n";
            pwm_mode &= ~modeMask;

            if (last_update & modeMask)
            {
                update_state &= ~modeMask;
            }
        }

        modeMask <<= 1;
    }

    if ((pwm_mode ^ old_pwm) && future_cycle && t2con->get_tmr2on())
    {
        update();
    }
}


//
// update
//  This member function will determine if/when there is a TMR2 break point
// that needs to be set and will set/move it if so.
//  There are two different types of break sources:
//     1) TMR2 matching PR2
//     2) TMR2 matching one of the ccp registers in pwm mode
//

void TMR2::update()
{

    RRprint((stderr, "TMR2::update %s=%d running=%d now=%ld use_clk=%d enabled=%d future_cycle=%ld\n", name().c_str(), value.get(), running, get_cycles().get(), use_clk, enabled, future_cycle));

    //std::cout << "TMR2 update. cpu cycle " << std::hex << cycles.get() <<'\n';

    if (running && use_clk && enabled)
    {
	if (future_cycle == 0)
        {
	    zero_cycle = get_cycles().get() - (value.get() * prescale + prescale_counter) * clk_ratio;
        }
#if DEBUG == 1
	unsigned int tmr2_val;
 	tmr2_val = (get_cycles().get() - zero_cycle) / (prescale * clk_ratio);
#endif

	break_value = next_break();

        uint64_t fc = zero_cycle + break_value;

    RRprint((stderr, "\tTMR2::update %s_val=%d pwm=0x%x fc=%ld future=%ld delta=%ld break_value=%d \n", name().c_str(), tmr2_val, last_update & TMR2_ANY_PWM_UPDATE, fc, future_cycle, get_cycles().get() - zero_cycle , break_value));
	if (fc < get_cycles().get()) // TMR2 > PR2 + 1
	{
	    fc = get_cycles().get() + (0x100 - value.get()) * prescale;
	}

        if (future_cycle)
        {

            if (fc < future_cycle && verbose & 0x04)
	    {
                std::cout << name() << " TMR2::update note: new breakpoint=" << std::hex << fc <<
                          " before old breakpoint " << future_cycle <<
                          " now " << get_cycles().get() << '\n';
	    }
            if (fc != future_cycle)
            {
                // std::cout << name() << " TMR2::update new break at cycle "<<hex<<fc<<'\n';
                get_cycles().reassign_break(future_cycle, fc, this);
                future_cycle = fc;
            }
	    else if (fc == future_cycle && fc == get_cycles().get())
	    {
	RRprint((stderr, "assert now=fc=%ld  zero_cycle=%ld break_value=%d\n", fc, zero_cycle , break_value));
		fc++;
                get_cycles().reassign_break(future_cycle, fc, this);
                future_cycle = fc;
		//callback();
	    }


        }
        else
        {
	RRprint((stderr, "\tfuture_cycle=0 break=%d tmr2_val=%d\n", break_value, tmr2_val));
	    get_cycles().set_break(fc, this);
	    future_cycle = fc;
        }
    }
#ifdef RRR
    else
    {
        // std::cout << name() << " TMR2 is not running (no update occurred)\n";
         std::cerr << name() << " TMR2::update is not running (no update occurred)\n";
    }
#endif
}

/* Scan ccp(pwm) registers for an active pwm channels
 * Find next TMR2 offset(s) for dutycycle between now and PR2+1 cycles.
 * The last_update global variable will indicate all registers involved
 * in the next tmr2 break.
 * if no dutycycles are in the timeslot, last_update == TMR2_PR2_UPDATE
 * and PR2+1 returned.
 * If prescale < 4 break values might be integer clock cycles, so
 *
 */
unsigned int TMR2::next_break()
{
    int modeMask = TMR2_PWM1_UPDATE;
    unsigned int break_here = (1 + pr2->value.get())*prescale;
    unsigned int high = break_here; 			// max TMR2 * 4
    unsigned int low; 					//tmr2 * 4
    unsigned int dc;
#if DEBUG == 1
    int		old_update = last_update;
#endif


    if (!future_cycle)
    {
	// low_bits is top 2 bits of prescaler
	unsigned int low_bits = (prescale_counter << 2)/prescale;
	low = (((value.get() << 2) + low_bits) *prescale) >>2;
    }
    else
    {
 	low = (get_cycles().get()-zero_cycle) / clk_ratio;
    }

    last_update = TMR2_PR2_UPDATE;

    for (int cc = 0; cc < MAX_PWM_CHANS; cc++)
    {
	dc = (duty_cycle[cc] * prescale + 2)>>2;
    if (ccp[cc])
    {
    RRprint((stderr, "TMR2::next_break %s cc= %d is_pwm=%d dc = %d low=%d high=%d\n", name().c_str(), cc, ccp[cc]->is_pwm(), dc, low, high));
    }
	// active PWM in current range?
	if (ccp[cc] && ccp[cc]->is_pwm() &&
		dc > low &&
		dc <= high)
	{
	RRprint((stderr, "\tTMR2::next_break %s dc=%d break_hear=%d modMask=0x%x\n", name().c_str(), dc, break_here, modeMask));
	    if (dc < break_here)
	    {
	        break_here = dc;
		last_update = modeMask;
	    }
	    else if (dc == break_here)
	    {
		last_update |= modeMask;
	    }
	}
	modeMask <<= 1;
    }
    break_here *= clk_ratio;
    RRprint((stderr, "\tTMR2::next_break %s %s=%d last_update 0x%x->0x%x break_here=%d prescale=%d high=%d low=%d zero_cycle=%ld clk_ratio=%.2f ",
	name().c_str(),
	pr2->name().c_str(), pr2->value.get(), old_update, last_update,
	break_here, prescale, high, low, zero_cycle, clk_ratio));
    RRprint((stderr, "(%s%s%s%s)\n",
	last_update&TMR2_WRAP?"TMR2_WRAP":"",
	last_update&TMR2_PAUSE?"TMR2_PAUSE":"",
	last_update&TMR2_ANY_PWM_UPDATE?"TMR2_ANY_PWM_UPDATE":"",
	last_update&TMR2_PR2_UPDATE?"TMR2_PR2_UPDATE":""));
    return break_here;
}



void TMR2::put(unsigned int new_value)
{
//    unsigned int old_value = get_value();
    trace.raw(write_trace.get() | value.get());
    value.put(new_value & 0xff);

    prescale_counter = 0;
    if (future_cycle)
    {
        // If TMR2 is enabled (i.e. counting) then 'future_cycle' is non-zero,
        // which means there's a cycle break point set on TMR2 that needs to
        // be moved to a new cycle.
	zero_cycle = get_cycles().get() - new_value * prescale;
	update();

        /*
        'clear' the post scale counter. (I've actually implemented the
        post scale counter as a count-down counter, so 'clearing it'
        means resetting it to the starting point.
        */
        if (t2con)
        {
            post_scale = t2con->get_post_scale();
        }
    }
}


unsigned int TMR2::get()
{
    if (running && enabled)
    {
        current_value();
    }

    trace.raw(read_trace.get() | value.get());
    return value.get();
}


unsigned int TMR2::get_value()
{
    if (running && enabled)
    {
        current_value();
    }

    return value.get();
}


void TMR2::new_pre_post_scale()
{
	RRprint((stderr, "TMR2::new_pre_post_scale %s running=%d enabled=%d future_cycle=%ld \n", name().c_str(), running, enabled, future_cycle));
    //std::cout << name() << " T2CON was written to, so update TMR2 " << running << "\n";
    if (!running || !enabled)
    {
	RRprint((stderr, "\tKill future_cycle TMR2::new_pre_post_scale %s running=%d enabled=%d future_cycle=%ld \n", name().c_str(), running, enabled, future_cycle));
        // TMR2 is not on. If has just been turned off, clear the callback breakpoint.
        if (future_cycle)
        {
            get_cycles().clear_break(this);
            future_cycle = 0;
        }
	zero_cycle = 0;

        return;
    }

    unsigned int old_prescale = prescale;
    prescale = t2con->get_pre_scale();
    post_scale = t2con->get_post_scale();


    if (future_cycle)
    {
        // If TMR2 is enabled (i.e. counting) then 'future_cycle' is non-zero,
        // which means there's a cycle break point set on TMR2 that needs to
        // be moved to a new cycle.
        if (prescale != old_prescale)  	// prescaler value change
        {
            // togo is number of cycles to next callback based on new prescaler.
            uint64_t togo = (future_cycle - get_cycles().get()) * prescale / old_prescale;

            if (!togo)  	// I am not sure this can happen RRR
            {
                callback();

            }
            else
            {
                uint64_t fc = togo + get_cycles().get();
                get_cycles().reassign_break(future_cycle, fc, this);
                future_cycle = fc;
            }
        }

    }
    else if (use_clk)
    {
        //std::cout << name() << " TMR2 was off, but now it's on.\n";
        if (value.get() == pr2->value.get())   // TMR2 == PR2
        {
            future_cycle = get_cycles().get() + prescale;
            get_cycles().set_break(future_cycle, this);
            callback();

        }
        else if (value.get() > pr2->value.get())     // TMR2 > PR2
        {
            std::cout << name() << " Warning TMR2 turned on with TMR2 greater than PR2\n";
            // this will cause TMR2 to wrap
            future_cycle  = get_cycles().get() + 1;
            get_cycles().set_break(future_cycle, this);

        }
	else if (value.get() || prescale_counter)
	{
	}
        else
        {
	    RRprint((stderr, "\tstart %s prescale=%d postscale=%d pr2=%d ivalue=0x%x prescale_counter=%d\n", name().c_str(), prescale, post_scale, pr2->value.get(), value.get(), prescale_counter));
            future_cycle = get_cycles().get() + 1;
            get_cycles().set_break(future_cycle, this);
            zero_cycle = get_cycles().get() - value.get() * prescale;
            prescale_counter = 0;
            update();
        }
    }
    else
    {
	break_value = next_break();
    }
}


// PR2 has changed value
void TMR2::new_pr2(unsigned int new_value)
{
    Dprintf(("TMR2::new_pr2 running=%u enabled=%u use_clk=%u\n",
		running, enabled, use_clk));

    RRprint((stderr, "TMR2::new_pr2 %s running=%u enabled=%u use_clk=%u\n",
		name().c_str(), running, enabled, use_clk));


    if (running && enabled && use_clk)
    {
        Dprintf(("TMR2::new_pr2(0x%02X) with timer at 0x%02X -\n", new_value, value.get()));
        unsigned int cur_break = (future_cycle - zero_cycle) / prescale;
        unsigned int new_break = 1 + new_value;
        unsigned int now_cycle = (get_cycles().get() - zero_cycle) / prescale;
        uint64_t fc = zero_cycle;
        Dprintf(("   cur_break = 0x%X,  new_break = 0x%X,  now = 0x%X\n", cur_break, new_break, now_cycle));
        Dprintf(("   zero_cycle = 0x%" PRINTF_GINT64_MODIFIER "X\n", fc));

	RRprint((stderr, "new_pr2 cur_break=%d new_break=%d now_cycle=%d\n", cur_break, new_break, now_cycle));

        /*
        PR2 change cases

        1> TMR2 greater than new PR2
        TMR2 wraps through 0xff
        2> New PR2 breakpoint less than current breakpoint or
        current break point due to PR2
        Change breakpoint to new value based on PR2
        3> Other breakpoint < PR2 breakpoint
        No need to do anything.
        */

        if (now_cycle > new_break)  	// TMR2 > PR2 do wrap
        {
            // set break to when TMR2 will overflow
            last_update |= TMR2_WRAP;
            fc += 0x100 * prescale;
            Dprintf(("   now > new, set future_cycle to 0x%" PRINTF_GINT64_MODIFIER "X\n", fc));
            get_cycles().reassign_break(future_cycle, fc, this);
            future_cycle = fc;

        }
        else if (cur_break == break_value ||	// breakpoint due to pr2
                 new_break < cur_break)  	// new break less than current
        {
            fc += new_break * prescale;
            Dprintf(("   new<break, set future_cycle to 0x%" PRINTF_GINT64_MODIFIER "X\n", fc));

            if (cur_break != break_value)
            {
		RRprint((stderr, "TMR2::new_pr2 cur_break != break_value\n"));
                last_update = TMR2_PR2_UPDATE;  // RP : fix bug 3092906
            }

	RRprint((stderr, "new_pr2 fc=%ld future_cycle=%ld\n",fc, future_cycle));
            get_cycles().reassign_break(future_cycle, fc, this);
            future_cycle = fc;
            Dprintf(("   next event mask %02X\n", last_update));
        }
    }
    else if (running && enabled)
    {
        fprintf(stderr, "FIXME new_pr2\n");
    }
}


void TMR2::current_value()
{
    unsigned int tmr2_val = value.get();

    if (future_cycle && !(last_update & TMR2_PAUSE))
    {
        tmr2_val = (get_cycles().get() - zero_cycle) / (prescale * clk_ratio);
    }

    if (tmr2_val == max_counts())
    {
        // tmr2 is about to roll over. However, the user code
        // has requested the current value before the callback function
        // has been invoked. So do callback and return 0.
        if (future_cycle)
        {
            callback();
        }

        tmr2_val = 0;
    }


    if (tmr2_val >= max_counts())  	// Can get to max_counts  during transition
    {
        std::cerr << name() << " TMR2 BUG!!  value = 0x" << std::hex << tmr2_val << " which is greater than 0x";
        std::cerr << std::hex << max_counts() << '\n';
        RRprint((stderr, "future_cycle = %ld now=%ld zero_cycle=%ld\n", future_cycle, get_cycles().get(), zero_cycle));
        if (future_cycle)
        {
            callback();
        }

        tmr2_val = 0;

    }
    value.put(tmr2_val & (max_counts() - 1));
}

// zero TMR2/4/6 and set pwm high if active
void TMR2::zero_tmr246()
{
    value.put(0);
    zero_cycle = get_cycles().get();
    prescale_counter = 0;
    for (int cc = 0; cc < MAX_PWM_CHANS; cc++)
    {
        if (ccp[cc] &&  ccp[cc]->is_pwm())
        {
            ccp[cc]->pwm_match(1);
        }
    }
}

// TMR2 callback is called when the cycle counter hits the break point that
// was set in TMR2::put. The cycle counter will clear the break point, so
// we don't need to worry about it. At this point, TMR2 is equal to PR2.

void TMR2::callback()
{
    //std::cout<< name() << " TMR2 callback cycle: " << hex << cycles.value << '\n';
    RRprint((stderr, "%s::callback now=%ld post_scale=%d last_update=0x%x update_state=0x%x\n", name().c_str(), get_cycles().get(), post_scale, last_update, update_state));

    if (last_update & TMR2_RESET)
    {
	zero_tmr246();
	future_cycle = 0;
	return;
    }
    if (running && enabled)
    {
        // What caused the callback: PR2 match or duty cyle match ?
        RRprint((stderr, "\tcallback running %s mode=0x%x\n", name().c_str(), update_state));

	new_t2_edge();
        update();

    }
    else //RRR if (!running)
    {
        future_cycle = 0;
    }
}

// Timer has expired
void TMR2::new_t2_edge()
{
    RRprint((stderr, "TMR2::new_t2_edge() %s last_update=0x%x (%s%s%s%s)\n",
	name().c_str(),
	last_update,
	last_update&TMR2_WRAP?"TMR2_WRAP":"",
	last_update&TMR2_PAUSE?"TMR2_PAUSE":"",
	last_update&TMR2_ANY_PWM_UPDATE?"TMR2_ANY_PWM_UPDATE":"",
	last_update&TMR2_PR2_UPDATE?"TMR2_PR2_UPDATE":""));
    // Are we wraping because TMR2 > PR2+1 or in pause after a gate event
    if ((last_update & TMR2_WRAP) || (last_update & TMR2_PAUSE))
    {
        last_update &= ~(TMR2_WRAP | TMR2_PAUSE);
        // This resets the timer to zero:
        zero_tmr246();
    }
    // end of 1 or more duty cycles
    else if (last_update & TMR2_ANY_PWM_UPDATE)
    {
        int modeMask = TMR2_PWM1_UPDATE;

        for (int cc = 0; cc < MAX_PWM_CHANS && last_update; cc++)
        {
            if (last_update & modeMask)
            {
                // duty cycle match
                //std::cout << name() << ": duty cycle match for pwm" << cc+1 << "\n";
                update_state &= (~modeMask);
                last_update &= ~modeMask;

                if (ccp[cc])        // shouldn't be needed
                {
		    RRprint((stderr, "\tcall pwm_match cc=%d\n", cc));
                    ccp[cc]->pwm_match(0);

                }
                else
                {
                    std::cerr << name() << " TMR2::callback() found update of non-existent CCP\n";
                }
            }

            modeMask <<= 1;
        }

    }
    // end of period
    else
    {
        // matches PR2
        pr2_match();

        update_state = TMR2_ANY_PWM_UPDATE | TMR2_PR2_UPDATE;
    }
}

void TMR2::pr2_match()
{
    //std::cout << name() << " TMR2: PR2 match. pwm_mode is " << pwm_mode <<'\n';
    // This (implicitly) resets the timer to zero:
    zero_cycle = get_cycles().get();
    if (tmr246_server)
	tmr246_server->send_data(true, MATCH | (tmr_number - '0'));
    for (int i = 0; i < 4; i++)
    {
        if (m_clc[i])
        {
            m_clc[i]->t246_match(tmr_number - '0');
        }
    }

    if (ssp_module[0])
    {
        ssp_module[0]->tmr2_clock();
    }

    if (ssp_module[1])
    {
        ssp_module[1]->tmr2_clock();
    }

    if (m_txgcon)  	// toggle T2_gate, if present
    {
        m_txgcon->T2_gate(1);
        m_txgcon->T2_gate(0);
    }

    if (tx_hlt && (tx_hlt->value.get() & 0x18))
    {
	unsigned int mode = (tx_hlt->value.get() & 0b11111);

	// if oneshot clear t2con ON bit
	if (((mode >= 0b01000) && (mode <= 0b01111)) ||
		(mode == 0b10110) || mode == 0b10111)
	{
	    on_or_off(false);
	    if (mode != 0b01000) // everything else needs trigger to start
	        set_enable(false);
	    RRprint((stderr, "TMR2::pr2_match ONESHOT %s mode=%s t2con=0x%x  ~(get_tmr2on() = 0x%x\n", name().c_str(), std::bitset<5>(mode).to_string().c_str(), t2con->value.get(), (~(t2con->get_tmr2on())&0xffff)));
	    t2con->put(t2con->value.get() & ~(t2con->get_tmr2on()));
	}
	// monostable stop counting
	else if ((mode & 0b10000) && (mode != 0b10000))
	{
	    RRprint((stderr, "TMR2::pr2_match MONOSTABLE %s mode=%s\n", name().c_str(), std::bitset<5>(mode).to_string().c_str()));
	    set_enable(false);
	}
	RRprint((stderr, "%s tx_hlt=%p 0x%d mode=%d t2con=0x%x\n", name().c_str(), tx_hlt, tx_hlt->value.get(), mode, t2con->value.get()));
	RRprint((stderr, "\tlast_update=0x%x\n", last_update));
    }
    else
    {
        for (int cc = 0; cc < MAX_PWM_CHANS; cc++)
        {
            // RRR FIX
            if (ccp[cc] &&  ccp[cc]->is_pwm())
            {
                ccp[cc]->pwm_match(1);
            }
        }
    }

    if (--post_scale <= 0)
    {
        //std::cout << name() << " setting IF\n";
        if (tmrx_clkcon)
        {
#if DEBUG == 1
            const char *myname = name().c_str();
            RRprint((stderr, "post_scale %s tmrx_clkcon=%p tmr_numer=%c %c\n", name().c_str(), tmrx_clkcon, tmr_number, myname[3]));
#endif
	    get_tmr246_server()->send_data(true,  POSTSCALE | (tmr_number-'0'));
	    get_tmr246_server()->send_data(false, POSTSCALE | (tmr_number-'0'));
	}
        if (m_Interrupt)     // for multiple T2 (T2, T4, T6)
            m_Interrupt->Trigger();
        else if (pir_set)
            pir_set->set_tmr2if();
        post_scale = t2con->get_post_scale();
    }
}
#ifdef RRR
void TMR2::clc_data(bool v1, int cm)
{
    RRprint((stderr, "%s::clc_data Fixme no code v1=%d cm=%d\n", name().c_str(), v1, cm));
}
#endif
void TMR2::set_enable(bool on_off, bool zero)
{
 RRprint((stderr, "TMR2::set_enable %s enabled=%d->%d running=%d tmr2on=%d zero=%d\n", name().c_str(), enabled, on_off, running, (bool)t2con->get_tmr2on(), zero));
    enabled = on_off;


//   if (enabled && (t2con->get_tmr2on() != running)
    on_or_off(enabled && (bool)t2con->get_tmr2on());
//RRR    if (enabled && (t2con->get_tmr2on()))
//         on_or_off(enabled);
    if (zero && !enabled)
    {
	 value.put(0);
         prescale_counter = 0;
	 zero_cycle = 0;
    }
}

DATA_SERVER *TMR2::get_tmr246_server()
{
    if (tmr246_server == nullptr)
        tmr246_server = new DATA_SERVER(DATA_SERVER::TMR2);
    return tmr246_server;
}


//------------------------------------------------------------------------
// TMR2_MODULE
//
//

TMR2_MODULE::TMR2_MODULE()
{
}


void TMR2_MODULE::initialize(T2CON *t2con_, PR2 *pr2_, TMR2  *tmr2_)
{
    t2con = t2con_;
    pr2   = pr2_;
    tmr2  = tmr2_;
}

class Tx_RST_RECEIVER : public DATA_RECEIVER
{
public:
    explicit Tx_RST_RECEIVER(TMRx_RST *_pt_rst, const char *_name) :
        DATA_RECEIVER(_name), pt_rst(_pt_rst)
    {}
    virtual ~Tx_RST_RECEIVER(){}
    void rcv_data(int v1, int v2) override;

private:
    TMRx_RST *pt_rst;

};

// process output from other modules for external reset
void Tx_RST_RECEIVER::rcv_data(int v1, int v2)
{
#if DEBUG == 1
    int serv = v2 & DATA_SERVER::SERV_MASK;
    v2 = v2 & ~DATA_SERVER::SERV_MASK;
    RRprint((stderr, "Tx_RST_RECEIVER::rcv_data %s %s v1=%d v2=%d\n", pt_rst->name().c_str(), mod_name[serv>>12], v1, v2));
#endif

    pt_rst->TMRx_ers(v1);
    return;

}

class Tx_CLK_RECEIVER : public DATA_RECEIVER
{
public:
    explicit Tx_CLK_RECEIVER(TMRx_CLKCON *_pt_clk, const char *_name) :
        DATA_RECEIVER(_name), pt_clk(_pt_clk)
    {}
    virtual ~Tx_CLK_RECEIVER(){}
    void rcv_data(int v1, int v2) override;

private:
    TMRx_CLKCON *pt_clk;

};

void Tx_CLK_RECEIVER::rcv_data(int v1, int v2)
{
    int serv = v2 & DATA_SERVER::SERV_MASK;
    v2 = v2 & ~DATA_SERVER::SERV_MASK;

    RRprint((stderr, "Tx_CLK_RECEIVER::rcv_data %s %s v1=%d v2=%d\n", pt_clk->name().c_str(), mod_name[serv>>12], v1, v2));
    switch(serv)
    {
    case DATA_SERVER::CLC:
        pt_clk->clc_data_clk(v1, v2);
	break;

    case DATA_SERVER::ZCD:
        pt_clk->zcd_data_clk(v1, v2);
	break;

    case DATA_SERVER::AT1:
	if ((v2 & ATx::ATxMask) == ATx::PERCLK)
            pt_clk->at1_data_clk(v1, v2 & ~(ATx::ATxMask));
	break;

    default:
	fprintf(stderr, "Tx_CLK_RECEIVER unexpected server 0x%x\n", v2 & DATA_SERVER::SERV_MASK);
	break;
    }

}

class Tx_CLC_RST_RECEIVER : public DATA_RECEIVER
{
public:
    explicit Tx_CLC_RST_RECEIVER(TMRx_RST *_pt_rst, const char *_name) :
        DATA_RECEIVER(_name), pt_rst(_pt_rst)
    {}
    virtual ~Tx_CLC_RST_RECEIVER(){}
    void rcv_data(int v1, int v2) override;

private:
    TMRx_RST *pt_rst;

};

void Tx_CLC_RST_RECEIVER::rcv_data(int v1, int v2)
{
    pt_rst->clc_data_rst(v1, v2 & ~DATA_SERVER::SERV_MASK);
}
class Tx_ZCD_RST_RECEIVER : public DATA_RECEIVER
{
public:
    explicit Tx_ZCD_RST_RECEIVER(TMRx_RST *pt, const char *_name) :
        DATA_RECEIVER(_name), pt_rst(pt)
    {}
    virtual ~Tx_ZCD_RST_RECEIVER(){}
    void rcv_data(int v1, int v2) override;

private:
    TMRx_RST *pt_rst;
};

void Tx_ZCD_RST_RECEIVER::rcv_data(int v1, int v2)
{
        pt_rst->zcd_data_rst(v1, v2 & ~DATA_SERVER::SERV_MASK);
}

class Tx_CLC_CLK_RECEIVER : public DATA_RECEIVER
{
public:
    explicit Tx_CLC_CLK_RECEIVER(TMRx_CLKCON *pt, const char *_name) :
        DATA_RECEIVER(_name), pt_clk(pt)
    {}
    virtual ~Tx_CLC_CLK_RECEIVER(){}
    void rcv_data(int v1, int v2) override;

private:
    TMRx_CLKCON *pt_clk;
};

void Tx_CLC_CLK_RECEIVER::rcv_data(int v1, int v2)
{
        pt_clk->clc_data_clk(v1, v2 & ~DATA_SERVER::SERV_MASK);
}




class Tx_ZCD_CLK_RECEIVER : public DATA_RECEIVER
{
public:
    explicit Tx_ZCD_CLK_RECEIVER(TMRx_CLKCON *pt, const char *_name) :
        DATA_RECEIVER(_name), pt_clk(pt)
    {}
    virtual ~Tx_ZCD_CLK_RECEIVER(){}
    void rcv_data(int v1, int v2) override;

private:
    TMRx_CLKCON *pt_clk;
};

void Tx_ZCD_CLK_RECEIVER::rcv_data(int v1, int v2)
{
        pt_clk->zcd_data_clk(v1, v2 & ~DATA_SERVER::SERV_MASK);
}

//#endif

TMRx_RST::TMRx_RST(TMR246_WITH_HLT *_tmrx_hlt, Processor *pCpu, const char *pName, const char *pDesc ) :
	sfr_register(pCpu, pName, pDesc), tmrx_hlt(_tmrx_hlt)
{
    mValidBits = 0x0f;
    pt_rst_receiver = new Tx_RST_RECEIVER(this, pName);
}
TMRx_RST::~TMRx_RST()
{
    delete pt_rst_receiver;
}

void TMRx_RST::put(unsigned int new_value)
{
    RST old_value = (RST)value.get();
    new_value &= mValidBits;
    Dprintf(("%s::put() new_value=0x%x\n", name().c_str(), new_value));
    RRprint((stderr, "%s::put() new_value=0x%x old_value=0x%x on=%d\n", name().c_str(), new_value, old_value, (bool)(tmrx_hlt->t246con.get_tmr2on())));
    trace.raw(write_trace.get() | value.get());
    value.put(new_value);
    if (!(old_value ^ new_value))
	return;

    switch(old_value)
    {
    case LC1_out:
        tmrx_hlt->get_cm_data_server(0)->detach_data(pt_rst_receiver);
        break;

    case LC2_out:
        tmrx_hlt->get_cm_data_server(1)->detach_data(pt_rst_receiver);
        break;

    case LC3_out:
        tmrx_hlt->get_cm_data_server(2)->detach_data(pt_rst_receiver);
        break;

    case LC4_out:
        tmrx_hlt->get_cm_data_server(3)->detach_data(pt_rst_receiver);
        break;

    case ZCD1_out:
        tmrx_hlt->get_zcd_data_server()->detach_data(pt_rst_receiver);
        break;

    case C1OUT_sync:
    case C2OUT_sync:
    case CCP1_out:
	tmrx_hlt->get_ccp_server(1)->detach_data(pt_rst_receiver);
	break;

    case CCP2_out:
	tmrx_hlt->get_ccp_server(2)->detach_data(pt_rst_receiver);
	break;

    case TMR2_postscale:
	tmrx_hlt->get_tmr246_server(2)->detach_data(pt_rst_receiver);
	break;

    case TMR4_postscale:
	tmrx_hlt->get_tmr246_server(4)->detach_data(pt_rst_receiver);
	break;

    case TMR6_postscale:
	tmrx_hlt->get_tmr246_server(6)->detach_data(pt_rst_receiver);
	break;

    case PWM3_out:
	tmrx_hlt->get_pwm_server(3)->detach_data(pt_rst_receiver);
	break;

    case PWM4_out:
	tmrx_hlt->get_pwm_server(4)->detach_data(pt_rst_receiver);
	break;

    default:
	break;
    }
    switch(new_value)
    {
    case LC1_out:
        tmrx_hlt->get_cm_data_server(0)->attach_data(pt_rst_receiver);
        break;

    case LC2_out:
        tmrx_hlt->get_cm_data_server(1)->attach_data(pt_rst_receiver);
        break;

    case LC3_out:
        tmrx_hlt->get_cm_data_server(2)->attach_data(pt_rst_receiver);
        break;

    case LC4_out:
        tmrx_hlt->get_cm_data_server(3)->attach_data(pt_rst_receiver);
        break;

    case ZCD1_out:
        tmrx_hlt->get_zcd_data_server()->attach_data(pt_rst_receiver);
	if (tmrx_hlt->get_zcd())
	    TMRx_ers_state = tmrx_hlt->get_zcd()->value.get();
        break;

    case CCP1_out:
	tmrx_hlt->get_ccp_server(1)->attach_data(pt_rst_receiver);
	break;

    case CCP2_out:
	tmrx_hlt->get_ccp_server(2)->attach_data(pt_rst_receiver);
	break;

    case TMR6_postscale:
	if (tmrx_hlt->tmr_number != '6')
        {
	    RRprint((stderr, "%s attach TMR6_postscale tmr_number=%c\n", name().c_str(), tmrx_hlt->tmr_number));
	    tmrx_hlt->get_tmr246_server(6)->attach_data(pt_rst_receiver);
	}
        else
	{
	    RRprint((stderr, "%s attach TMR6_postscale tmr_number=%c not allowed\n", name().c_str(), tmrx_hlt->tmr_number));
	}
	break;

    case TMR4_postscale:
	if (tmrx_hlt->tmr_number != '4')
        {
	    RRprint((stderr, "%s attach TMR4_postscale tmr_number=%c\n", name().c_str(), tmrx_hlt->tmr_number));
	    tmrx_hlt->get_tmr246_server(4)->attach_data(pt_rst_receiver);
	}
        else
	{
	    RRprint((stderr, "%s attach TMR4_postscale tmr_number=%c not allowed\n", name().c_str(), tmrx_hlt->tmr_number));
	}
	break;

    case TMR2_postscale:
	if (tmrx_hlt->tmr_number != '2')
        {
	    RRprint((stderr, "%s attach TMR2_postscale tmr_number=%c\n", name().c_str(), tmrx_hlt->tmr_number));
	    tmrx_hlt->get_tmr246_server(2)->attach_data(pt_rst_receiver);
	}
        else
	{
	    RRprint((stderr, "%s attach TMR2_postscale tmr_number=%c not allowed\n", name().c_str(), tmrx_hlt->tmr_number));
	}
	break;


    case PWM3_out:
	tmrx_hlt->get_pwm_server(3)->attach_data(pt_rst_receiver);
	break;

    case PWM4_out:
	tmrx_hlt->get_pwm_server(4)->attach_data(pt_rst_receiver);
	break;

    default:
	break;
    }
}
// Moniter TMRx_ers pin input
void TMRx_RST::TMRx_ers(bool state)
{
    unsigned int mode = tmrx_hlt->t246HLT.value.get() & 0x1f;


    RRprint((stderr, "TMRx_RST::TMRx_ers mode=%s state=%d->%d enabled=%d running=%d\n", std::bitset<5>(mode).to_string().c_str(), TMRx_ers_state, state,tmrx_hlt->t246con.tmr2->enabled, tmrx_hlt->t246con.tmr2->running));

    if (state == TMRx_ers_state)
	return;

    TMRx_ers_state = state;

    if ((mode == 0b00000) || (mode == 0b01000) || (mode == 0b10000) ||
	(mode == 0b10100) || (mode == 0b10101) || (mode >= 0b11000)
       )
    {
	return;
    }

    switch(mode)
    {
    case 0b00000:	// gate inactive
	action = NOP;
    	tmrx_hlt->tmr246.set_enable(true);
	break;

    case 0b00001:	//gate high level start low level stop
#ifdef RRR
	if (TMRx_ers_state && (bool)(tmrx_hlt->t246con.get_tmr2on()))
	if (TMRx_ers_state)
        {
	    action = START;
            set_delay();
	}
	// RRR FIXME Level needs to work if level set before on
	else if (!TMRx_ers_state && tmrx_hlt->t246con.tmr2->running)
        {
	    action = STOP;
	    set_delay();
	}
#endif
	action = TMRx_ers_state?START:STOP;
	set_delay();
	break;

    case 0b00010:	//gate low level start, high level stop
	action = TMRx_ers_state?STOP:START;
        set_delay();
	break;

    case 0b00011:	//gate any edge reset
	action = RESET;
        set_delay();
	break;

    case 0b00100:	//gate +edge reset
	if (TMRx_ers_state && tmrx_hlt->t246con.tmr2->running)
	{
	    action = RESET;
            set_delay();
	}
	break;

    case 0b00101:	//gate -edge reset
	if (!TMRx_ers_state)
	{
	    action = RESET;
            set_delay();
	}
	break;

    case 0b00110:	//gate low level reset or stop
	if (!TMRx_ers_state)
	{
	    action = (tmrx_hlt->t246con.tmr2->running)?STOP_ZERO:RESET;
            set_delay();
	}
	break;

    case 0b00111:	//gate high level reset or stop
	if (TMRx_ers_state)
	{
	    action = (tmrx_hlt->t246con.tmr2->running)?STOP_ZERO:RESET;
            set_delay();
	}
	else
	{
	    action = START;
            set_delay();
	}
	break;

    case 0b01001:	//gate +edge start
	if (TMRx_ers_state)
	{
	    action = START;
            set_delay();
	}
	break;

    case 0b01010:	//gate -edge start
	if (!TMRx_ers_state)
	{
	    action = START;
            set_delay();
	}
	break;

    case 0b01011:	//gate any edge start
	action = START;
        set_delay();
	break;

    case 0b01100:	//gate +edge start or reset
	if (TMRx_ers_state)
        {
	    if ((bool)tmrx_hlt->t246con.get_tmr2on())
	    {
	        action = tmrx_hlt->t246con.tmr2->enabled?RESET:START;
                set_delay();
	    }
	}
	break;

    case 0b01101:	//gate -edge start or reset
	if (!TMRx_ers_state)
        {
	    if ((bool)tmrx_hlt->t246con.get_tmr2on())
	    {
	        action = tmrx_hlt->t246con.tmr2->enabled?RESET:START;
                set_delay();
	    }
	}
	break;

    case 0b01110:	//gate +edge start, low level reset
	if(	(bool)tmrx_hlt->t246con.get_tmr2on() &&
		tmrx_hlt->t246con.tmr2->enabled == 0 &&
		TMRx_ers_state)
	{
	    action = START;
	    set_delay();
 	}
	else if ( !TMRx_ers_state)
	{
	    action = STOP_ZERO;
	    set_delay();
 	}
	break;

    case 0b01111:	//gate -edge start, high level reset
	if ((bool)tmrx_hlt->t246con.get_tmr2on() &&
		!tmrx_hlt->t246con.tmr2->enabled &&
		!TMRx_ers_state)
	{
	    action = START;
	    set_delay();
 	}
	else if (TMRx_ers_state)
	{
	    action = STOP;
	    set_delay();
 	}
	break;

    case 0b10001:	//gate +edge start monostable
	if (TMRx_ers_state && (bool)(tmrx_hlt->t246con.get_tmr2on()) &&
				(action != START))
	{
	    action = START;
	    set_delay();
        }
	break;

    case 0b10010:	//gate -edge start monostable
	if (!TMRx_ers_state && (bool)(tmrx_hlt->t246con.get_tmr2on()) &&
				(action != STOP))
	{
	    action = START;
	    set_delay();
        }
	break;

    case 0b10011:	//gate any edge start monostable
	//RRRif ((bool)(tmrx_hlt->t246con.get_tmr2on()) && (action == NOP))
	RRprint((stderr, "mode=19 action=%d on=%d\n", action, (bool)tmrx_hlt->t246con.get_tmr2on()));
	if ((bool)(tmrx_hlt->t246con.get_tmr2on() && action != START))
	{
	    action = START;
	    set_delay();
        }
	break;

   case 0b10110:	//+ level triggered hardware limit oneshot, - reset
	action = TMRx_ers_state?START:STOP_ZERO;
	set_delay();


	break;

   case 0b10111:	//- level triggered hardware limit oneshot, + reset
	action = !TMRx_ers_state?START:STOP_ZERO;
	set_delay();

	break;

    default:
	fprintf(stderr, "TMRx_RST::TMRx_ers unexpected mode=0x%x\n", mode);
	action = NOP;
	break;
    }
}
void TMRx_RST::set_delay()
{
        uint64_t fc = get_cycles().get() + 2;

        if (future_cycle)
            get_cycles().reassign_break(future_cycle, fc, this);
        else
            get_cycles().set_break(fc, this);

        future_cycle = fc;
}
void TMRx_RST::callback()
{
    future_cycle = 0;
#if DEBUG == 1
    unsigned int mode = tmrx_hlt->t246HLT.value.get() & 0x1f;
#endif

    RRprint((stderr, "TMRx_RST::callback() mode=0b%s now=%ld on=%d TMRx_ers=%d ", std::bitset<5>(mode).to_string().c_str(), get_cycles().get(), (bool)(tmrx_hlt->t246con.get_tmr2on()), TMRx_ers_state));

    switch(action)
    {
    case NOP:
	RRprint((stderr, "NOP\n"));
	break;

    case START:
	RRprint((stderr, " START\n"));
    	tmrx_hlt->tmr246.set_enable(true);
	break;

    case RESET:
	RRprint((stderr, " RESET\n"));
	tmrx_hlt->tmr246.reset_edge();
	break;

    case STOP:
	RRprint((stderr, " STOP\n"));
    	tmrx_hlt->tmr246.set_enable(false);
	break;

    case STOP_ZERO:
	RRprint((stderr, " STOP_ZERO\n"));
    	tmrx_hlt->tmr246.set_enable(false, true);
	break;

    }
    action = NOP;
}

void TMRx_RST::zcd_data_rst(bool v1, int cm)
{
    unsigned int rsel = value.get();

    RRprint((stderr, "%s zcd_data_rst(v1=%d cm=%d) tmron=%d rsel=%d\n", name().c_str(), v1, cm, (bool)(tmrx_hlt->t246con.get_tmr2on()), rsel));
    if (rsel == ZCD1_out)
        TMRx_ers(v1);
}
void TMRx_RST::clc_data_rst(bool v1, int cm)
{
#if DEBUG == 1
    unsigned int rsel = value.get();
    unsigned int mode = tmrx_hlt->t246HLT.value.get() & 0x1f;
#endif
    RRprint((stderr, "%s::clc_data_rst on=%d rsel=%d mode=%d v1=%d cm=%d\n", name().c_str(), tmrx_hlt->t246con.get_tmr2on(), rsel, mode, v1, cm));
    if (tmrx_hlt->t246con.get_tmr2on()) // do nothing unless tmr is on
    {
#ifdef RRR
        if ((cm == 0 && rsel != LC1_out) ||
	    (cm == 1 && rsel != LC2_out) ||
	    (cm == 2 && rsel != LC3_out) ||
	    (cm == 3 && rsel != LC4_out) )
	{
	    return;		// call not for active case
        }
#endif
    }
}

TMRx_HLT::TMRx_HLT(TMR246_WITH_HLT *_tmrx_hlt, Processor *pCpu, const char *pName, const char *pDesc ) :
	sfr_register(pCpu, pName, pDesc), tmrx_hlt(_tmrx_hlt)
{
	mValidBits = 0xff;
}
TMRx_HLT::~TMRx_HLT()
{
}

void TMRx_HLT::put(unsigned int new_value)
{
    unsigned int old_value = value.get();
    new_value &= mValidBits;
    Dprintf(("%s::put() new_value=0x%x\n", name().c_str(), new_value));
    RRprint((stderr, "%s::put() new_value=0x%x old_value=0x%x trm2on=%d\n", name().c_str(), new_value, old_value, tmrx_hlt->t246con.get_tmr2on()));
    trace.raw(write_trace.get() | value.get());
    value.put(new_value);
    if (!(old_value ^ new_value))
	return;
    tmrx_hlt->tmr_on(tmrx_hlt->t246con.get_tmr2on());
}



TMRx_CLKCON::TMRx_CLKCON(TMR246_WITH_HLT *_tmrx_hlt, Processor *pCpu, const char *pName, const char *pDesc ) :
	sfr_register(pCpu, pName, pDesc), tmrx_hlt(_tmrx_hlt)
{
    mValidBits = 0x0f;
    pt_clk_receiver = new Tx_CLK_RECEIVER(this, pName);
}
TMRx_CLKCON::~TMRx_CLKCON()
{
    delete pt_clk_receiver;
}

void TMRx_CLKCON::put(unsigned int new_value)
{
    unsigned int old_value = value.get();
    double clk_ratio = 1.0;
    new_value &= mValidBits;
    Dprintf(("%s::put() new_value=0x%x\n", name().c_str(), new_value));
    RRprint((stderr, "%s::put() new_value=0x%x old_value=0x%x on=%d\n", name().c_str(), new_value, old_value, (bool)(tmrx_hlt->t246con.get_tmr2on())));
    trace.raw(write_trace.get() | value.get());
    value.put(new_value);
    if (!(old_value ^ new_value))
	return;
    bool running = (bool)tmrx_hlt->t246con.get_tmr2on() && tmrx_hlt->tmr246.enabled;

    // remove old clock source
    switch(old_value)
    {
    case FOSC4:
	RRprint((stderr, "%s::put old FOSC4\n", name().c_str()));
    case FOSC:
    case HFINTOSC:
    case LFINTOSC:
    case MFINTOSC:
//	tmrx_hlt->tmr246.use_clk = false;
//	tmrx_hlt->tmr246.on_or_off(false);
	break;

    case T2INPPS:
        if (m_PinModule)
	{
	    if (sink_active)
	        m_PinModule->removeSink(this);
	    m_PinModule->getPin()->newGUIname("");
	}
	break;
    case LC1_out:
	RRprint((stderr, "TMRx_CLKCON::put stop LC1_out\n"));
        tmrx_hlt->get_cm_data_server(0)->detach_data(pt_clk_receiver);
        break;

    case LC2_out:
        tmrx_hlt->get_cm_data_server(1)->detach_data(pt_clk_receiver);
        break;

    case LC3_out:
        tmrx_hlt->get_cm_data_server(2)->detach_data(pt_clk_receiver);
        break;

    case LC4_out:
        tmrx_hlt->get_cm_data_server(3)->detach_data(pt_clk_receiver);
        break;

    case ZCD1_out:
        tmrx_hlt->get_zcd_data_server()->detach_data(pt_clk_receiver);
        break;

    case AT1_PERCLK:
	RRprint((stderr, "TMRx_CLKCON::put stop AT1_PERCLK\n"));
        tmrx_hlt->get_at1_data_server()->detach_data(pt_clk_receiver);
        break;

    default:
	break;
    }
    tmrx_hlt->tmr246.set_clk_ratio(1.0);
    switch((CLK_SRC)new_value)
    {
    case FOSC4:
        tmrx_hlt->tmr246.set_clk_ratio(1.0);
	RRprint((stderr, "\t%s::put FOSC4 clk_ratio=%.2f\n", name().c_str(), tmrx_hlt->tmr246.clk_ratio));
	tmrx_hlt->tmr246.use_clk = true;
	tmrx_hlt->tmr246.on_or_off(running);
	break;

    case FOSC:
        tmrx_hlt->tmr246.set_clk_ratio(0.25);
	RRprint((stderr, "\t%s::put FOSC clk_ratio=%.2f on=%d enabled=%d\n", name().c_str(), tmrx_hlt->tmr246.clk_ratio, (bool)tmrx_hlt->t246con.get_tmr2on(), tmrx_hlt->tmr246.enabled));
	tmrx_hlt->tmr246.use_clk = true;
	tmrx_hlt->tmr246.on_or_off(running);
	break;

    case HFINTOSC:
	clk_ratio = get_cycles().instruction_cps() / 16000000;
	tmrx_hlt->tmr246.set_clk_ratio(clk_ratio);
	RRprint((stderr, "\t%s::put HFINTOSC clk_ratio=%.3f instruct_cps=%.1f\n", name().c_str(), tmrx_hlt->tmr246.clk_ratio, get_cycles().instruction_cps()));
	tmrx_hlt->tmr246.use_clk = true;
	tmrx_hlt->tmr246.on_or_off(running);
	break;

    case MFINTOSC:
	clk_ratio = get_cycles().instruction_cps() / 31250;
	tmrx_hlt->tmr246.set_clk_ratio(clk_ratio);
	RRprint((stderr, "\t%s::put MFINTOSC clk_ratio=%.3f instruct_cps=%.1f\n", name().c_str(), clk_ratio, get_cycles().instruction_cps()));
	tmrx_hlt->tmr246.use_clk = true;
	tmrx_hlt->tmr246.on_or_off(running);
	break;

    case LFINTOSC:
	clk_ratio = get_cycles().instruction_cps() / 31000;
	tmrx_hlt->tmr246.set_clk_ratio(clk_ratio);
	RRprint((stderr, "\t%s::put LFINTOSC clk_ratio=%.3f instruct_cps=%.1f\n", name().c_str(), clk_ratio, get_cycles().instruction_cps()));
	tmrx_hlt->tmr246.use_clk = true;
	tmrx_hlt->tmr246.on_or_off(running);
	break;

    case T2INPPS:

	RRprint((stderr, "m_PinModule=%s sink_active=%d\n", m_PinModule->getPin()->name().c_str(), sink_active));
        if (m_PinModule)
	{
	    std::string pin_name = name().substr(0, 2) + "in";
	    m_PinModule->addSink(this);
	    sink_active = true;
	    m_PinModule->getPin()->newGUIname(pin_name.c_str());
	    last_state = m_PinModule->getPin()->getState();
	}
	tmrx_hlt->tmr246.use_clk = false;
	break;

    case LC1_out:
	RRprint((stderr, "TMRx_CLKCON::put start LC1_out\n"));
        tmrx_hlt->get_cm_data_server(0)->attach_data(pt_clk_receiver);
	tmrx_hlt->tmr246.use_clk = false;
        break;

    case LC2_out:
        tmrx_hlt->get_cm_data_server(1)->attach_data(pt_clk_receiver);
	tmrx_hlt->tmr246.use_clk = false;
        break;

    case LC3_out:
        tmrx_hlt->get_cm_data_server(2)->attach_data(pt_clk_receiver);
	tmrx_hlt->tmr246.use_clk = false;
        break;

    case LC4_out:
        tmrx_hlt->get_cm_data_server(3)->attach_data(pt_clk_receiver);
	tmrx_hlt->tmr246.use_clk = false;
        break;

    case ZCD1_out:
        tmrx_hlt->get_zcd_data_server()->attach_data(pt_clk_receiver);
	tmrx_hlt->tmr246.use_clk = false;
        break;

    case AT1_PERCLK:
	RRprint((stderr, "TMRx_CLKCON::put start AT1_PERCLK\n"));
        tmrx_hlt->get_at1_data_server()->attach_data(pt_clk_receiver);
	tmrx_hlt->tmr246.use_clk = false;
        break;

    default:
	break;
    }
}

void TMRx_CLKCON::setIOpin(PinModule *pin, int arg)
{
    if (value.get() == T2INPPS && m_PinModule)
    {
	if (sink_active)
	    m_PinModule->removeSink(this);
	pin->addSink(this);
	sink_active = true;
    }
    m_PinModule = pin;
}


void TMRx_CLKCON::setSinkState(char new3State)
{
    unsigned int current_state = new3State == '1' || new3State == 'W';
    if (current_state != last_state)
    {
        bool active_edge = tmrx_hlt->t246HLT.get_ckpol();
        RRprint((stderr, "\tTMRx_CLKCON::setSinkState current_state=%d active_edge=%d edge=%d\n", current_state, active_edge, current_state^active_edge));
	last_state = current_state;
     if (last_state^active_edge)
	tmrx_hlt->tmr246.increment();
    }
}
void TMRx_CLKCON::zcd_data_clk(bool v1, int cm)
{
    RRprint((stderr, "%s::zcd_data_clk v1=%d chpol=%d cm=%d\n", name().c_str(), v1, get_ckpol(), cm));


    if (v1 == old_data_clk)
	return;
    old_data_clk = v1;
    if (v1 ^ get_ckpol())
    {
	RRprint((stderr, "%s::zcd_data_clk increment\n", name().c_str()));
	tmrx_hlt->tmr246.increment();
    }
}
void TMRx_CLKCON::at1_data_clk(bool v1, int cm)
{
    RRprint((stderr, "%s::at1_data_clk v1=%d chpol=%d cm=%d\n", name().c_str(), v1, get_ckpol(), cm));


    if (v1 == old_data_clk)
	return;
    old_data_clk = v1;
    if (v1 ^ get_ckpol())
    {
	RRprint((stderr, "%s::at1_data_clk increment\n", name().c_str()));
	tmrx_hlt->tmr246.increment();
    }
}
void TMRx_CLKCON::clc_data_clk(bool v1, int cm)
{
    int clk_select = value.get() - LC1_out;

    RRprint((stderr, "%s::clc_data_clk cm=%d clk_select=%d\n", name().c_str(), cm, clk_select));
    if ((cm != clk_select) || (v1 == old_data_clk))
	 return;

    RRprint((stderr, "\t%s::clc_data_clk v1=%d chpol=%d cm=%d\n", name().c_str(), v1, get_ckpol(), cm ));
    old_data_clk = v1;
    if (v1 ^ get_ckpol())
    {
	RRprint((stderr, "%s::clc_data_clk increment\n", name().c_str()));
	tmrx_hlt->tmr246.increment();
    }
}

bool TMRx_CLKCON::get_ckpol()
{
    return tmrx_hlt->t246HLT.get_ckpol();
}

TMR246_WITH_HLT::TMR246_WITH_HLT(Processor *_pCpu, char _timer) :
    pr246(_pCpu, "pr2", "Module Period Register"),
    t246con(_pCpu, "t2con", "Timer Control Register", this),
    tmr246(_pCpu, "tmr2", "TMR246 Register"),
    t246HLT(this, _pCpu, "t2hlt", "TIMERx Hardware Limit Control Register"),
    t246CLKCON(this, _pCpu, "t2clkcon", "TIMERx Clock Selection Register"),
    t246RST(this, _pCpu, "t2rst", "TIMERX External Reset Signal Selection Register"),
    tmr_number(_timer), pCpu(_pCpu)
{
    t246con.tmr2 = &tmr246;
    tmr246.pr2 = &pr246;
    pr246.tmr2 = &tmr246;
    tmr246.t2con = &t246con;
    tmr246.tx_hlt = &t246HLT;
    tmr246.tmrx_clkcon = &t246CLKCON;
    std::fill_n(duty_cycle, MAX_PWM_CHANS, 0);
    std::fill_n(m_clc, 4, nullptr);
    std::fill_n(m_ccp, MAX_PWM_CHANS, nullptr);
    std::fill_n(m_tmr246, 3, nullptr);
    std::fill_n(m_pwm, 4, nullptr);
}
TMR246_WITH_HLT::~TMR246_WITH_HLT()
{
}

void TMR246_WITH_HLT::new_pre_post_scale()
{
#ifdef RRR
    pre_scale = t246con.get_pre_scale();
    post_scale = t246con.get_post_scale();
    pr2_buf = pr246.value.get();
    RRprint((stderr, "TMR246_WITH_HLT::new_pre_post_scale pre_scale=%d post_scale=%d pr2_buf=%d\n", pre_scale, post_scale, pr2_buf));
#else
    t246con.tmr2->new_pre_post_scale();
#endif
}
void TMR246_WITH_HLT::tmr_on(bool is_on)
{
    unsigned int txhlt_mode  = t246HLT.value.get() & TMRx_HLT::MODE40;

    RRprint((stderr, "TMR246_WITH_HLT::tmr_on %s is_on=%d txHLT=0x%x TMRx_ers=%d\n", tmr246.name().c_str(), is_on, t246HLT.value.get(), t246RST.get_TMRx_ers()));

    // can start regardless of gate state
    if ((txhlt_mode == 0b00000) || (txhlt_mode == 0b00011) ||
	(txhlt_mode == 0b00100) || (txhlt_mode == 0b00101) ||
	(txhlt_mode == 0b00111) || (txhlt_mode == 0b01000)
       )
    {
	//tmr246.set_enable(t246con.value.get() & TMR2ON);
	tmr246.set_enable(true);
	tmr246.on_or_off(is_on);
    }
    // Can start if gate is high
    else if ( (txhlt_mode == 0b00001) || (txhlt_mode == 0b10110) )
    {
	if (t246RST.get_TMRx_ers())
	    tmr246.set_enable(t246con.value.get() & TMR2ON);
	else
	    tmr246.set_enable(false);
	tmr246.on_or_off(is_on);
	RRprint((stderr, "TMR246_WITH_HLT::tmr_on is_on=%d TMRx_ers=%d enabled=%d\n", is_on, t246RST.get_TMRx_ers(), tmr246.enabled));
    }
    // Can start if gate is low
    else if ( (txhlt_mode == 0b00010) || (txhlt_mode == 0b10111) )
    {
	if (!t246RST.get_TMRx_ers())
	    tmr246.set_enable(t246con.value.get() & TMR2ON);
	else
	    tmr246.set_enable(false);
	tmr246.on_or_off(is_on);
    }
    // edge trigger
    else
    {
 	RRprint((stderr, "\tTMR246_WITH_HLT::tmr_on line=%d txhlt_mode=%s is_on=%d TMRx_ers=%d enabled=%d\n", __LINE__, std::bitset<5>(txhlt_mode).to_string().c_str(), is_on, t246RST.get_TMRx_ers(), tmr246.enabled));
        tmr246.set_enable(false);
        tmr246.on_or_off(is_on);


    }
    // else timer does not start until proper gate edge or state
}

void TMR246_WITH_HLT::set_clc(CLC_BASE *clc1, CLC_BASE *clc2,
                        CLC_BASE *clc3, CLC_BASE *clc4)
{
    m_clc[0] = clc1;
    m_clc[1] = clc2;
    m_clc[2] = clc3;
    m_clc[3] = clc4;
}

void TMR246_WITH_HLT::set_pt_pwm(PWMxCON *pt1, PWMxCON *pt2, PWMxCON *pt3, PWMxCON *pt4)
{
    m_pwm[0] = pt1;
    m_pwm[1] = pt2;
    m_pwm[2] = pt3;
    m_pwm[3] = pt4;
}
void TMR246_WITH_HLT::set_m_ccp(CCPCON *p1, CCPCON *p2, CCPCON *p3, CCPCON *p4, CCPCON *p5)
{
    m_ccp[0] = p1;
    m_ccp[1] = p2;
    m_ccp[2] = p3;
    m_ccp[3] = p4;
    m_ccp[4] = p5;
}

void TMR246_WITH_HLT::set_tmr246(TMR246_WITH_HLT *t2,
			         TMR246_WITH_HLT *t4,
				  TMR246_WITH_HLT *t6)
{
    m_tmr246[0] = t2;
    m_tmr246[1] = t4;
    m_tmr246[2] = t6;
}


DATA_SERVER * TMR246_WITH_HLT::get_cm_data_server(unsigned int cm)
{
        if (!m_clc[cm])
    {
        fprintf(stderr, "***ERROR TMR246_WITH_HLT::get_cm_data_server m_clc[%u] not defined\n", cm);
        assert(m_clc[cm]);
    }
    return m_clc[cm]->get_CLC_data_server();
}
DATA_SERVER * TMR246_WITH_HLT::get_zcd_data_server()
{
        if (!m_zcd)
    {
        fprintf(stderr, "***ERROR TMR246_WITH_HLT::get_zcd_data_server() m_zcd not defined\n");
        assert(m_zcd);
    }
    return m_zcd->get_zcd_data_server();
}
DATA_SERVER * TMR246_WITH_HLT::get_at1_data_server()
{
        if (!m_at1)
    {
        fprintf(stderr, "***ERROR TMR246_WITH_HLT::get_at1_data_server() m_at1 not defined\n");
        assert(m_at1);
    }
    return m_at1->get_atx_data_server();
}
// return data server from T2, T4 or T6
DATA_SERVER * TMR246_WITH_HLT::get_tmr246_server(int t_number)
{
    int index = (t_number /2 -1); //2,4,6->0,1,2
    if (index < 0 || index > 2 || !m_tmr246[index])
    {
        fprintf(stderr, "***ERROR TMR246_WITH_HLT::get_tmr246_server(%d) not defined for T%c\n", t_number, tmr_number);
        assert(false);
    }
    return m_tmr246[index]->tmr246.get_tmr246_server();
}
DATA_SERVER * TMR246_WITH_HLT::get_pwm_server(int _index)
{
    int index = _index - 1;
    if (index < 0 || index > 4 || !m_pwm[index])
    {
        fprintf(stderr, "***ERROR TMR246_WITH_HLT::get_pwm_server(%d) not defined for T%c\n", _index, tmr_number);
        assert(false);
    }
    return m_pwm[index]->get_pwmx_server();
}
DATA_SERVER * TMR246_WITH_HLT::get_ccp_server(int _index)
{
    int index = _index - 1;
    if (index < 0 || index > MAX_PWM_CHANS || !m_ccp[index])
    {
        fprintf(stderr, "***ERROR TMR246_WITH_HLT::get_ccp_server(%d) not defined for T%c\n", _index, tmr_number);
	assert(false);
    }
    return m_ccp[index]->get_ccp_server();
}
#ifdef RRR
void TMR246_WITH_HLT::attach_clc_receiver(unsigned int cm, DATA_RECEIVER *pt_dr)
{
    if (!m_clc[cm])
    {
	fprintf(stderr, "***ERROR TMR246_WITH_HLT::attach_clc_receiver m_clc[%u] not defined\n", cm);
	assert(m_clc[cm]);
    }
    m_clc[cm]->get_CLC_data_server()->attach_data(pt_dr);
}
void TMR246_WITH_HLT::detach_clc_receiver(unsigned int cm, DATA_RECEIVER *pt_dr)
{
	m_clc[cm]->get_CLC_data_server()->detach_data(pt_dr);
}
void TMR246_WITH_HLT::attach_clc_rst_receiver(unsigned int cm)
{
    RRprint((stderr, "TMR246_WITH_HLT::attach_clc_rst_receiver cm=%d pt_clc_rst_receiver=%p %p\n", cm, pt_clc_rst_receiver, m_clc[cm]));//->get_CLC_data_server());
	m_clc[cm]->get_CLC_data_server()->attach_data(pt_clc_rst_receiver);
}
void TMR246_WITH_HLT::detach_clc_rst_receiver(unsigned int cm)
{
	m_clc[cm]->get_CLC_data_server()->detach_data(pt_clc_rst_receiver);
}
void TMR246_WITH_HLT::attach_clc_clk_receiver(unsigned int cm)
{
    RRprint((stderr, "TMR246_WITH_HLT::attach_clc_clk_receiver cm=%d pt_clc_data_receiver=%p %p\n", cm, pt_clc_clk_receiver, m_clc[cm]));//->get_CLC_data_server());
        t246HLT.old_data_clk = -1;
	tmr246.use_clk = false;
	m_clc[cm]->get_CLC_data_server()->attach_data(pt_clk_receiver);
}
void TMR246_WITH_HLT::detach_clc_clk_receiver(unsigned int cm)
{
	m_clc[cm]->get_CLC_data_server()->detach_data(pt_clk_receiver);
}
#endif
//--------------------------------------------------
//
//--------------------------------------------------

class INT_SignalSink : public SignalSink
{
public:
    INT_SignalSink(ECCPAS *_eccpas, int _index)
        : m_eccpas(_eccpas), m_index(_index)
    {
        assert(_eccpas);
    }

    virtual void release() override
    {
        delete this;
    }
    void setSinkState(char new3State) override
    {
        m_eccpas->set_trig_state(m_index, new3State == '0' || new3State == 'w');
    }

private:
    ECCPAS *m_eccpas;
    int     m_index;
};


//--------------------------------------------------
// ECCPAS
//--------------------------------------------------
ECCPAS::ECCPAS(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc)
{
    trig_state[0] = trig_state[1] = trig_state[2] = false;
    mValidBits = 0xff;
}


ECCPAS::~ECCPAS()
{
}


void ECCPAS::link_registers(PWM1CON *_pwm1con, CCPCON *_ccp1con)
{
    pwm1con = _pwm1con;
    ccp1con = _ccp1con;
}


void ECCPAS::put(unsigned int new_value)
{
    Dprintf(("ECCPAS::put() new_value=0x%x\n", new_value));
    trace.raw(write_trace.get() | value.get());
    put_value(new_value);
}


void ECCPAS::put_value(unsigned int new_value)
{
    int old_value = value.get();
    new_value &= mValidBits;


    // Auto-shutdown trigger active
    //   make sure ECCPASE is set
    //   if change in shutdown status, drive bridge outputs as per current flags
    if (shutdown_trigger(new_value))
    {
        new_value |= ECCPASE;

        if ((new_value ^ old_value) & (ECCPASE | PSSAC1 | PSSAC0 | PSSBD1 | PSSBD0))
        {
            ccp1con->shutdown_bridge(new_value);
        }

    }
    else     // no Auto-shutdown triggers active
    {
        if (pwm1con->value.get() & PWM1CON::PRSEN)   // clear ECCPASE bit
        {
            new_value &= ~ ECCPASE;
        }
    }

    value.put(new_value);
}


// Return true is shutdown trigger is active
bool ECCPAS::shutdown_trigger(int key)
{
    RRprint((stderr, "ECCPAS::shutdown_trigger key=0x%x\n", key));
    for(int i=0; i<3; i++)
	RRprint((stderr, "ECCPAS::shutdown_trigger %d trig_state=0x%x\n",i, trig_state[i]));
    if ((key & ECCPAS0) && trig_state[0])
    {
        return true;
    }

    if ((key & ECCPAS1) && trig_state[1])
    {
        return true;
    }

    if ((key & ECCPAS2) && trig_state[2])
    {
        return true;
    }

    return false;
}


// connect IO pins to shutdown trigger source
void ECCPAS::setIOpin(PinModule *p0, PinModule *p1, PinModule *p2)
{
    if (p0)
    {
        m_PinModule = p0;
        m_sink = new INT_SignalSink(this, 0);
        p0->addSink(m_sink);
    }

    if (p1)
    {
        m_PinModule = p1;
        m_sink = new INT_SignalSink(this, 1);
        p1->addSink(m_sink);
    }

    if (p2)
    {
        m_PinModule = p2;
        m_sink = new INT_SignalSink(this, 2);
        p2->addSink(m_sink);
    }
}


// set shutdown trigger states
void ECCPAS::set_trig_state(int index, bool state)
{
    if (trig_state[index] != state)
    {
        Dprintf(("index=%d state=%d old=%d\n", index, state, trig_state[index]));
        trig_state[index] = state;
        put_value(value.get());
    }
}


// Trigger state from comparator 1
void ECCPAS::c1_output(int state)
{
    set_trig_state(0, state);
}


// Trigger state from comparator 2
void ECCPAS::c2_output(int state)
{
    set_trig_state(1, state);
}


//--------------------------------------------------
// PWM1CON
//--------------------------------------------------
PWM1CON::PWM1CON(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc)
{
    mValidBits = 0xff;
}


PWM1CON::~PWM1CON()
{
}


void PWM1CON::put(unsigned int new_value)
{
    new_value &= mValidBits;
    Dprintf(("PWM1CON::put() new_value=0x%x\n", new_value));
    trace.raw(write_trace.get() | value.get());
    value.put(new_value);
}


//--------------------------------------------------
// PSTRCON
//--------------------------------------------------
PSTRCON::PSTRCON(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc)
{
}


PSTRCON::~PSTRCON()
{
}


void PSTRCON::put(unsigned int new_value)
{
    Dprintf(("PSTRCON::put() new_value=0x%x\n", new_value));
    new_value &= STRSYNC | STRD | STRC | STRB | STRA;
    trace.raw(write_trace.get() | value.get());
    value.put(new_value);
}


//--------------------------------------------------------------------
//	PWM TIMER SELECTION CONTROL REGISTER
//--------------------------------------------------------------------
CCPTMRS14::CCPTMRS14(Processor *pCpu, const char *pName, const char *pDesc):
    sfr_register(pCpu, pName, pDesc)
{
    std::fill_n(ccp, 4, nullptr);
}


void CCPTMRS14::put(unsigned int new_value)
{
    TMR2 *tx;
    trace.raw(write_trace.get() | value.get());
    value.put(new_value);

    for (int i = 0; i < 4; i++)
    {
        switch (new_value & 0x3)
        {
        case 0:
            tx = t2;
            break;

        case 1:
            tx = t4;
            break;

        case 2:
            tx = t6;
            break;

        default:
            tx = 0;
            break;
        }

    Dprintf(("CCPTMRS14::put i=%d ccp[i]=%p %s tx=%p %s\n", i, ccp[i], ccp[i]?ccp[i]->name().c_str() : "null", tx, tx->name().c_str()));
        if (ccp[i] && tx)
        {
            ccp[i]->set_tmr2(tx);
            tx->add_ccp(ccp[i]);
        }

        new_value >>= 2;
    }
}
