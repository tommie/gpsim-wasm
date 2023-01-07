/*
   Copyright (C) 2017,2022 Roy R Rankin

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

// CONFIGURABLE LOGIC CELL (CLC)

#include "../config.h"

#define DEBUG  0
#if DEBUG == 1
#define RRprint(arg) {fprintf arg;}
#else
#define RRprint(arg) {}
#endif
#if DEBUG == 2
#define Dprintf(arg) {printf("%s:%d ",__FILE__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

#include <assert.h>
#include <stdio.h>

#include "gpsim_time.h"
#include "clc.h"
#include "nco.h"
#include "cwg.h"
#include "zcd.h"
#include "at.h"
#include "pir.h"
#include "stimuli.h"
#include "trace.h"
#include "14bit-tmrs.h"

// Report state changes on incoming INx pins
class INxSignalSink: public SignalSink
{
public:
    INxSignalSink(CLC_BASE * _clc, int _index)
        : m_clc(_clc), index(_index)
    {
    }

    void setSinkState(char new3State) override
    {
        m_clc->setState(new3State, index);
    }
    void release() override
    {
        delete this;
    }

private:
    CLC_BASE * m_clc;
    int index;
};


class CLCSigSource: public SignalControl
{
public:
    CLCSigSource(CLC_BASE * _clc, PinModule * _pin)
        : m_clc(_clc), m_pin(_pin), m_state('?')
    {
        assert(m_clc);
    }
    virtual ~CLCSigSource()
    {
    }

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
        m_clc->releasePinSource(m_pin);
    }

private:
    CLC_BASE *m_clc;
    PinModule *m_pin;
    char m_state;
};

class CLC_DATA_RECEIVER : public DATA_RECEIVER
{
public:
    explicit CLC_DATA_RECEIVER(CLC_BASE *pt, const char *_name) :
        DATA_RECEIVER(_name), pt_clc(pt)
    {}
    virtual ~CLC_DATA_RECEIVER(){}
    void rcv_data(int v1, int v2) override;

private:
    CLC_BASE *pt_clc;
};

void CLC_DATA_RECEIVER::rcv_data(int v1, int v2)
{
    int module = v2 & DATA_SERVER::SERV_MASK;
    v2 &= ~DATA_SERVER::SERV_MASK;
    switch(module)
    {
    case DATA_SERVER::CLC:
        pt_clc->clc_lcxupdate(v1, v2);
	break;

    case DATA_SERVER::ZCD:
        pt_clc->ZCDx_out(v1);
	break;

    case DATA_SERVER::AT1:
	pt_clc->ATx_out(v1, v2);
	break;

    case DATA_SERVER::TMR1:
	pt_clc->t135_overflow(v2);
	break;

    case DATA_SERVER::TMR2:
	if ((v2 & TMR2::MASK) == TMR2::MATCH)
	    pt_clc->t246_match(v2 & ~TMR2::MATCH);
	break;

    default:
	fprintf(stderr, "DATA_SERVER unexpected type 0x%x\n", module);
	break;
    };
}


void CLCxCON::put(unsigned int new_value)
{
    new_value &= write_mask;
    new_value |= (value.get() & read_only);
    unsigned int diff = new_value ^ value.get();
    trace.raw(write_trace.get() | value.get());
    value.put(new_value);


    if (!diff)
    {
        return;
    }

    m_clc->update_clccon(diff);
}


void CLCxPOL::put(unsigned int new_value)
{
    new_value &= write_mask;
    unsigned int diff = new_value ^ value.get();
    trace.raw(write_trace.get() | value.get());
    value.put(new_value);

    if (!diff)
    {
        return;
    }

    m_clc->compute_gates();
}


// used when CLC has 4 data select registers (CLCxSEL0-3)
// reg_number in range 0-3 to indicate which register
CLCxSELx::CLCxSELx(CLC_4SEL * _clc, Processor * pCpu, const char *pName,
                   const char *pDesc, unsigned int _reg_number)
    : sfr_register(pCpu, pName, pDesc), m_clc(_clc), write_mask(0x1f),
      reg_number(_reg_number)
{
}

void CLCxSELx::put(unsigned int new_value)
{
    Dprintf(("CLCxSELx::put ( %02X ) on SEL%d\n", new_value, reg_number ));
    new_value &= write_mask;
    trace.raw(write_trace.get() | value.get());
    unsigned int diff = new_value ^ value.get();
    value.put(new_value);

    if (diff)
    {
        switch (reg_number)
        {
        case 0:
            m_clc->D1S(new_value);
            break;

        case 1:
            m_clc->D2S(new_value);
            break;

        case 2:
            m_clc->D3S(new_value);
            break;

        case 3:
            m_clc->D4S(new_value);
            break;
        }

        if (m_clc->CLCenabled())
        {
            m_clc->config_inputs(true);
        }
    }
}

CLCxSEL0::CLCxSEL0(CLC * _clc, Processor * pCpu, const char *pName,
                   const char *pDesc)
    : sfr_register(pCpu, pName, pDesc), m_clc(_clc), write_mask(0x77)
{
}


void CLCxSEL0::put(unsigned int new_value)
{
    new_value &= write_mask;
    trace.raw(write_trace.get() | value.get());
    unsigned int diff = new_value ^ value.get();
    value.put(new_value);


    if (diff & 0xf)
    {
        m_clc->D1S(new_value & 0xf);
    }

    if (diff & 0xf0)
    {
        m_clc->D2S((new_value & 0xf0) >> 4);
    }

    if (diff && m_clc->CLCenabled())
    {
        m_clc->config_inputs(true);
    }
}


CLCxSEL1::CLCxSEL1(CLC * _clc, Processor * pCpu, const char *pName,
                   const char *pDesc)
    : sfr_register(pCpu, pName, pDesc), m_clc(_clc), write_mask(0x77)
{
}


void CLCxSEL1::put(unsigned int new_value)
{
    new_value &= write_mask;
    trace.raw(write_trace.get() | value.get());
    unsigned int diff = new_value ^ value.get();
    value.put(new_value);

    if (diff & 0xf)
    {
        m_clc->D3S(new_value & 0xf);
    }

    if (diff & 0xf0)
    {
        m_clc->D4S((new_value & 0xf0) >> 4);
    }

    if (diff && m_clc->CLCenabled())
    {
        m_clc->config_inputs(true);
    }
}


void CLCxGLS0::put(unsigned int new_value)
{
    unsigned int diff = new_value ^ value.get();
    trace.raw(write_trace.get() | value.get());
    value.put(new_value);

    if (!diff)
    {
        return;
    }

    if (m_clc->CLCenabled())
    {
        m_clc->config_inputs(true);
    }

    m_clc->compute_gates();
}


void CLCxGLS1::put(unsigned int new_value)
{
    unsigned int diff = new_value ^ value.get();
    trace.raw(write_trace.get() | value.get());
    value.put(new_value);

    if (!diff)
    {
        return;
    }

    if (m_clc->CLCenabled())
    {
        m_clc->config_inputs(true);
    }

    m_clc->compute_gates();
}


void CLCxGLS2::put(unsigned int new_value)
{
    unsigned int diff = new_value ^ value.get();
    trace.raw(write_trace.get() | value.get());
    value.put(new_value);

    if (!diff)
    {
        return;
    }

    if (m_clc->CLCenabled())
    {
        m_clc->config_inputs(true);
    }

    m_clc->compute_gates();
}


void CLCxGLS3::put(unsigned int new_value)
{
    unsigned int diff = new_value ^ value.get();
    trace.raw(write_trace.get() | value.get());
    value.put(new_value);

    if (!diff)
    {
        return;
    }

    if (m_clc->CLCenabled())
    {
        m_clc->config_inputs(true);
    }

    m_clc->compute_gates();
}


// CLCx calls to set it's LCx_OUT bit, result shared with
// all CLCx instances.
void CLCDATA::setbit(bool bit_val, unsigned int pos)
{
    Dprintf(("setbit LC%u_OUT %d\n", pos + 1, bit_val));

    if (bit_val)
    {
        value.put(value.get() | (1 << pos));

    }
    else
    {
        value.put(value.get() & ~(1 << pos));
    }

}


// used where all CLC instances use same input pins
void CLCDATA::setIOpin(PinModule *pin, int data)
{
    for(int i = 0; i < 4; i++)
    {
        if (m_clc[i])
            m_clc[i]->setIOpin(pin,data);
    }
}

CLC::CLC(Processor * cpu, unsigned int _index, CLCDATA * _clcdata):
    CLC_BASE(cpu, _index, _clcdata),
    clcxsel0(this, cpu, "clcxsel0", "Multiplexer Data 1 and 2 Select Register"),
    clcxsel1(this, cpu, "clcxsel1", "Multiplexer Data 3 and 4 Select Register")
{
}


CLC_BASE::CLC_BASE(Processor * cpu, unsigned int _index, CLCDATA * _clcdata):
    index(_index),
    clcxcon(this, cpu, "clcxcon", "Configurable Logic Cell Control Register"),
    clcxpol(this, cpu, "clcxpol", "Configurable Logic Cell Signal Polarity"),
    clcxgls0(this, cpu, "clcxgls0", "Gate 1 Logic Select Register"),
    clcxgls1(this, cpu, "clcxgls1", "Gate 2 Logic Select Register"),
    clcxgls2(this, cpu, "clcxgls2", "Gate 3 Logic Select Register"),
    clcxgls3(this, cpu, "clcxgls3", "Gate 4 Logic Select Register"),
    clcdata(_clcdata)
{
    for (int i = 0; i < 4; i++)
    {
        CMxOUT_level[i] = false;
        pwmx_level[i] = false;
        lcxdT[i] = false;
        lcxg[i] = false;
        dxs_data_length[i] = 0;
        dxs_data[i] = nullptr;
        INxsink[i] = nullptr;
        INxactive[i] = 0;
        INxstate[i] = false;
        m_clc[i] = nullptr;
	clc_data_receiver[i] = nullptr;
    }
    clc_data_server = new DATA_SERVER(DATA_SERVER::CLC);
    std::fill_n(DxS_data, 4, UNUSED);
    std::fill_n(pinCLCxIN, 4, nullptr);
    std::fill_n(p_tmr135, 3, nullptr);
    std::fill_n(p_tmr246, 3, nullptr);
    std::fill_n(attached_tmr135, 3, false);
    std::fill_n(t246_data_receiver, 3, nullptr);
}


CLC_BASE::~CLC_BASE()
{
    delete CLCxsrc;
    for(int i=0; i<4; i++)
    {
	if (clc_data_receiver[i]) delete clc_data_receiver[i];
    }
    for(int i=0; i<3; i++)
    {
	if (t246_data_receiver[i]) delete t246_data_receiver[i];
    }
    if(t135_data_receiver) delete t135_data_receiver;
    if(zcd_data_receiver) delete zcd_data_receiver;
    if(atx_data_receiver) delete atx_data_receiver;
    if (clc_data_server) delete clc_data_server;
}


/*
**    Set the DxS input selection table
**         input - table 1-4
**	   length - number of entries in the data array
**         data   - array of data_in enums
*/
void CLC_BASE::set_dxs_data(int input, int length, data_in *data)
{
    assert(input && input < 5);

    dxs_data_length[input-1] = length;
    dxs_data[input-1] = data;
#if DEBUG == 2
    Dprintf(("CLC_BASE::set_dxs_data  CLC%u table %d\n", index+1, input));
    for(int i=0; i<length; i++)
        printf("%d, ", dxs_data[input-1][i]);
    printf("\n");
#endif
    /* The CLCxSELx register values are cached after translation through the
     * values we've just accepted. The method that sets them has a diff
     * check, so its first invocation won't work if the value matches the
     * reset value. To get round that we do the updates here, assuming that
     * this method will only be called before the simulated code is run, so
     * the reset default of zero is still in effect.
     */
    DxS_data[input-1] = dxs_data[input-1][0];
}

