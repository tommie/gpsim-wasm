/*
   Copyright (C) 2019 Roy R. Rankin
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

// Zero Crossing Detector (ZCD) MODULE


//#define DEBUG
#if defined(DEBUG)
#include <config.h>
#define Dprintf(arg) {printf("%s:%d ",__FILE__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

#include "zcd.h"
#include <assert.h>
#include <algorithm>
#include <string>

#include "ioports.h"
#include "pic-processor.h"
#include "pir.h"
#include "processor.h"
#include "stimuli.h"
#include "trace.h"
#include "clc.h"

// Set ZCDCON pin drive level high
class ZCDSignalSource : public SignalControl
{
public:
    explicit ZCDSignalSource(ZCDCON *_zcdcon)
        : m_zcdcon(_zcdcon), state('1')
    {
        assert(m_zcdcon);
    }
    ~ZCDSignalSource() { }
    char getState() override
    {
        return state;
    }
    void putState(bool high)
    {
	state = high ? '1' : '0';
    }
    void release() override
    {
        m_zcdcon->releasePin();
    }
private:
    ZCDCON *m_zcdcon;
    char    state;
};

// set ZCDCON pin direction
class ZCDSignalControl : public SignalControl
{
public:
    explicit ZCDSignalControl(ZCDCON *_zcdcon)
        : m_zcdcon(_zcdcon)
    { }
    ~ZCDSignalControl() { }
    char getState() override { return '0'; }
    void release() override
    {
        m_zcdcon->releasePin();
    }
private:
    ZCDCON *m_zcdcon;
};

class ZCDPinMonitor : public PinMonitor
{
public:
    explicit ZCDPinMonitor(ZCDCON *_zcdcon) : state(UNKNOWN), m_zcdcon(_zcdcon) { }
    ~ZCDPinMonitor() {}

    void setDrivenState(char) override {}
    void setDrivingState(char) override {}
    void set_nodeVoltage(double) override;
    void putState(char) override {}
    void setDirection() override {}

private:
    enum State
    {
        LOW = 0,
        HIGH = 1,
        UNKNOWN = 2
    } state;
    ZCDCON *m_zcdcon;
};

void ZCDPinMonitor::set_nodeVoltage(double volt)
{
    State new_state;
    new_state =  (volt >= 0.75) ? HIGH : LOW;

    if (new_state != state)
    {
        state = new_state;
        m_zcdcon->new_state(state);
    }
}



ZCDCON::ZCDCON(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc), con_mask(0x93),
      m_Interrupt(nullptr), m_control(nullptr), m_monitor(nullptr),
      m_source(nullptr), m_out_source(nullptr), m_SaveMonitor(nullptr),
      save_Vth(0.0)
{
    zcd_data_server = new DATA_SERVER(DATA_SERVER::ZCD);
    std::fill_n(m_PinMod, 2, nullptr);
}

ZCDCON::~ZCDCON()
{
    if (m_monitor)
    {
        delete m_monitor;
        delete m_Interrupt;
        delete m_source;
    }
    if (m_out_source)
	delete m_out_source;

    delete zcd_data_server;
}

void ZCDCON::put(unsigned int new_value)
{
    unsigned int diff = (new_value ^ value.get()) & con_mask;
    bool zcddis = cpu_pic->get_zcddis();  // from config2 false enables zcd.

    if (!diff && zcddis)
    {
        return;
    }
    trace.raw(write_trace.get() | value.get());
    value.put((new_value & con_mask) | (value.get() & ~con_mask));

    if (diff & ZCDxPOL)
        m_Interrupt->Trigger();

    if ((diff & ZCDxEN) || !zcddis)	// enable change
    {
        if ((new_value & ZCDxEN) || !zcddis)	// enabling module
        {
            if (!m_SaveMonitor)
                m_SaveMonitor = m_PinMod[0]->getPin()->getMonitor();
            if (!m_monitor)
            {
                m_monitor = new ZCDPinMonitor(this);
                m_control = new ZCDSignalControl(this);
                m_source = new ZCDSignalSource(this);
                m_out_source = new ZCDSignalSource(this);
            }
            m_PinMod[0]->AnalogReq(this, true,"ZCD");
            m_PinMod[0]->getPin()->setMonitor(0);
            m_PinMod[0]->getPin()->setMonitor(m_monitor);
            m_PinMod[0]->setSource(m_source);
            m_PinMod[0]->setControl(m_control);
            m_PinMod[0]->getPin()->newGUIname("ZCD");
            save_Vth = m_PinMod[0]->getPin()->get_Vth();
            m_PinMod[0]->getPin()->set_Vth(0.75);
            m_PinMod[0]->updatePinModule();
	    if (m_PinMod[1])
	    {
		m_PinMod[1]->getPin()->newGUIname("ZCDout");
		m_PinMod[1]->setSource(m_out_source);
		m_PinMod[1]->updatePinModule();
	    }

            Dprintf(("%s enable ZCD %f\n", name().c_str(), m_PinMod[0]->getPin()->get_nodeVoltage()));

        }
        else				// disabling module
        {
            close_module();
            return;
        }
    }
    if (diff & (ZCDxINTN | ZCDxINTP | ZCDxPOL))
        new_state(m_PinMod[0]->getPin()->get_nodeVoltage() >= 0.75);

}

void ZCDCON::close_module()
{
    if (m_monitor && m_PinMod[0])
    {
        m_PinMod[0]->getPin()->setMonitor(0);
        m_PinMod[0]->getPin()->setMonitor(m_SaveMonitor);
        m_PinMod[0]->getPin()->set_Vth(save_Vth);
        m_PinMod[0]->setSource(0);
        m_PinMod[0]->setControl(0);
        m_PinMod[0]->AnalogReq(this, false, m_PinMod[0]->getPin()->name().c_str());
        m_PinMod[0]->updatePinModule();
    }
    if (m_PinMod[1])
    {
	m_PinMod[1]->setSource(0);
	m_PinMod[1]->getPin()->newGUIname(m_PinMod[1]->getPin()->name().c_str());
    }
}

void ZCDCON::new_state(bool state)
{
    unsigned int zcdcon_value = value.get();
    bool invert = (zcdcon_value & ZCDxPOL);
    Dprintf(("ZCDCON::new_state zcdcon=0x%x state=%d polarity=%d new_state=%d\n", zcdcon_value, state, invert, state ^ invert));
    if (!(zcdcon_value & ZCDxEN))	// module not enabled
        return;

    state ^= invert;

    if (state)
    {
        zcdcon_value |= ZCDxOUT;
	if (m_PinMod[1])
	{
	    m_out_source->putState(true);
	    m_PinMod[1]->updatePinModule();
	}
        if (zcdcon_value & ZCDxINTP)
        {
            m_Interrupt->Trigger();
        }
    }
    else
    {
	if (m_PinMod[1])
	{
	    m_out_source->putState(false);
	    m_PinMod[1]->updatePinModule();
	}
        zcdcon_value &= ~ZCDxOUT;
        if (zcdcon_value & ZCDxINTN)
        {
            m_Interrupt->Trigger();
        }
    }
    value.put(zcdcon_value);
    zcd_data_server->send_data(zcdcon_value & ZCDxOUT, 0);
}

void ZCDCON::setIOpin(PinModule *_pin, int arg)
{
    m_PinMod[arg] =_pin;
}
void ZCDCON::releasePin()
{
    if (m_PinMod[0])
    {
//	m_PinMod->setSource(0);
//	m_PinMod->setControl(0);
        //m_PinMod = nullptr;
    }
}