void CLC_BASE::setIOpin(PinModule *pin, int data)
{
    Dprintf(("CLC_BASE::setIOpin CLC%u pin=%s data=%d\n", index+1, pin?pin->getPin().name().c_str():"NULL", data));
    if (data == CLCout_PIN)
    {
        if (!pin)
            oeCLCx(false);
        else
            setCLCxPin(pin);
    }
    else if (data >= CLCin0_PIN && data <= CLCin3_PIN)
    {
        int i = data - CLCin0_PIN;
        if (pin != pinCLCxIN[i])
        {
            if (CLCenabled())
            {
                if (pinCLCxIN[i])
                    enableINxpin(i, false);
                pinCLCxIN[i] = pin;
                enableINxpin(i, true);
            }
            else
            {
                pinCLCxIN[i] = pin;
            }
        }
    }
    else
    {
        fprintf(stderr, "CLC_BASE::setIOpin data=%d not supported\n", data);
    }
}


// Handle output pin multiplexing
void CLC_BASE::setCLCxPin(PinModule * alt_pin)
{
    Dprintf(("CLC_BASE::setCLCxPin CLC%u pin=%s\n", index+1, alt_pin?alt_pin->getPin().name().c_str():"NULL" ));
    if (alt_pin != pinCLCx)
    {
        oeCLCx(false);
        pinCLCx = alt_pin;
        oeCLCx(true);
    }
}


// Handle T0 overflow notification
void CLC_BASE::t0_overflow()
{
    bool gate_change = false;


    for (int i = 0; i < 4; i++)
    {
        if (DxS_data[i] == T0_OVER)
        {
            lcxdT[i] = true;
            gate_change = true;
        }
    }

    if (gate_change)
    {
        Dprintf(("CLC%u t0_overflow() enable=%d\n", index + 1,
                 CLCenabled()));
        compute_gates();

        for (int i = 0; i < 4; i++)
        {
            if (DxS_data[i] == T0_OVER)
            {
                lcxdT[i] = false;
            }
        }

        compute_gates();
    }
}


// Handle T1 (3, 5) overflow notification and toggle gate
void CLC_BASE::t135_overflow(int timer_number)
{
    bool gate_change = false;

    for (int i = 0; i < 4; i++)
    {
        if ( (DxS_data[i] == T1_OVER && timer_number == 1) ||
	     (DxS_data[i] == T3_OVER && timer_number == 3) ||
	     (DxS_data[i] == T5_OVER && timer_number == 5)
	   )
        {
    RRprint((stderr, "CLC_BASE::t135_overflow DxS_data[%d]=tmr%d_over \n",i, timer_number));
            lcxdT[i] = true;
            gate_change = true;
        }
    }

    if (gate_change)
    {
        compute_gates();

        for (int i = 0; i < 4; i++)
        {
            if ( (DxS_data[i] == T1_OVER && timer_number == 1) ||
	        (DxS_data[i] == T3_OVER && timer_number == 3) ||
	        (DxS_data[i] == T5_OVER && timer_number == 5)
	       )
            {
                lcxdT[i] = false;
            }
        }

        compute_gates();
    }
}

// Handle T1 overflow notification
void CLC_BASE::t1_overflow()
{
    RRprint((stderr, "CLC_BASE::t1_overflow \n"));
    bool gate_change = false;

    for (int i = 0; i < 4; i++)
    {
    RRprint((stderr, "CLC_BASE::t1_overflow DxS_data[%d]=0x%x\n",i, DxS_data[i]));
        if (DxS_data[i] == T1_OVER)
        {
            lcxdT[i] = true;
            gate_change = true;
        }
    }

    if (gate_change)
    {
        Dprintf(("CLC%u t1_overflow() enable=%d\n", index + 1,
                 CLCenabled()));
        compute_gates();

        for (int i = 0; i < 4; i++)
        {
            if (DxS_data[i] == T1_OVER)
            {
                lcxdT[i] = false;
            }
        }

        compute_gates();
    }
}


// Handle T[246] match notification
// If an input gate using a t[246]  match, toggle input gate
void CLC_BASE::t246_match(char tmr_number)
{
    bool gate_change = false;

    for (int i = 0; i < 4; i++)
    {
        if (
            (DxS_data[i] == T2_MATCH && tmr_number == 2) ||
            (DxS_data[i] == T4_MATCH && tmr_number == 4) ||
            (DxS_data[i] == T6_MATCH && tmr_number == 6)
        )
        {
            lcxdT[i] = true;
            gate_change = true;
        }
    }

    if (gate_change)
    {
        Dprintf(("CLC%u t2_match(%d) enable=%d\n", index + 1, tmr_number, CLCenabled()));
        compute_gates();

        for (int i = 0; i < 4; i++)
        {
            if (
                (DxS_data[i] == T2_MATCH && tmr_number == 2) ||
                (DxS_data[i] == T4_MATCH && tmr_number == 4) ||
                (DxS_data[i] == T6_MATCH && tmr_number == 6)
            )
            {
                lcxdT[i] = false;
            }
        }

        compute_gates();
    }
}


// Handle updates for frc or lfintosc
void CLC_BASE::osc_out(bool level, int kind)
{
    bool gate_change = false;

    for (int i = 0; i < 4; i++)
    {
        if (DxS_data[i] == kind && lcxdT[i] != level)
        {
            lcxdT[i] = level;
            gate_change = true;
        }
    }

    if (gate_change)
    {
        Dprintf(("CLC%u osc_out() kind=%d level=%d enable=%d\n", index + 1,
                 kind, level, CLCenabled()));
        compute_gates();
    }
}


// Handle updates for NCO module
void CLC_BASE::NCO_out(bool level)
{
    if (NCO_level != level)
    {
        bool gate_change = false;
        NCO_level = level;

        for (int i = 0; i < 4; i++)
        {
            if (DxS_data[i] == NCOx)
            {
                lcxdT[i] = level;
                gate_change = true;
            }
        }

        if (gate_change)
        {
            Dprintf(("CLC%u NCO_out() level=%d enable=%d\n", index + 1,
                     level, CLCenabled()));
            compute_gates();
        }
    }
}

// Handle updates for ZCD module
void CLC_BASE::ZCDx_out(bool level)
{
    if (ZCD_level != level)
    {
        bool gate_change = false;
        ZCD_level = level;

        for (int i = 0; i < 4; i++)
        {
            if (DxS_data[i] == ZCD_OUT)
            {
                lcxdT[i] = level;
                gate_change = true;
            }
        }

        if (gate_change)
        {
            Dprintf(("CLC%u ZCD_out() level=%d enable=%d\n", index + 1,
                     level, CLCenabled()));
            compute_gates();
        }
    }
}

//Handle updates from ATx
void CLC_BASE::ATx_out(bool level, int v2)
{

    bool gate_change = false;
    switch(v2 & ATx::ATxMask)
    {
    case ATx::PERCLK:
	for(int i = 0; i < 4; i++)
	{
	    if (DxS_data[i] == AT1_PERCLK)
	    {
		if (lcxdT[i] != level)
		{
		    lcxdT[i] = level;
		    gate_change = true;
		}
	    }
	}
        if (gate_change)
	{
	    compute_gates();
	}
	break;

    case ATx::MISSPUL:
	for(int i = 0; i < 4; i++)
	{
	    if (DxS_data[i] == AT1_MISSPULSE)
	    {
		if (lcxdT[i] != level)
		{
		    lcxdT[i] = level;
		    gate_change = true;
		}
	    }
	}
        if (gate_change)
	{
	    compute_gates();
	}
	break;

    case ATx::PHSCLK:
	for(int i = 0; i < 4; i++)
	{
	    if (DxS_data[i] == AT1_PHSCLK)
	    {
		if (lcxdT[i] != level)
		{
		    lcxdT[i] = level;
		    gate_change = true;
		}
	    }
	}
        if (gate_change)
	{
	    compute_gates();
	}
	break;

    case ATx::CMP1:
	for(int i = 0; i < 4; i++)
	{
	    if (DxS_data[i] == AT1_CMP1)
	    {
		if (lcxdT[i] != level)
		{
		    lcxdT[i] = level;
		    gate_change = true;
		}
	    }
	}
        if (gate_change)
	{
	    compute_gates();
	}
	break;

    case ATx::CMP2:
	for(int i = 0; i < 4; i++)
	{
	    if (DxS_data[i] == AT1_CMP2)
	    {
		if (lcxdT[i] != level)
		{
		    lcxdT[i] = level;
		    gate_change = true;
		}
	    }
	}
        if (gate_change)
	{
	    compute_gates();
	}
	break;

    case ATx::CMP3:
	for(int i = 0; i < 4; i++)
	{
	    if (DxS_data[i] == AT1_CMP3)
	    {
		if (lcxdT[i] != level)
		{
		    lcxdT[i] = level;
		    gate_change = true;
		}
	    }
	}
        if (gate_change)
	{
	    compute_gates();
	}
	break;

    };
}

// Handle updates from comparator module
void CLC_BASE::CxOUT_sync(bool level, int cm)
{
    if (CMxOUT_level[cm] != level)
    {
        bool gate_change = false;
        CMxOUT_level[cm] = level;

        for (int i = 0; i < 4; i++)
        {
            if ((DxS_data[i] == C1OUT && cm == 0) ||
                (DxS_data[i] == C2OUT && cm == 1) )
            {
                lcxdT[i] = level;
                gate_change = true;
            }
        }

        if (gate_change)
        {
            Dprintf(("CLC%u C%dOUT_sync() level=%d enable=%d\n", index + 1,
                     cm + 1, level, CLCenabled()));
            compute_gates();
        }
    }
}


// Handle updates from pwm module
void CLC_BASE::out_pwm(bool level, int id)
{
    Dprintf(("CLC%u out_pwm() pwm%d level=%d enable=%d\n", index + 1, id + 1,
             level, CLCenabled()));

    if (pwmx_level[id] != level)
    {
        bool gate_change = false;
        pwmx_level[id] = level;

        for (int i = 0; i < 4; i++)
        {
            if ((DxS_data[i] == PWM1 && id == 0) ||
                    (DxS_data[i] == PWM2 && id == 1) ||
                    (DxS_data[i] == PWM3 && id == 2) ||
                    (DxS_data[i] == PWM4 && id == 3))
            {
                lcxdT[i] = level;
                gate_change = true;
            }
        }

        if (gate_change)
        {
            Dprintf(("CLC%u out_pwm() pwm%d level=%d enable=%d\n",
                     index + 1, id + 1, level, CLCenabled()));
            compute_gates();
        }
    }
}


// notification on CLCxIN[12]
void CLC_BASE::setState(char new3State, int id)
{
    bool state = (new3State == '1' || new3State == 'W');

    if (state != INxstate[id])
    {
        bool gate_change = false;
        INxstate[id] = state;

        for (int i = 0; i < 4; i++)
        {
            if ( (DxS_data[i] == CLCxIN0 && id == 0) ||
                 (DxS_data[i] == CLCxIN1 && id == 1) ||
                 (DxS_data[i] == CLCxIN2 && id == 2) ||
                 (DxS_data[i] == CLCxIN3 && id == 3) )
            {
                lcxdT[i] = state;
                gate_change = true;
            }
        }

        if (gate_change)
        {
            Dprintf(("CLC%u setState() IN%d level=%d enable=%d\n",
                     index + 1, id, state, CLCenabled()));
            compute_gates();
        }
    }
}


// Enable/Disable input pin i
void CLC_BASE::enableINxpin(int i, bool on)
{
    if (on)
    {
        if (!INxactive[i])
        {
            char name[7] = "LCyINx";

            if (!INxgui[i].length())
            {
                INxgui[i] = pinCLCxIN[i]->getPin().GUIname();
            }

            name[2] = '0' + index;
            name[5] = '0' + i;
            pinCLCxIN[i]->getPin().newGUIname(name);

            if (!INxsink[i])
            {
                INxsink[i] = new INxSignalSink(this, i);
            }

            pinCLCxIN[i]->addSink(INxsink[i]);
            setState(pinCLCxIN[i]->getPin().getState() ? '1' : '0', i);
        }

        INxactive[i]++;

    }
    else if (!--INxactive[i])
    {
        if (INxgui[i].length())
        {
            pinCLCxIN[i]->getPin().newGUIname(INxgui[i].c_str());

        }
        else
            pinCLCxIN[i]->getPin().newGUIname(pinCLCxIN[i]->getPin().
                                              name().c_str());

        if (INxsink[i])
        {
            pinCLCxIN[i]->removeSink(INxsink[i]);
        }
    }
}


// Enable/disable output pin
void CLC_BASE::oeCLCx(bool on)
{
    if (on)
    {
        if (!srcCLCxactive)
        {
            char name[] = "CLCx";
            name[3] = '1' + index;
            if (!CLCxsrc)
            {
                CLCxsrc = new CLCSigSource(this, pinCLCx);
            }
            CLCxsrc->setState((clcxcon.value.get() & LCxOE) ? '1' : '0');
	    if (pinCLCx)
	    {
            	CLCxgui = pinCLCx->getPin().GUIname();
            	pinCLCx->getPin().newGUIname(name);
		pinCLCx->setSource(CLCxsrc);
		srcCLCxactive = true;
		pinCLCx->updatePinModule();
	    }
        }

    }
    else if (srcCLCxactive)
    {
	if (pinCLCx)
	{
            if (CLCxgui.length())
                pinCLCx->getPin().newGUIname(CLCxgui.c_str());
            else
                pinCLCx->getPin().newGUIname(pinCLCx->getPin().name().c_str());

            pinCLCx->setSource(0);
            pinCLCx->updatePinModule();
	}
        delete CLCxsrc;
        CLCxsrc = nullptr;
        srcCLCxactive = false;
    }
}


// Update the output value of each of the 4 Data Gates
// taking into account both input and output polarity
void CLC_BASE::compute_gates()
{
    unsigned int glsx[] =
    {
        clcxgls0.value.get(), clcxgls1.value.get(),
        clcxgls2.value.get(), clcxgls3.value.get()
    };
    int mask = 0;
    unsigned int pol = clcxpol.value.get();

    // The gate logic feeds the four inputs and their inverted forms into
    // eight AND gates with the Gate Logic Select register, then OR's the
    // eight outputs together. This first loop constructs the normal and
    // inverted signals on the bus as a byte, the second loop does the ANDs
    // and the OR for each gate
    for (int i = 0; i < 4; i++)
    {
        mask |= ( lcxdT[i] ? 2 : 1 ) << (2*i);
    }
    for (int j = 0; j < 4; j++)
    {
        bool gate_out = false;
        if (glsx[j] & mask)
            gate_out = true;
        if ( pol & (1 << j) )
            gate_out =  !gate_out;
        lcxg[j] = gate_out;
    }

    if (CLCenabled())
    {
        Dprintf(("CLC_BASE::compute_gates CLC%u lcxdT = {%d %d %d %d} lcxg={%d %d %d %d}\n", index + 1, lcxdT[0], lcxdT[1], lcxdT[2], lcxdT[3], lcxg[0], lcxg[1], lcxg[2], lcxg[3]));
    }

    cell_function();
}


// Select and execute cell functions
void CLC_BASE::cell_function()
{
    bool out = false;
    unsigned int con = clcxcon.value.get();
    unsigned int pol = clcxpol.value.get();

    switch (con & 0x7)
    {
    case 0:			// AND-OR
        out = (lcxg[0] && lcxg[1]) || (lcxg[2] && lcxg[3]);
        break;

    case 1:			// OR-XOR
        out = (lcxg[0] || lcxg[1]) ^ (lcxg[2] || lcxg[3]);
        break;

    case 2:			// 4 input AND
        out = lcxg[0] && lcxg[1] && lcxg[2] && lcxg[3];
        break;

    case 3:
        out = cell_sr_latch();
        break;

    case 4:
        out = cell_1_in_flipflop();
        break;

    case 5:
        out = cell_2_in_flipflop();
        break;

    case 6:
        out = JKflipflop();
        break;

    case 7:
        out = transparent_D_latch();
        break;
    }

    if (pol & LCxPOL)
    {
        out = !out;
    }

    if (CLCenabled())
    {
        outputCLC(out);
    }
}


// Send output to required consumers
void CLC_BASE::outputCLC(bool out)
{
    unsigned int con = clcxcon.value.get();
    bool old_out = con & LCxOUT;
    Dprintf(("outputCLC CLC%u out=%d old_out=%d clcdata=0x%x\n", index, out,
             old_out, clcdata->value.get()));

    if (out)
    {
        con |= LCxOUT;
    }
    else
    {
        con &= ~LCxOUT;
    }

    clcxcon.value.put(con);
    assert(m_Interrupt);
    Dprintf(("CLC_BASE::outputCLC CLC%u old_out %d out %d int 0x%x en=%d\n", index + 1,
             old_out, out, con & LCxINTP, out_enabled()));

    if (!old_out && out && (con & LCxINTP))  	//Positive edge interrupt
    {
        m_Interrupt->Trigger();
    }

    if (old_out && !out && (con & LCxINTN))  	//Negative edge interrupt
    {
        m_Interrupt->Trigger();
    }

    assert(clcdata);
    clcdata->setbit(out, index);

    clc_data_server->send_data(out, index);

    if (p_cog)
        p_cog->out_clc(out, index);

    if (out_enabled())
    {
	if (CLCxsrc)
           CLCxsrc->setState(out ? '1' : '0');
	if (pinCLCx)
            pinCLCx->updatePinModule();
    }
}


bool CLC_BASE::cell_sr_latch()
{
    bool set = lcxg[0] || lcxg[1];
    bool reset = lcxg[2] || lcxg[3];


    if (set)
    {
        Doutput = true;

    }
    else if (reset)
    {
        Doutput = false;
    }

    return Doutput;
}


bool CLC_BASE::cell_1_in_flipflop()
{
    bool set = lcxg[3];
    bool reset = lcxg[2];
    bool clock = lcxg[0];
    bool D = lcxg[1];

    if (set)
    {
        Doutput = true;

    }
    else if (reset)
    {
        Doutput = false;

    }
    else if (!Dclock && clock)
    {
        Doutput = D;
    }

    Dclock = clock;
    return Doutput;
}


bool CLC_BASE::cell_2_in_flipflop()
{
    bool reset = lcxg[2];
    bool clock = lcxg[0];
    bool D = lcxg[1] || lcxg[3];

    if (reset)
    {
        Doutput = false;

    }
    else if (!Dclock && clock)
    {
        Doutput = D;
    }

    Dclock = clock;
    return Doutput;
}


bool CLC_BASE::JKflipflop()
{
    bool J = lcxg[1];
    bool K = lcxg[3];
    bool reset = lcxg[2];
    bool clock = lcxg[0];

    if (reset)
    {
        Doutput = false;

    }
    else if (!Dclock && clock)  	// Clock + edge
    {
        if (J && K)  	// Toggle output
        {
            Doutput = !Doutput;

        }
        else if (J && !K)  	// Set output
        {
            Doutput = true;

        }
        else if (!J && K)  	// clear output
        {
            Doutput = false;

        }
        else if (!J && !K)    // no change
        {
            ;
        }
    }

    Dclock = clock;
    return Doutput;
}


bool CLC_BASE::transparent_D_latch()
{
    bool reset = lcxg[0];
    bool D = lcxg[1];
    bool LE = lcxg[2];
    bool set = lcxg[3];

    if (set)
    {
        Doutput = true;

    }
    else if (reset)
    {
        Doutput = false;

    }
    else if (!LE)
    {
        Doutput = D;
    }

    return Doutput;
}


void CLC_BASE::releasePinSource(PinModule * pin)
{
    if (pin == pinCLCx)
    {
        srcCLCxactive = false;
    }
}


// Called from clcdata, process LCx_OUT updates where x = pos
void CLC_BASE::clc_lcxupdate(bool bit_val, unsigned int pos)
{
    bool update = false;

    for (int i = 0; i < 4; i++)
    {
        if ((lcxdT[i] != bit_val) &&
                ((DxS_data[i] == LC1 && pos == 0) ||
                 (DxS_data[i] == LC2 && pos == 1) ||
                 (DxS_data[i] == LC3 && pos == 2) ||
                 (DxS_data[i] == LC4 && pos == 3)))
        {
            update = true;
            lcxdT[i] = bit_val;
        }
    }

    if (update)
    {
        if (CLCenabled())
            Dprintf(("CLC%u lcxupdate LC%u_OUT=%d\n", index + 1, pos + 1,
                     bit_val));

        compute_gates();
    }
}


// CLCCON register has changed
void CLC_BASE::update_clccon(unsigned int diff)
{
    unsigned int val = clcxcon.value.get();

    if (diff & LCxOE)
    {
        if ((val & (LCxOE | LCxEN)) == (LCxOE | LCxEN))
        {
            oeCLCx(true);
        }

        if ((val & (LCxOE | LCxEN)) == (LCxEN))
        {
            oeCLCx(false);
        }
    }

    if (diff & LCxEN)  	// clc off or on
    {
        if (val & LCxEN)  	// CLC on
        {
            config_inputs(true);

        }
        else  		// CLC off
        {
            config_inputs(false);
            oeCLCx(false);
        }
    }
}


// Initialize inputs as required, called when CLC is enabled or disabled
// or changes in clcxselx clcxglsx while clc enabled
void CLC_BASE::config_inputs(bool on)
{
    unsigned int active_gates = clcxgls0.value.get() |
                                clcxgls1.value.get() | clcxgls2.value.get() | clcxgls3.value.get();
    Dprintf(("config_inputs CLC%u on=%d active_gates=0x%x\n", index + 1, on,
             active_gates));
    bool haveIN[4] = {false, false, false, false};
    bool haveFRC = false;
    bool haveFOSCLK = false;
    bool haveLFINTOSC = false;
    bool haveHFINTOSC = false;
    int mask = 3;

    // the input pins and freqency inputs need to be turned on and off.
    // this function does this. Note that the inputs which are from
    // other modules are not turned on or off here, but are ignored
    // if they are not active inputs.

    for (int i = 0; i < 4; i++)
    {
        if (active_gates & mask)  	// data input used
        {

	    switch(DxS_data[i])
	    {
	    case T0_OVER:
	    case PWM3:
	    case PWM4:
	    case NCOx:
		break;		//setup not required

	    case FOSCLK:
		haveFOSCLK = true;
		break;

	    case T2_MATCH:
		if(!t246_data_receiver[0] && p_tmr246[0])
		{
		    t246_data_receiver[0] = new CLC_DATA_RECEIVER(this, "t2_overflow_receiver");
		    p_tmr246[0]->get_tmr246_server()->attach_data(t246_data_receiver[0]);
		}
		break;

	    case T4_MATCH:
		if(!t246_data_receiver[1] && p_tmr246[1])
		{
		    t246_data_receiver[1] = new CLC_DATA_RECEIVER(this, "t4_overflow_receiver");
		    p_tmr246[1]->get_tmr246_server()->attach_data(t246_data_receiver[1]);
		}
		break;

	    case T6_MATCH:
		if(!t246_data_receiver[2] && p_tmr246[2])
		{
		    t246_data_receiver[2] = new CLC_DATA_RECEIVER(this, "t6_overflow_receiver");
		    p_tmr246[2]->get_tmr246_server()->attach_data(t246_data_receiver[2]);
		}
		break;

	    case T1_OVER:
		if(!t135_data_receiver)
		{
		    t135_data_receiver = new CLC_DATA_RECEIVER(this, "t135_overflow_receiver");
		}
	        if (p_tmr135[0] && !attached_tmr135[0])
	        {
		    p_tmr135[0]->get_tmr135_server()->attach_data(t135_data_receiver);
		    attached_tmr135[0] = true;
		}
		break;

	    case T3_OVER:
		if(!t135_data_receiver)
		{
		    t135_data_receiver = new CLC_DATA_RECEIVER(this, "t135_overflow_receiver");
		}
	        if (p_tmr135[1] && !attached_tmr135[1])
	        {
		    p_tmr135[1]->get_tmr135_server()->attach_data(t135_data_receiver);
		    attached_tmr135[1] = true;
		}
		break;

	    case T5_OVER:
		if(!t135_data_receiver)
		{
		    t135_data_receiver = new CLC_DATA_RECEIVER(this, "t135_overflow_receiver");
		}
	        if (p_tmr135[2] && !attached_tmr135[2])
	        {
		    p_tmr135[2]->get_tmr135_server()->attach_data(t135_data_receiver);
		    attached_tmr135[2] = true;
		}
		break;

	    case LC1:
		if(!clc_data_receiver[0])
		{
		    clc_data_receiver[0] = new CLC_DATA_RECEIVER(this, "clc1_receiver");
		    m_clc[0]->get_CLC_data_server()->attach_data(clc_data_receiver[0]);
		}
		break;

	    case LC2:
		if(!clc_data_receiver[1])
		{
		    clc_data_receiver[1] = new CLC_DATA_RECEIVER(this, "clc2_receiver");
		    m_clc[1]->get_CLC_data_server()->attach_data(clc_data_receiver[1]);
		}
		break;

	    case LC3:
		if(!clc_data_receiver[2])
		{
		    clc_data_receiver[2] = new CLC_DATA_RECEIVER(this, "clc3_receiver");
		    m_clc[2]->get_CLC_data_server()->attach_data(clc_data_receiver[2]);
		}
		break;

	    case LC4:
		if(!clc_data_receiver[3])
		{
		    clc_data_receiver[3] = new CLC_DATA_RECEIVER(this, "clc4_receiver");
		    m_clc[3]->get_CLC_data_server()->attach_data(clc_data_receiver[3]);
		}
		break;

	     case ZCD_OUT:
		if(!zcd_data_receiver)
		{
		    zcd_data_receiver = new CLC_DATA_RECEIVER(this, "zcd_receiver");
		    m_zcd->get_zcd_data_server()->attach_data(zcd_data_receiver);
		}
		break;

	     case AT1_PERCLK:
	     case AT1_MISSPULSE:
	     case AT1_PHSCLK:
	     case AT1_CMP1:
	     case AT1_CMP2:
	     case AT1_CMP3:
		if (!atx_data_receiver)
		{
		    atx_data_receiver = new CLC_DATA_RECEIVER(this, "atx_receiver");
		    m_at1->get_atx_data_server()->attach_data(atx_data_receiver);
		}
		break;


	     case CLCxIN0:	//  CLCxIN0 first input pin
		haveIN[0] = true;
		// if on=true but active=false or on=false and active=true
		// enable or disable pin depending on  value of on
		if(INxactive[0] ^ on)  enableINxpin(0, on);
		break;

	     case CLCxIN1:	//  CLCxIN1 first input pin
		haveIN[1] = true;
		// if on=true but active=false or on=false and active=true
		// enable or disable pin depending on  value of on
		if(INxactive[1] ^ on)  enableINxpin(1, on);
		break;

	     case CLCxIN2:	//  CLCxIN2 first input pin
		haveIN[2] = true;
		// if on=true but active=false or on=false and active=true
		// enable or disable pin depending on  value of on
		if(INxactive[2] ^ on)  enableINxpin(2, on);
		break;

	     case CLCxIN3:	//  CLCxIN3 first input pin
		haveIN[3] = true;
		// if on=true but active=false or on=false and active=true
		// enable or disable pin depending on  value of on
		if(INxactive[3] ^ on)  enableINxpin(3, on);
		break;

	    case LFINTOSC:
		haveLFINTOSC = true;
        	Dprintf(("config_inputs CLC%u LFINTOSC LFINTOSCactive=%d on=%d changr=%d\n",
                 	index + 1, LFINTOSCactive, on, LFINTOSCactive ^ on));
    		if (LFINTOSCactive ^ on)  // change state
    		{
        	    LFINTOSCactive = on;
        	    if (!lfintosc) // sanity check
		    {
	    		fprintf(stderr, "lfintosc not defined\n");
	    		assert(lfintosc);
        	    }
        	    lfintosc->start_osc_sim(on);
    		}
		break;

	    case HFINTOSC:
		haveHFINTOSC = true;
        	Dprintf(("config_inputs CLC%u HFINTOSC LFINTOSCactive=%d on=%d\n",
                 	index + 1, HFINTOSCactive, on));
    		if (HFINTOSCactive ^ on)  // change state
    		{
        	    HFINTOSCactive = on;
        	    if (!hfintosc) // sanity check
		    {
	    		fprintf(stderr, "hfintosc not defined\n");
	    		assert(hfintosc);
        	    }
        	    hfintosc->start_osc_sim(on);
    		}
		break;

	    case FRC_IN:
		haveFRC = true;
        Dprintf(("config_inputs CLC%u FRC FRCactive=%d on=%d\n", index + 1,
                 FRCactive, on));

    		if (FRCactive ^ on)  // change state of FRC
		{
        	    if (!frc) // sanity check
		    {
	     		fprintf(stderr, "frc not defined\n");
	    		assert(frc);
        	    }
        	    FRCactive = on;
        	    frc->start_osc_sim(on);
		}
		break;

	    default:
//#ifdef DEBUG ==1
		fprintf(stderr, "CLC_BASE::config_inputs CLC%u DxS_data[%d]=0x%x not found\n",index+1, i, DxS_data[i]);
//#endif
		break;
	    }
        }

        mask <<= 2;
    }

    for (int i = 0; i < 4; i++)
    {
        if (!haveIN[i] && INxactive[i])  // Turn off unused active IOpin
        {
            Dprintf(("config_inputs CLC%u IN%d OFF on=%d\n", index + 1, i, on));
            enableINxpin(i, false);
        }
    }



    if (!haveLFINTOSC && LFINTOSCactive)
    {
        Dprintf(("config_inputs CLC%u LFINTOSC OFF  on=%d\n", index + 1,
                 on));
        LFINTOSCactive = false;
        lfintosc->start_osc_sim(false);
    }

    if (!haveHFINTOSC && HFINTOSCactive)
    {
        Dprintf(("config_inputs CLC%u HFINTOSC OFF  on=%d\n", index + 1,
                 on));
        HFINTOSCactive = false;
        hfintosc->start_osc_sim(false);
    }
    if (!haveFRC && FRCactive)  // turn off, not used
    {
        Dprintf(("config_inputs CLC%u FRC OFF  on=%d\n", index + 1, on));
        FRCactive = false;
        frc->start_osc_sim(false);
    }

    if (!haveFOSCLK)
    {
    }

    if (on)
    {
        compute_gates();
    }
}


CLC_4SEL::CLC_4SEL(Processor * cpu, unsigned int _index, CLCDATA * _clcdata)
    : CLC_BASE(cpu, _index, _clcdata),
      clcxsel0(this, cpu, "clcxsel0", "Multiplexer Data 1 Select Register", 0),
      clcxsel1(this, cpu, "clcxsel1", "Multiplexer Data 2 Select Register", 1),
      clcxsel2(this, cpu, "clcxsel2", "Multiplexer Data 3 Select Register", 2),
      clcxsel3(this, cpu, "clcxsel3", "Multiplexer Data 4 Select Register", 3)
{
}




// OSC_SIM simulates clock inputs for CLC.
// If the requested frequency > FOSC/4, OSC_SIM will generate pulses at
// a frequency of FOSC/4.
// If the requested frequency is not a whole fraction of FOSC/4, the
// simulated clock will have jitter to approximate the requested frequency.
// The duty cycle of the simulated frequency will only be 50% when
// the requested frequency is a whole fraction of FOSC/8.
OSC_SIM::OSC_SIM(double _freq, int _data_in)
    : frequency(_freq), data_in(_data_in)
{
    std::fill_n(m_clc, 4, nullptr);
}


void OSC_SIM::start_osc_sim(bool on)
{
    if (on)
    {
        Dprintf(("OSC_SIM::start_osc_sim freq=%.0f kHz active %d\n",
                 frequency / 1000.0, active));

        if (!active)
        {
            int cycles = get_cycles().instruction_cps() / frequency + 0.5;

            if (cycles < 2)
            {
                fprintf(stderr,
                   "OSC_SIM  %.1f kHz not simulated at current CPU frequency\n",
                        frequency / 1000.0);
                fprintf(stderr, "Using pulses at %.1f kHz\n",
                        get_cycles().instruction_cps() / 1000.0);
                cycles = 2;
            }

            adjust_cycles = frequency - get_cycles().instruction_cps() / cycles;
            next_cycle = cycles / 2;
            level = true;

            for (int i = 0; i < 4; i++)
            {
                if (m_clc[i])
                {
                    m_clc[i]->osc_out(level, data_in);
                }
            }

            if (future_cycle)
            {
                get_cycles().clear_break(this);
            }

            future_cycle = get_cycles().get() + cycles - next_cycle;
            get_cycles().set_break(future_cycle, this);
            Dprintf(("OSC_SIM::start_osc_sim cycles=%d adj_cycles=%" PRINTF_GINT64_MODIFIER "d freq=%.1f kHz inst_cps=%e\n", cycles, adjust_cycles, frequency / 1000.0, get_cycles().instruction_cps()));
        }
        active++;
    }
    else if (--active == 0)
    {
        Dprintf(("OSC_SIM::start_osc_sim stop freq=%.0f\n",
                 frequency / 1000.0));

        if (future_cycle)
        {
            get_cycles().clear_break(this);
            future_cycle = 0;
        }
    }
}


void OSC_SIM::callback()
{
    for (int i = 0; i < 4; i++)
    {
        if (m_clc[i])
        {
            m_clc[i]->osc_out(!level, data_in);
	    if (!next_cycle && level)   // Sending a pulse
		m_clc[i]->osc_out(level, data_in);
        }
    }


    if (next_cycle)
    {
        future_cycle = get_cycles().get() + next_cycle;
        next_cycle = 0;
        level = false;
    }
    else
    {
        adjust_cycles += frequency;
        int cycles = get_cycles().instruction_cps() / adjust_cycles + 0.5;

        if (cycles < 2)
        {
            cycles = 1;
            adjust_cycles = 0;
        }
        else
        {
            adjust_cycles -= get_cycles().instruction_cps() / cycles;
        }
        next_cycle = cycles / 2;
        level = true;
        future_cycle = get_cycles().get() + cycles - next_cycle;
    }
    get_cycles().set_break(future_cycle, this);
}
