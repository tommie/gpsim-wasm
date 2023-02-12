/*

   Copyright (C) 1998 T. Scott Dattalo
   Copyright (C) 2006,2010,2013,2015,2020 Roy R. Rankin

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


#include <algorithm>
#include <iostream>
#include <cstdio>

#include <assert.h>
#include <math.h>
#include <string.h>

#include "14bit-registers.h"
#include "14bit-tmrs.h"
#include "clc.h"
#include "ctmu.h"
#include "cwg.h"
#include "ioports.h"
#include "modules.h"
#include "pir.h"
#include "processor.h"
#include "stimuli.h"
#include "trace.h"
#include "ui.h"

#include "comparator.h"

#define DEBUG 0
#if DEBUG & 1
#include <config.h>
#define Dprintf(arg) {printf("%s:%d ",__FILE__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif
#if DEBUG & 2
#define RRprint(arg) {fprintf(stderr, "%s:%d %s ",__FILE__,__LINE__, __FUNCTION__); fprintf arg;}
#else
#define RRprint(arg) {}
#endif

#define p_cpu static_cast<Processor*>(get_module())


ComparatorModule::ComparatorModule(Processor *pCpu)
    : cmcon(pCpu, "cmcon", "Comparator Module Control"),
      cmcon1(pCpu, "cmcon1", "Comparator Configure Register"),
      vrcon(pCpu, "vrcon", "Voltage Reference Control")
{
}


void ComparatorModule::initialize(PIR_SET *pir_set,
                                  PinModule *pin_vr0, PinModule *pin_cm0,
                                  PinModule *pin_cm1, PinModule *pin_cm2, PinModule *pin_cm3,
                                  PinModule *pin_cm4, PinModule *pin_cm5)
{
    //  cmcon = new CMCON;
    cmcon.assign_pir_set(pir_set);
    cmcon.setINpin(0, pin_cm0, "an0");
    cmcon.setINpin(1, pin_cm1, "an1");
    cmcon.setINpin(2, pin_cm2, "an2");
    cmcon.setINpin(3, pin_cm3, "an3");
    cmcon.setIOpin(pin_cm4, 0);
    cmcon.setIOpin(pin_cm5, 1);
    vrcon.setIOpin(pin_vr0);
    cmcon._vrcon = &vrcon;
    vrcon._cmcon = &cmcon;
}


//--------------------------------------------------
//
//--------------------------------------------------
class CMSignalSource : public SignalControl
{
public:
    CMSignalSource(CMCON *_cmcon, int _index)
        : m_state('0'), m_cmcon(_cmcon), index(_index)
    {
    }
    ~CMSignalSource()
    {
        //    cout << "deleting CMsignal source " << this << endl;
    }

    void release() override
    {
        m_cmcon->releasePin(index);
    }
    char getState() override
    {
        return m_state;
    }
    void putState(bool new_val)
    {
        m_state = new_val ? '1' : '0';
    }

private:
    char m_state;
    CMCON *m_cmcon;
    int  index;
};


//--------------------------------------------------
//
//--------------------------------------------------
class CMxSignalSource : public PeripheralSignalSource
{
public:
    CMxSignalSource(PinModule *_pin, CMxCON0_base *_cmcon)
        : PeripheralSignalSource(_pin),  m_cmcon(_cmcon)
    {
    }
    ~CMxSignalSource()
    {
        //    cout << "deleting CMsignal source " << this << endl;
    }

    void release() override
    {
        m_cmcon->releasePin();
    }

private:
    CMxCON0_base *m_cmcon;
};


//--------------------------------------------------
//
//--------------------------------------------------

CM_stimulus::CM_stimulus(CMCON * arg, const char *cPname, double _Vth, double _Zth)
    : stimulus(cPname, _Vth, _Zth), _cmcon(arg)
{
}


CM_stimulus::~CM_stimulus()
{
}


void   CM_stimulus::set_nodeVoltage(double v)
{
    if (nodeVoltage != v)
    {
        nodeVoltage = v;
        Dprintf(("set_nodeVoltage %s _cmcon %p %s v=%.2f\n", name().c_str(), _cmcon, _cmcon->name().c_str(), v));
        _cmcon->get();  // recalculate comparator values
    }
}


/*
    Setup the configuration for the comparators. Must be called
    for each comparator and each mode(CN2:CM0) that can be used.
        il1 = input Vin- when CIS == 0
        ih1 = input Vin+ when CIS == 0
        il2 = input Vin- when CIS == 1
        ih2 = input Vin+ when CIS == 1

        if input == VREF, reference voltage is used.
*/

void CMCON::set_configuration(int comp, int mode, int il1, int ih1, int il2,
                              int ih2, int out)
{
    if (comp > cMaxComparators || comp < 1)
    {
        std::cout << "CMCON::set_configuration comp=" << comp << " out of range\n";
        return;
    }

    if (mode > cMaxConfigurations)
    {
        std::cout << "CMCON::set_configuration mode too large\n";
        return;
    }

    m_configuration_bits[comp - 1][mode] = (il1 << CFG_SHIFT * 4) |
                                           (ih1 << CFG_SHIFT * 3) |
                                           (il2 << CFG_SHIFT * 2) | (ih2 << CFG_SHIFT) | out;
}


//------------------------------------------------------------------------
CMCON::CMCON(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc)
{
    value.put(0);
    cm_input[0] = cm_input[1] = cm_input[2] = cm_input[3] = nullptr;
    cm_output[0] = cm_output[1] = nullptr;
    cm_source[0] = cm_source[1] = nullptr;
    cm_stimulus[0] = cm_stimulus[1] = cm_stimulus[2] = cm_stimulus[3] = nullptr;
    cm_source_active[0] = cm_source_active[1] = false;
    m_CMval[0] = m_CMval[1] = 0;
}


CMCON::~CMCON()
{
    unsigned int mode = value.get() & 0x07;

    for (int i = 0; i < 2; i++)
    {
        if (cm_source[i])
        {
            int cfg = m_configuration_bits[i][mode] & CFG_MASK;

            // Our Source active so port still defined if cm_output defined,
            // set default source.
            if (cfg == i && cm_output[i] && cm_source_active[i])
            {
                cm_output[i]->setSource(0);
            }

            delete cm_source[i];
        }
    }

    for (int i = 0; i < 4; i++)
    {
        delete cm_stimulus[i];
    }
}


void CMCON::releasePin(int i)
{
    cm_source_active[i] = false;
}


void CMCON::setINpin(int i, PinModule *newPinModule, const char *an)
{
    if (newPinModule == nullptr)
    {
        return;
    }

    cm_input[i] = newPinModule;
    cm_input_pin[i] = newPinModule->getPin()->name();
    cm_an[i] = an;
}


void CMCON::setIOpin(PinModule *newPinModule, int i)
{
    if (newPinModule == nullptr)
    {
        return;
    }

    cm_output[i] = newPinModule;
    cm_output_pin[i] = newPinModule->getPin()->name();

}


void CMCON::assign_pir_set(PIR_SET *new_pir_set)
{
    pir_set = new_pir_set;
}


double CMCON::comp_voltage(int ind, int invert)
{
    double Voltage;
    const char *name;

    switch (ind)
    {
    case V06:
        Voltage = 0.6;
        name = "V0.6";
        break;

    case VREF:
        Voltage = _vrcon->get_Vref();
        name = "Vref";
        break;

    case NO_IN:
        Voltage = invert ? p_cpu->get_Vdd() : 0.0;
        name = "No_IN";
        break;

    default:
        Voltage = cm_input[ind]->getPin()->get_nodeVoltage();
        name = cm_input[ind]->getPin()->name().c_str();
        break;
    }
    if (name)	// this is just to avoid a compiler warning
    {
        Dprintf(("CMCON::comp_voltage %s ind=%d IN%c %.2f %s\n", name, ind, invert?'-':'+', Voltage, name));
    }
    return Voltage;
}


/*
**      get()
**              read the comparator inputs and set C2OUT and C1OUT
**              as required. Also drive output pins if required.
*/
unsigned int CMCON::get()
{
    unsigned int cmcon_val = value.get();
    int mode = cmcon_val & 0x07;
    int i;

    for (i = 0; i < 2; i++)
    {
        int invert_bit = (i == 0) ? C1INV : C2INV;
        int output_bit = (i == 0) ? C1OUT : C2OUT;
        int shift = (cmcon_val & CIS) ? CFG_SHIFT : CFG_SHIFT * 3;

        if ((m_configuration_bits[i][mode] & CFG_MASK) != ZERO)
        {
            bool out_true;
            int out;
            double Vhigh = comp_voltage(
                               (m_configuration_bits[i][mode] >> shift) & CFG_MASK,
                               cmcon_val & invert_bit);
            double Vlow = comp_voltage(
                              (m_configuration_bits[i][mode] >> (shift + CFG_SHIFT)) & CFG_MASK,
                              (cmcon_val & invert_bit) == 0);

            if (Vhigh > Vlow)
            {
                out_true = (cmcon_val & invert_bit) ? false : true;

            }
            else
            {
                out_true = (cmcon_val & invert_bit) ? true : false;
            }

            if (out_true)
            {
                cmcon_val |= output_bit;

            }
            else
            {
                cmcon_val &= ~output_bit;
            }

            if ((out = m_configuration_bits[i][mode] & CFG_MASK) < 2)
            {
                cm_source[out]->putState(out_true);
                cm_output[out]->updatePinModule();
            }

        }
        else                    // Don't care about inputs, register value 0
        {
            cmcon_val &= ~output_bit;
        }
    }

    if (value.get() ^ cmcon_val)   // change of state
    {
        int diff = value.get() ^ cmcon_val;

        // Signal ECCPAS ?
        if (m_eccpas)
        {
            if (diff & C1OUT)
            {
                m_eccpas->c1_output(cmcon_val & C1OUT);
            }

            if (diff & C2OUT)
            {
                m_eccpas->c2_output(cmcon_val & C2OUT);
            }
        }

        // Generate interupt ?
        if (pir_set)
        {
            if (diff & C1OUT)
            {
                pir_set->set_c1if();
            }

            if (diff & C2OUT)
            {
                pir_set->set_c2if();
            }
        }
    }

    if (m_tmrl)
    {
        m_tmrl->compare_gate((cmcon_val & C1OUT) == C1OUT);
    }

    value.put(cmcon_val);
    return cmcon_val;
}


void CMCON::put(unsigned int new_value)
{
    unsigned int mode = new_value & 0x7;
    unsigned int in_mask = 0;
    unsigned int out_mask = 0;
    int i;


    if (verbose)
    {
        std::cout << "CMCON::put(new_value) =" << std::hex << new_value << '\n';
    }

    trace.raw(write_trace.get() | value.get());

    // Determine used input and output pins
    for (i = 0; i < 2; i++)
    {
        unsigned int configuration = m_configuration_bits[i][mode];

        if ((configuration & CFG_MASK) < 2)
        {
            out_mask |= (1 << (configuration & CFG_MASK));
        }

        for (int j = 0; j < 4; j++)
        {
            configuration >>= CFG_SHIFT;

            if ((configuration & CFG_MASK) < 6)
            {
                in_mask |= (1 << (configuration & CFG_MASK));
            }
        }
    }

    if (verbose)
    {
        std::cout << "CMCON::put in_mask=" << in_mask << " out_mask=" << out_mask << '\n';
    }

    if ((mode != 0) && (mode != 7) && ! cm_stimulus[0])   // initialize stimulus
    {
        cm_stimulus[0] = new CM_stimulus(this, "cm_stimulus_1", 0, 1e12);
        cm_stimulus[1] = new CM_stimulus(this, "cm_stimulus_2", 0, 1e12);
        cm_stimulus[2] = new CM_stimulus(this, "cm_stimulus_3", 0, 1e12);
        cm_stimulus[3] = new CM_stimulus(this, "cm_stimulus_4", 0, 1e12);
    }

    //
    // setup outputs
    //
    for (i = 0; i < 2 && cm_output[i]; i++)
    {
        if (out_mask & (1 << i))
        {
            char name[20];

            if (! cm_source[i])
            {
                cm_source[i] = new CMSignalSource(this, i);
            }

            snprintf(name, sizeof(name), "c%dout", i + 1);
            cm_output[i]->getPin()->newGUIname(name);
            cm_output[i]->setSource(cm_source[i]);
            cm_source_active[i] = true;

        }
        else if (cm_source_active[i])
        {
            cm_output[i]->getPin()->newGUIname(cm_output[i]->getPin()->name().c_str());
            cm_output[i]->setSource(0);
        }
    }

    //
    // setup inputs
    for (i = 0; i < 4 ; i++)
    {
        if (cm_input[i])
        {
            const char *name = cm_input[i]->getPin()->GUIname().c_str();

            if (cm_input[i]->getPin()->snode)
            {
                if (in_mask & (1 << i))
                {
                    (cm_input[i]->getPin()->snode)->attach_stimulus(cm_stimulus[i]);

                }
                else
                {
                    (cm_input[i]->getPin()->snode)->detach_stimulus(cm_stimulus[i]);
                }
            }

            // rewrite GUI name as required
            if (in_mask & (1 << i))
            {
                cm_input[i]->AnalogReq(this, true, cm_an[i].c_str());

            }
            else
            {
                if (!strncmp(name, "an", 2))
                {
                    cm_input[i]->AnalogReq(this, false, cm_input[i]->getPin()->name().c_str());
                }
            }
        }
    }

    // if only one comparator,  mask C2INV
    if (!cm_output[1])
    {
        new_value &= 0x1f;
    }

    value.put(new_value);

    if (verbose)
    {
        std::cout << "CMCON::put() val=0x" << std::hex << new_value << '\n';
    }

    get();        // update comparator values
}


//------------------------------------------------------------------------
CMCON1::CMCON1(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc), valid_bits(0x3)
{
}


CMCON1::~CMCON1()
{
}


void CMCON1::put(unsigned int new_value)
{
    if (verbose)
    {
        std::cout << "CMCON1::put(new_value) =" << std::hex << new_value << '\n';
    }

    assert(m_tmrl);
    m_tmrl->set_T1GSS(new_value & T1GSS);
    trace.raw(write_trace.get() | value.get());
    value.put(new_value & valid_bits);
}


//------------------------------------------------------------------------
SRCON::SRCON(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc), set(false), reset(false)
{
    writable_bits = SR1 | SR0 | C1SEN | C2REN | FVREN;
    SR_Q = false;
}


void SRCON::put(unsigned int new_value)
{
    // PULSR and PULSS should be only settable using bsf
    // it is not clear if these bits read anything other than 0,
    // but this is what I am assuming RRR
    if (new_value & PULSR)
    {
        SR_Q = false;

    }
    else if (new_value & PULSS && ! reset)
    {
        SR_Q = true;
    }

    trace.raw(write_trace.get() | value.get());
    value.put(new_value & writable_bits);
}

void SRCON::set_sr_out(int cm, bool output, bool out_enable, bool output_enable, CMxSignalSource   *cm_source)
{
    RRprint((stderr, "SRCON::set_sr_out\n"));

}


CMxCON0_base::CMxCON0_base(Processor *pCpu, const char *pName,
                           const char *pDesc, unsigned int _cm, ComparatorModule2 *cmModule)
    : sfr_register(pCpu, pName, pDesc), cm(_cm), m_cmModule(cmModule)
{
    value.put(0);
    cm_input[0] = cm_input[1] = cm_input[2] = cm_input[3] = cm_input[4] = nullptr;
    cm_stimulus[0] = cm_stimulus[1] = nullptr;
    cm_snode[0] = cm_snode[1] = nullptr;
}


CMxCON0_base::~CMxCON0_base()
{
    if (cm_source_active && cm_output)
    {
        cm_output->setSource(0);
    }

    delete cm_source;

    if ((!cm_snode[0]) && cm_stimulus[0])
    {
        delete cm_stimulus[0];
    }

    if ((!cm_snode[1]) && cm_stimulus[1])
    {
        delete cm_stimulus[1];
    }

    delete IntSrc;
}


//
// evaluate inputs and determine output
//
unsigned int CMxCON0_base::get()
{
    bool output;

    if (! is_on())
    {
        // need to test what happens in a real device RRR
        output = out_invert()?true:false;
        //output = false;

    }
    else
    {
        double Vpos = get_Vpos();
        double Vneg = get_Vneg();
        output = output_high();

        if (fabs(Vpos - Vneg) > get_hysteresis())
        {
            output = Vpos > Vneg;
            if (out_invert())
                output = !output;
        }
        else if (old_invert() != out_invert())
        {
            output = !output;
        }
        Dprintf(("%s %s ON Vpos %.2f Vneg %.2f output %d invert %d\n", __FUNCTION__, name().c_str(), Vpos, Vneg, output, out_invert()));
    }


    set_output(output);
    return value.get();
}


//--------------------------------------------------
//      Voltage reference
//--------------------------------------------------

VRCON::VRCON(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc)
{
    valid_bits = VR0 | VR1 | VR2 | VR3 | VRR | VROE | VREN;
    value.put(0);
}


VRCON::~VRCON()
{
}


void VRCON::setIOpin(PinModule *newPinModule)
{
    if (newPinModule == nullptr)
    {
        return;
    }

    vr_PinModule = newPinModule;
    pin_name = newPinModule->getPin()->name();
}


double VRCON::get_Vref()
{
    unsigned int new_value = value.get();
    Vref_high = p_cpu->get_Vdd();
    Vref_low = 0.0;
    vr_Rhigh = (8 + (16 - (new_value & 0x0f))) * 2000.0;
    vr_Rlow = (new_value & 0x0f) * 2000.0;

    if (!(new_value & VRR))         // High range ?
    {
        vr_Rlow += 16000.0;
    }

    vr_Vref = (Vref_high - Vref_low) * vr_Rlow / (vr_Rhigh + vr_Rlow) + Vref_low;

    if (verbose)
    {
        std::cout << "VRCON::put Rhigh=" << vr_Rhigh << " Rlow=" << vr_Rlow
                  << " Vout=" << vr_Vref << '\n';
    }

    return vr_Vref;
}


void VRCON::put(unsigned int new_value)
{
    new_value &= valid_bits;
    unsigned int old_value = value.get();
    unsigned int diff = new_value ^ old_value;
    trace.raw(write_trace.get() | value.get());

    if (verbose & 2)
    {
        std::cout << "VRCON::put old=" << std::hex << old_value << " new=" << new_value << '\n';
    }

    if (!diff)
    {
        return;
    }

    // if no PinModule clear VROE bit
    if (!vr_PinModule)
    {
        new_value &= ~VROE;
    }

    value.put(new_value);

    if (new_value & VREN)         // Vreference enable set
    {
        get_Vref();

        if (new_value & VROE)   // output voltage to pin
        {
            if (! vr_pu)
            {
                vr_pu = new stimulus("vref_pu", Vref_high, vr_Rhigh);
            }

            if (! vr_pd)
            {
                vr_pd = new stimulus("vref_pd", Vref_low, vr_Rlow);
            }

            if (strcmp("Vref", vr_PinModule->getPin()->name().c_str()))
            {
                vr_PinModule->getPin()->newGUIname("Vref");
            }

            if (vr_PinModule->getPin()->snode)
            {
                vr_pu->set_Zth(vr_Rhigh);
                vr_pd->set_Zth(vr_Rlow);
                vr_PinModule->getPin()->snode->attach_stimulus(vr_pu);
                vr_PinModule->getPin()->snode->attach_stimulus(vr_pd);
                vr_PinModule->getPin()->snode->update();
            }

        }
        else if (vr_PinModule)     // not outputing voltage to pin
        {
            if (!strcmp("Vref", vr_PinModule->getPin()->name().c_str()))
            {
                vr_PinModule->getPin()->newGUIname(pin_name.c_str());
            }

            if (diff & 0x2f)    // did value of vreference change ?
            {
                _cmcon->get();
            }

            if (vr_PinModule && vr_PinModule->getPin()->snode)
            {
                vr_PinModule->getPin()->snode->detach_stimulus(vr_pu);
                vr_PinModule->getPin()->snode->detach_stimulus(vr_pd);
                vr_PinModule->getPin()->snode->update();
            }

        }
        else  	// output pin not defined
        {
            if (diff & 0x2f)    // did value of vreference change ?
            {
                _cmcon->get();
            }
        }

    }
    else     // vref disable
    {
        if (vr_PinModule && !strcmp("Vref", vr_PinModule->getPin()->name().c_str()))
        {
            vr_PinModule->getPin()->newGUIname(pin_name.c_str());
        }

        if (vr_PinModule && vr_PinModule->getPin()->snode)
        {
            vr_PinModule->getPin()->snode->detach_stimulus(vr_pu);
            vr_PinModule->getPin()->snode->detach_stimulus(vr_pd);
            vr_PinModule->getPin()->snode->update();
        }
    }
}


//--------------------------------------------------
//      Voltage reference
//--------------------------------------------------

VRCON_2::VRCON_2(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc)
{
    value.put(0);
    vr_06v = new stimulus("vref_06v", 0.0, 100.0);
    vr_pu = new stimulus("Cvref_pu", 0.0, 48000.0);
    vr_pd = new stimulus("Cvref_pd", 0.0, 0.0);
    p_cpu->CVREF = new Stimulus_Node("CVREF");
    p_cpu->V06REF = new Stimulus_Node("V0.6REF");
    p_cpu->CVREF->attach_stimulus(vr_pu);
    p_cpu->CVREF->attach_stimulus(vr_pd);
    p_cpu->V06REF->attach_stimulus(vr_06v);
}


VRCON_2::~VRCON_2()
{
    delete vr_06v;
    delete vr_pu;
    delete vr_pd;
    delete p_cpu->CVREF;
    delete p_cpu->V06REF;
    p_cpu->CVREF = nullptr;
}


void VRCON_2::put(unsigned int new_value)
{
    unsigned int old_value = value.get();
    unsigned int diff = new_value ^ old_value;

    if (verbose & 2)
    {
        std::cout << "VRCON_2::put old=" << std::hex << old_value << " new=" << new_value << '\n';
    }

    if (!diff)
    {
        return;
    }

    trace.raw(write_trace.get() | value.get());
    value.put(new_value);

    // Turn 0.6 V reference on or off ?
    if (diff & VP6EN)
    {
        if (new_value & VP6EN)
        {
            vr_06v->set_Vth(0.6);

        }
        else
        {
            vr_06v->set_Vth(0.0);
        }

        p_cpu->V06REF->update();
    }

    if (diff & (C1VREN | C2VREN | VRR | VR3 | VR2 | VR1 | VR0))
    {
        double vr_Rhigh, vr_Rlow;

        if (new_value & (C1VREN | C2VREN))
        {
            vr_pu->set_Vth(p_cpu->get_Vdd());

        }
        else
        {
            vr_pu->set_Vth(0.0);
        }

        vr_Rhigh = (8 + (16 - (new_value & 0x0f))) * 2000.0;
        vr_pu->set_Zth(vr_Rhigh);
        vr_Rlow = (new_value & 0x0f) * 2000.0;

        if (!(new_value & VRR))         // High range ?
        {
            vr_Rlow += 16000.0;
        }

        vr_pd->set_Zth(vr_Rlow);
        p_cpu->CVREF->update();
        p_cpu->CVREF->update();
    }
}


CMxCON0_PPS::CMxCON0_PPS(Processor *pCpu, const char *pName,
                         const char *pDesc, unsigned int _cm, ComparatorModule2 *cmModule)
    : CMxCON0(pCpu, pName, pDesc, _cm, cmModule)
{
}


void CMxCON0_PPS::put(unsigned int new_value)
{
    unsigned int old_value = value.get();
    // assume masked bits are read-only
    new_value = (new_value & mValidBits) | (old_value & ~mValidBits);
    trace.raw(write_trace.get() | value.get());
    value.put(new_value);

    new_pin(cm_output, cm_output);
    get();
}

void CMxCON0_PPS::new_pin(PinModule *old_pin, PinModule *new_pin)
{
    bool enable = value.get() & CxON;

    if (old_pin && !new_pin && cm_source_active)
    {
        old_pin->getPin()->newGUIname(old_pin->getPin()->name().c_str());
        old_pin->setSource(0);
        cm_source_active = false;
        delete cm_source;
        cm_source = nullptr;
    }
    if (enable)
    {
        if (!cm_source_active && new_pin)
        {
            char name[20];

            if (! cm_source)
                cm_source = new CMxSignalSource(new_pin, this);

            snprintf(name, sizeof(name), "c%uout", cm + 1);
            new_pin->getPin()->newGUIname(name);
            new_pin->setSource(cm_source);
            cm_source_active = true;
            cm_source->putState((value.get() & CxOUT) ? '1' : '0');
            new_pin->updatePinModule();
        }
    }
    if ((!enable || !new_pin) && cm_source_active)  	// Enable output enable turned off
    {
        new_pin->getPin()->newGUIname(new_pin->getPin()->name().c_str());
        new_pin->setSource(0);
        new_pin->updatePinModule();
        cm_source_active = false;
    }
}
void CMxCON0_PPS::setIOpin(PinModule *pin_cm, int arg)
{
    if (cm_output != pin_cm)
    {
        new_pin(cm_output, pin_cm);
        cm_output = pin_cm;
    }
}
void CMxCON0_PPS::set_output(bool output)
{
    unsigned int v_cmxcon0 = value.get();
    bool old_out = v_cmxcon0 & CxOUT;

    if (output)
    {
        v_cmxcon0 |= CxOUT;

    }
    else
    {
        v_cmxcon0 &= ~CxOUT;
    }

    Dprintf(("%s cm%u POL %d output %d cmxcon0=%x old_out %d\n", __FUNCTION__, cm + 1, (bool)(v_cmxcon0 & CxPOL), output, v_cmxcon0, old_out));
    value.put(v_cmxcon0);
    m_cmModule->set_cmout(cm, output);

    if (cm_output && cm_source)
    {
        cm_source->putState(output ? '1' : '0');
        cm_output->updatePinModule();
    }

    if (old_out != output)   // state change
    {
        // Positive going edge, set interrupt ?
        if (output && (m_cmModule->cmxcon1[cm]->value.get() & CMxCON1::CxINTP))
        {
            IntSrc->Trigger();
        }

        // Negative going edge, set interrupt ?
        if (!output && (m_cmModule->cmxcon1[cm]->value.get() & CMxCON1::CxINTN))
        {
            IntSrc->Trigger();
        }
    }
}

CMxCON0::CMxCON0(Processor *pCpu, const char *pName,
                 const char *pDesc, unsigned int _cm, ComparatorModule2 *cmModule)
    : CMxCON0_base(pCpu, pName, pDesc, _cm, cmModule)
{
}


CMxCON0::~CMxCON0()
{
}


void CMxCON0::put(unsigned int new_value)
{
    unsigned int old_value = value.get();
    unsigned int diff = (new_value ^ old_value) & mValidBits;
    // assume masked bits are read-only
    new_value = (new_value & mValidBits) | (old_value & ~mValidBits);
    trace.raw(write_trace.get() | value.get());
    value.put(new_value);


    if (diff == 0)
    {
        get();
        return;
    }

    if (diff & CxOE)
    {
            cm_output = m_cmModule->cmxcon1[cm]->output_pin();

            if (new_value & CxOE)
            {
                char name[20];

                if (! cm_source)
                {
                    cm_source = new CMxSignalSource(cm_output, this);
                }

                snprintf(name, sizeof(name), "c%uout", cm + 1);
                assert(cm_output);
                cm_output->getPin()->newGUIname(name);
                cm_output->setSource(cm_source);
                cm_source_active = true;

            }
            else if (cm_source_active)  	// Enable output enable turned off
            {
                cm_output->getPin()->newGUIname(cm_output->getPin()->name().c_str());
                cm_output->setSource(0);
                cm_source_active = false;
            }
    }
    get();
}


double CMxCON0::get_hysteresis()
{
    double ret = 0.0;

    if (value.get() & CxHYS)
    {
        ret = 0.05;
    }

    return ret;
}


double CMxCON0::get_Vpos()
{
    return m_cmModule->cmxcon1[cm]->get_Vpos();
}


double CMxCON0::get_Vneg()
{
    return m_cmModule->cmxcon1[cm]->get_Vneg();
}


void CMxCON0::set_output(bool output)
{
    unsigned int v_cmxcon0 = value.get();
    bool old_out = v_cmxcon0 & CxOUT;

    if (output)
    {
        v_cmxcon0 |= CxOUT;

    }
    else
    {
        v_cmxcon0 &= ~CxOUT;
    }

    Dprintf(("cm%u POL %d output %d v_cmxcon0=%x old_out %d\n", cm + 1, (bool)(v_cmxcon0 & CxPOL), output, v_cmxcon0, old_out));
    value.put(v_cmxcon0);
    m_cmModule->set_cmout(cm, output);

    if (v_cmxcon0 & CxOE)
    {
        cm_source->putState(output ? '1' : '0');
        m_cmModule->cmxcon1[cm]->output_pin()->updatePinModule();
    }

    if (old_out != output)   // state change
    {
        // Positive going edge, set interrupt ?
        if (output && (m_cmModule->cmxcon1[cm]->value.get() & CMxCON1::CxINTP))
        {
            IntSrc->Trigger();
        }

        // Negative going edge, set interrupt ?
        if (!output && (m_cmModule->cmxcon1[cm]->value.get() & CMxCON1::CxINTN))
        {
            IntSrc->Trigger();
        }
    }
}


void CMxCON0_V2::put(unsigned int new_value)
{
    bool         old_out = value.get() & CxOUT;
    unsigned int diff = (new_value ^ value.get()) & mValidBits;

    new_value &= mValidBits;

    old_value = value.get();

    if (verbose)
    {
        std::cout << name() << " put(new_value) =" << std::hex << new_value << '\n';
    }


    // assume masked bits are read-only

    if (diff == 0)
    {
        get();
        return;
    }

    trace.raw(write_trace.get() | value.get());
    value.put(old_out ? new_value | CxOUT : new_value);
#ifdef RRR
    if (is_on() && (diff & CxPOL))
    {
        old_out = !old_out;
    }
#endif

    if ((diff & CxON) && !(new_value & CxON))   // turning off
    {
        cm_output = m_cmModule->cmxcon1[cm]->output_pin(cm);
        cm_output->getPin()->newGUIname(cm_output->getPin()->name().c_str());
        cm_output->setSource(0);
        // remove stimulus from input pins
        m_cmModule->cmxcon1[0]->setPinStimulus(0, POS + cm * 2);
        m_cmModule->cmxcon1[0]->setPinStimulus(0, NEG + cm * 2);
        return;
    }


    if (diff & CxOE)
    {
//RRR    fprintf(stderr, "RRR %s sr_module=%p cxoutput=%d new_value=0x%x\n", name().c_str(), m_cmModule->sr_module, m_cmModule->sr_module->cxoutput, new_value);
        if (m_cmModule && m_cmModule->sr_module && m_cmModule->sr_module->cxoutput)
        {
            m_cmModule->sr_module->set_cxoen(cm, new_value & CxOE);
	    get();
	    return;
        }
        cm_output = m_cmModule->cmxcon1[cm]->output_pin(cm);

        if (new_value & CxOE)
        {
            char name[20];

            if (!cm_source && cm_output)
            {
                cm_source = new CMxSignalSource(cm_output, this);
            }


            snprintf(name, sizeof(name), "c%uout", cm + 1);
            assert(cm_output);
            cm_output->getPin()->newGUIname(name);
            cm_output->setSource(cm_source);
            cm_source_active = true;

        }
        else if (cm_source_active)  	// Enable output enable turned off
        {
            cm_output->getPin()->newGUIname(cm_output->getPin()->name().c_str());
            cm_output->setSource(0);
        }
    }

    get();
}


void CMxCON0_V2::set_output(bool output)
{
    unsigned int v_cmxcon0 = value.get();
    unsigned int cmxcon1 = m_cmModule->cmxcon1[cm]->value.get();
    bool old_out = v_cmxcon0 & CxOUT;


    if (m_eccpas)
    {
        if (cm == 0)
            m_eccpas->c1_output(output);
        else if (cm == 1)
            m_eccpas->c2_output(output);
    }
    if (output)
    {
        v_cmxcon0 |= CxOUT;
        cmxcon1 |= ((cm == 0) ? CM2CON1_V2::MC1OUT : CM2CON1_V2::MC2OUT);
    }
    else
    {
        v_cmxcon0 &= ~CxOUT;
        cmxcon1 &= ~((cm == 0) ? CM2CON1_V2::MC1OUT : CM2CON1_V2::MC2OUT);
    }

    Dprintf(("cm%u POL %d output %d v_cmxcon0=%x old_out %d\n", cm + 1, (bool)(v_cmxcon0 & CxPOL), output, v_cmxcon0, old_out));
    value.put(v_cmxcon0);
    m_cmModule->cmxcon1[cm]->value.put(cmxcon1);
    m_cmModule->set_cmout(cm, output);

    RRprint((stderr, "m_srcon=%p\n", m_srcon));
    if (m_srcon)
    {
//	 m_srcon->set_sr_out(cm, output, v_cmxcon0 & CxOE, cm_source);
        m_cmModule->cmxcon1[cm]->output_pin(cm)->updatePinModule();
    }
    else if (cm_source && (v_cmxcon0 & CxOE))
    {
        cm_source->putState(output ? '1' : '0');
        m_cmModule->cmxcon1[cm]->output_pin(cm)->updatePinModule();
    }

    if (old_out != output)   // state change
    {
        m_cmModule->cmxcon1[cm]->tmr_gate(cm, output);

        // Positive going edge, set interrupt ?
        if (output)
        {
            IntSrc->Trigger();
        }
    }
}


double CMxCON0_V2::get_hysteresis()
{
    double hyst_volt = 0.0;

    if (m_cmModule->cmxcon1[cm]->hyst_active(cm))
    {
        hyst_volt =  0.05; // assume 50 mv hysteresis
    }

    return hyst_volt;
}


CMxCON0_V2::CMxCON0_V2(Processor *pCpu, const char *pName,
                       const char *pDesc, unsigned int _cm, ComparatorModule2 *cmModule)
    : CMxCON0_base(pCpu, pName, pDesc, _cm, cmModule)
{
    stimulus_pin[0] = nullptr;
    stimulus_pin[1] = nullptr;
}


CMxCON0_V2::~CMxCON0_V2()
{
}


double CMxCON0_V2::get_Vpos()
{
    return m_cmModule->cmxcon1[cm]->get_Vpos(cm, value.get());
}


double CMxCON0_V2::get_Vneg()
{
    return m_cmModule->cmxcon1[cm]->get_Vneg(cm, value.get());
}


void CM2CON1_V4::put(unsigned int new_value)
{
    trace.raw(write_trace.get() | value.get());
    value.put(new_value & mValidBits);

    if (m_cmModule->tmr1l[0])
    {
        m_cmModule->tmr1l[0]->set_T1GSS((new_value & T1GSS) == T1GSS);
    }
}


bool CM2CON1_V4::hyst_active(unsigned int cm)
{
    bool hyst = false;

    if (cm == 0)
    {
        hyst = value.get() & C1HYS;

    }
    else if (cm == 1)
    {
        hyst = value.get() & C2HYS;
    }

    RRprint((stderr, "cm%d ret=%d\n", cm+1, hyst));
    return hyst;
}

double CM2CON1_V4::get_Vpos(unsigned int cm, unsigned int cmxcon0)
{
    double Voltage = 0.0;
    assert(m_vrcon);

    if (cmxcon0 & CMxCON0_V2::CxR)   // use Vref defined in cm2con1
    {
        RRprint((stderr, "cm=%d vrcon=0x%x\n", cm+1, m_vrcon->value.get()));
        if ((cm == 0 && (m_vrcon->value.get() & VRCON_2::C1VREN)) ||
                (cm == 1 && (m_vrcon->value.get() & VRCON_2::C2VREN)))
        {
            Voltage = p_cpu->CVREF->get_nodeVoltage();
            Dprintf(("%s CVref %.2f\n", __FUNCTION__, Voltage));

        }
        else
        {
            Voltage = p_cpu->V06REF->get_nodeVoltage();
            Dprintf(("%s cm%u V06ref %.2f\n", __FUNCTION__, cm + 1, Voltage));
        }

    }
    else  	// use CM1IN+ or CM2IN+
    {

        unsigned int pos = cm == 0? 1: 3;
        if (stimulus_pin[pos] != cm_inputPos[cm])
        {
            setPinStimulus(cm_inputPos[cm], pos);
        }

        Voltage =  cm_inputPos[cm]->getPin()->get_nodeVoltage();
        Dprintf(("%s cm%u %s %.2f\n", __FUNCTION__, cm + 1, cm_inputPos[cm]->getPin()->name().c_str(), Voltage));
    }

    return Voltage;
}


CM2CON1_V4::CM2CON1_V4(Processor *pCpu, const char *pName, const char *pDesc,
                       unsigned int _cm, ComparatorModule2 * cmModule) :
    CM2CON1_V3(pCpu, pName, pDesc, cmModule)
{
    cm1_cvref = new CM_stimulus((CMCON *)m_cmModule->cmxcon0[0], "cm1_cvref", 0, 1e12);
    cm1_v06ref = new CM_stimulus((CMCON *)m_cmModule->cmxcon0[0], "cm1_v06ref", 0, 1e12);
    cm2_cvref = new CM_stimulus((CMCON *)m_cmModule->cmxcon0[1], "cm2_cvref", 0, 1e12);
    cm2_v06ref = new CM_stimulus((CMCON *)m_cmModule->cmxcon0[1], "cm2_v06ref", 0, 1e12);
    p_cpu->CVREF->attach_stimulus(cm1_cvref);
    p_cpu->V06REF->attach_stimulus(cm1_v06ref);
    p_cpu->CVREF->attach_stimulus(cm2_cvref);
    p_cpu->V06REF->attach_stimulus(cm2_v06ref);
}


CM2CON1_V4::~CM2CON1_V4()
{

    delete cm1_cvref;
    delete cm1_v06ref;
    delete cm2_cvref;
    delete cm2_v06ref;
}


void CM2CON1_V3::put(unsigned int new_value)
{
    unsigned int old_value = value.get();
    trace.raw(write_trace.get() | value.get());
    value.put(new_value & mValidBits);

    if ((new_value ^ old_value) & C1RSEL)
    {
        m_cmModule->cmxcon0[0]->get();
    }

    if ((new_value ^ old_value) & C2RSEL)
    {
        m_cmModule->cmxcon0[1]->get();
    }

    if (m_cmModule->tmr1l[0])
    {
        m_cmModule->tmr1l[0]->set_T1GSS((new_value & T1GSS) == T1GSS);
    }
}


double CM2CON1_V3::get_Vpos(unsigned int cm, unsigned int cmxcon0)
{
    double Voltage = 0.0;
    unsigned int cmxcon1 = value.get();
    assert(m_vrcon);

    if (cmxcon0 & CMxCON0_V2::CxR)
    {
        if ((cm == 0 && (cmxcon1 & C1RSEL)) |
                (cm == 1 && (cmxcon1 & C2RSEL)))
        {
            Voltage =  m_vrcon->get_Vref();
            Dprintf(("%s cm%u Vref %.2f\n", __FUNCTION__, cm + 1, Voltage));

        }
        else
        {
            Voltage =  0.6;
            Dprintf(("%s cm%u Absref %.2f\n", __FUNCTION__, cm + 1, Voltage));
        }


    }
    else  	// use CM1IN+ or CM2IN+
    {
        unsigned int pos = cm == 0? 1: 3;
        if (stimulus_pin[pos] != cm_inputPos[cm])
        {
            setPinStimulus(cm_inputPos[cm], pos);
        }

        Voltage =  cm_inputPos[cm]->getPin()->get_nodeVoltage();
        Dprintf(("%s cm%u %s %.2f\n", __FUNCTION__, cm + 1, cm_inputPos[cm]->getPin()->name().c_str(), Voltage));
    }

    return Voltage;
}


double CM2CON1_V3::get_Vneg(unsigned int cm, unsigned int cmxcon0)
{
    unsigned int cxNchan = cmxcon0 & (CMxCON0_V2::CxCH0 | CMxCON0_V2::CxCH1);

    unsigned int neg = NEG + cm * 2;

    if (stimulus_pin[neg] != cm_inputNeg[cxNchan])
    {
        setPinStimulus(cm_inputNeg[cxNchan], neg);
    }

    Dprintf(("%s cm%u pin %u %s %.2f\n", __FUNCTION__, cm + 1, cxNchan, cm_inputNeg[cxNchan]->getPin()->name().c_str(), cm_inputNeg[cxNchan]->getPin()->get_nodeVoltage()));
    return cm_inputNeg[cxNchan]->getPin()->get_nodeVoltage();
}


//*************************************************************
// CM2CON1_V2

void CM2CON1_V2::put(unsigned int new_value)
{
    unsigned int old_value = value.get();
    new_value &= mValidBits;
    unsigned int diff = old_value ^ new_value;
    trace.raw(write_trace.get() | value.get());
    value.put(new_value);

    if (diff & (C1RSEL | C1HYS))
    {
        m_cmModule->cmxcon0[0]->get();
    }

    if (diff & (C2RSEL | C2HYS))
    {
        m_cmModule->cmxcon0[1]->get();
    }
}


CM2CON1_V3::CM2CON1_V3(Processor *pCpu, const char *pName, const char *pDesc,
                       ComparatorModule2 * cmModule) :
    CMxCON1_base(pCpu, pName, pDesc, 0, cmModule)
{
    assert(m_cmModule->cmxcon0[1]);
    cm_stimulus[2] = new CM_stimulus((CMCON *)m_cmModule->cmxcon0[1], "cm_stimulus_2-", 0, 1e12);
    cm_stimulus[3] = new CM_stimulus((CMCON *)m_cmModule->cmxcon0[1], "cm_stimulus_2+", 0, 1e12);
}

CM2CON1_V3::~CM2CON1_V3()
{
    delete cm_stimulus[2];
    delete cm_stimulus[3];
}

void CM2CON1_V3::tmr_gate(unsigned int cm, bool output)
{
    if (cm == 1 && m_cmModule->tmr1l[0])   //CM2
    {
        Dprintf(("CM2CON1_V3::tmr_gate cm%u output=%d\n", cm + 1, output));
        m_cmModule->tmr1l[0]->compare_gate(output);
    }
}


CM2CON1_V2::CM2CON1_V2(Processor *pCpu, const char *pName,
                       const char *pDesc, ComparatorModule2 * cmModule):
    CMxCON1_base(pCpu, pName, pDesc, 0, cmModule)
{
    assert(m_cmModule->cmxcon0[1]);
    cm_stimulus[2] = new CM_stimulus((CMCON *)m_cmModule->cmxcon0[1], "cm_stimulus_2-", 0, 1e12);
    cm_stimulus[3] = new CM_stimulus((CMCON *)m_cmModule->cmxcon0[1], "cm_stimulus_2+", 0, 1e12);
    ctmu_stimulus_pin = nullptr;
}


CM2CON1_V2::~CM2CON1_V2()
{
    delete cm_stimulus[2];
    delete cm_stimulus[3];
}


double CM2CON1_V2::get_Vpos(unsigned int cm, unsigned int cmxcon0)
{
    double Voltage = 0.0;
    unsigned int cmxcon1 = value.get();

    if (cmxcon0 & CMxCON0_V2::CxR)   // use Vref defined in cm2con1
    {
        if ((cm == 0 && (cmxcon1 & C1RSEL)) |
                (cm == 1 && (cmxcon1 & C2RSEL)))
        {
            Voltage =  m_cmModule->FVR_voltage;
            Dprintf(("%s cm%u FVR %.2f\n", __FUNCTION__, cm + 1, Voltage));

        }
        else
        {
            Voltage =  m_cmModule->DAC_voltage;
            Dprintf(("%s cm%u DAC %.2f\n", __FUNCTION__, cm + 1, Voltage));
        }

    }
    else  	// use CM1IN+ or CM2IN+
    {
        if (stimulus_pin[POS + cm * 2] != cm_inputPos[cm])
        {
            setPinStimulus(cm_inputPos[cm], POS + cm * 2);
        }

        Voltage =  cm_inputPos[cm]->getPin()->get_nodeVoltage();
        Dprintf(("%s cm%u %s %.2f\n", __FUNCTION__, cm + 1, cm_inputPos[cm]->getPin()->name().c_str(), Voltage));
    }

    return Voltage;
}


double CM2CON1_V2::get_Vneg(unsigned int cm, unsigned int cmxcon0)
{
    unsigned int cxNchan = cmxcon0 & (CMxCON0_V2::CxCH0 | CMxCON0_V2::CxCH1);

    if (stimulus_pin[NEG + cm * 2] != cm_inputNeg[cxNchan])
    {
        setPinStimulus(cm_inputNeg[cxNchan], NEG + cm * 2);
    }

    if (cm_inputNeg[cxNchan]->getPin()->snode)
    {
        cm_inputNeg[cxNchan]->getPin()->snode->update();
    }

    Dprintf(("%s cm%u pin %u %s %.2f\n", __FUNCTION__, cm + 1, cxNchan, cm_inputNeg[cxNchan]->getPin()->name().c_str(), cm_inputNeg[cxNchan]->getPin()->get_nodeVoltage()));
    return cm_inputNeg[cxNchan]->getPin()->get_nodeVoltage();
}


bool CM2CON1_V2::hyst_active(unsigned int cm)
{
    bool hyst = false;

    if (cm == 0)
    {
        hyst = value.get() & C1HYS;

    }
    else if (cm == 1)
    {
        hyst = value.get() & C2HYS;
    }

    return hyst;
}


void CM2CON1_V2::tmr_gate(unsigned int cm, bool output)
{
    Dprintf(("CM2CON1_V2::tmr_gate cm%u output %d\n", cm + 1, output));

    for (int i = 0; i < 3; i++)
    {
        if (m_cmModule->t1gcon[i])
        {
            if (cm == 0)   // CM1
            {
                m_cmModule->t1gcon[i]->CM1_gate(output);

            }
            else if (cm == 1)     //CM2
            {
                m_cmModule->t1gcon[i]->CM2_gate(output);
            }
        }
    }
}


void CM2CON1_V2::set_ctmu_stim(stimulus *_ctmu_stim, CTMU *_ctmu_module)
{
    if (_ctmu_stim)
    {
        if (!m_cmModule->ctmu_module)
        {
            m_cmModule->ctmu_module = _ctmu_module;
        }

        ctmu_stim = _ctmu_stim;
        attach_ctmu_stim();

    }
    else
    {
        detach_ctmu_stim();
        ctmu_stim = 0;
    }
}


void CM2CON1_V2::attach_ctmu_stim()
{
    if (!cm_inputNeg[1])
    {
        fprintf(stderr, "ERROR CM2CON1_V2::attach_ctmu_stim C12IN1- not defined\n");
        return;
    }

    if (!(cm_inputNeg[1]->getPin()->snode))
    {
        printf("Warning CM2CON1_V2::attach_ctmu_stim %s has no node attached CTMU will not work properly\n", cm_inputNeg[1]->getPin()->name().c_str());
        return;
    }

    if (ctmu_stim)
    {
        cm_inputNeg[1]->getPin()->snode->attach_stimulus(ctmu_stim);
        cm_inputNeg[1]->getPin()->snode->update();
        ctmu_attached = true;
    }
}


void CM2CON1_V2::detach_ctmu_stim()
{
    if (ctmu_attached)
    {
        cm_inputNeg[1]->getPin()->snode->detach_stimulus(ctmu_stim);
        cm_inputNeg[1]->getPin()->snode->update();
        ctmu_attached = false;
    }
}


CMxCON1::CMxCON1(Processor *pCpu, const char *pName, const char *pDesc, unsigned int _cm, ComparatorModule2 *cmModule)
    : CMxCON1_base(pCpu, pName, pDesc, _cm, cmModule)
{
}


CMxCON1::~CMxCON1()
{
}

double CMxCON1::get_Vpos(unsigned int /* arg */, unsigned int /* arg2 */ )
{
    unsigned int cxPchan = (value.get() & CxPMASK) >> 3;
    double	Voltage = 0.0;
    unsigned int mask = m_cmModule->Pmask[cxPchan];

    if (mask == CM_PIN)
    {
        if(cm_inputPos[cxPchan])
        {
            if (stimulus_pin[POS] != cm_inputPos[cxPchan])
            {
                setPinStimulus(cm_inputPos[cxPchan], POS);
            }

            Voltage = cm_inputPos[cxPchan]->getPin()->get_nodeVoltage();
            Dprintf(("%s %s cxPchan=%u %s v=%.2f\n", __FUNCTION__, name().c_str(), cxPchan, cm_inputPos[cxPchan]->getPin()->name().c_str(), Voltage));
        }
        else
        {
            fprintf(stderr, "Warning: %s cxPchan=%u Input pin not defined\n", name().c_str(), cxPchan);
            Voltage = 0.0;
        }
    }
    else if (mask == CM_UNUSED)
    {
        fprintf(stderr, "Warning: %s %s cxPchan=%u Channel not used\n", __FUNCTION__, name().c_str(), cxPchan);
        for(int i=0; i<8; i++)
            RRprint((stderr, "\ti=%d Pchan=%d Nchan=%d\n", i, m_cmModule->Pmask[i], m_cmModule->Nmask[i]));
        Voltage = 0.0;
    }
    else
    {
        Voltage = m_cmModule->IN_voltage[mask];
        Dprintf(("%s %s cxPchan=%u mask=%u v=%.2f\n", __FUNCTION__, name().c_str(), cxPchan, mask, Voltage));
    }

    return Voltage;
}

double CMxCON1::get_Vneg(unsigned int /* arg */, unsigned int /* arg2 */ )
{
    unsigned int cxNchan = (value.get() & CxNMASK);
    unsigned int mask = m_cmModule->Nmask[cxNchan];
    double	Voltage = 0.0;

    if (mask == CM_PIN)
    {
        if(cm_inputNeg[cxNchan])
        {
            if (stimulus_pin[NEG] != cm_inputNeg[cxNchan])
            {
                setPinStimulus(cm_inputNeg[cxNchan], NEG);
            }

            Voltage = cm_inputNeg[cxNchan]->getPin()->get_nodeVoltage();
            Dprintf(("%s %s cxNchan=%u %s v=%.2f\n", __FUNCTION__, name().c_str(), cxNchan, cm_inputNeg[cxNchan]->getPin()->name().c_str(), Voltage));
        }
        else
        {
            fprintf(stderr, "Warning: %s cxNchan=%u Input pin not defined\n", name().c_str(), cxNchan);
            Voltage = 0.0;
        }
    }
    else if (mask == CM_UNUSED)
    {
        fprintf(stderr, "Warning: %s cxNchan=%u Channel not used\n", name().c_str(), cxNchan);
        Voltage = 0.0;
    }
    else
    {
        Voltage = m_cmModule->IN_voltage[mask];
        Dprintf(("%s %s cxNchan=%u mask=%u v=%.2f\n", __FUNCTION__, name().c_str(), cxNchan, mask, Voltage));
    }

    return Voltage;
}

CMxCON1_base::CMxCON1_base(Processor *pCpu, const char *pName,
                           const char *pDesc, unsigned int _cm, ComparatorModule2 *cmModule)
    : sfr_register(pCpu, pName, pDesc),
      cm(_cm), m_cmModule(cmModule)
{
    assert(m_cmModule->cmxcon0[cm]);
    cm_stimulus[NEG] = new CM_stimulus((CMCON *)m_cmModule->cmxcon0[cm], "cm_stimulus_-", 0, 1e12);
    cm_stimulus[POS] = new CM_stimulus((CMCON *)m_cmModule->cmxcon0[cm], "cm_stimulus_+", 0, 1e12);

    std::fill_n(cm_inputNeg, 5, nullptr);
    std::fill_n(stimulus_pin, 4, nullptr);
    std::fill_n(cm_inputPos, 2, nullptr);
    std::fill_n(cm_output, 2, nullptr);

    ctmu_stimulus_pin = nullptr;
    std::fill_n(cmModule->Pmask, 8, CMxCON1::CM_UNUSED);
    std::fill_n(cmModule->Nmask, 8, CMxCON1::CM_UNUSED);

    cmModule->IN_voltage[CMxCON1::CM_AGND] = 0.0;
}


CMxCON1_base::~CMxCON1_base()
{
    delete cm_stimulus[NEG];
    delete cm_stimulus[POS];
}


// Attach a stimulus to an input pin so that changes
// in the pin voltage can be reflected in the comparator output.
//
// pin may be 0 in which case a current stimulus, if any, will be detached
// pol is either the enum POS or NEG
//
void CMxCON1_base::setPinStimulus(PinModule *pin, int pol)
{
    if (pin == stimulus_pin[pol])
    {
        return;
    }

    if (stimulus_pin[pol])
    {
        (stimulus_pin[pol]->getPin()->snode)->detach_stimulus(cm_stimulus[pol]);
        stimulus_pin[pol] = nullptr;
    }

    if (pin && pin->getPin()->snode)
    {
        stimulus_pin[pol] = pin;
        (stimulus_pin[pol]->getPin()->snode)->attach_stimulus(cm_stimulus[pol]);
    }
}


void CMxCON1::put(unsigned int new_value)
{
    unsigned int old_value = value.get();
    new_value &= mValidBits;
    unsigned int diff = old_value ^ new_value;
    trace.raw(write_trace.get() | value.get());
    value.put(new_value);

    if ((diff & CxNMASK) || !stimulus_pin[NEG])
    {
        unsigned int cxNchan = new_value & CxNMASK;
        setPinStimulus(cm_inputNeg[cxNchan], NEG);
    }

    if ((diff & CxPMASK) || !stimulus_pin[POS])
    {
        unsigned int cxPchan = (new_value & CxPMASK) >> 3;

        if (cxPchan == 0)
        {
            setPinStimulus(cm_inputPos[cxPchan], POS);

        }
        else if (stimulus_pin[POS])
        {
            setPinStimulus(0, POS);
        }
    }

    m_cmModule->run_get(cm);
}


void CMxCON1_base::set_OUTpin(PinModule *pin_cm0, PinModule *pin_cm1)
{
    cm_output[0] = pin_cm0;
    cm_output[1] = pin_cm1;
}

void CMxCON1_base::setIOpin(PinModule *pin_cm, int arg)
{
    cm_output[arg] = pin_cm;
}

#define PIN_N(x) (x?x->getPin()->name().c_str():"null")
void CMxCON1_base::set_INpinNeg(PinModule *pin_cm0, PinModule *pin_cm1, PinModule *pin_cm2,  PinModule *pin_cm3,  PinModule *pin_cm4)
{
    Dprintf(("set_INpinNeg %s %s %s %s %s %s\n", name().c_str(),PIN_N(pin_cm0), PIN_N(pin_cm1), PIN_N(pin_cm2), PIN_N(pin_cm3), PIN_N(pin_cm4)));
    cm_inputNeg[0] = pin_cm0;
    cm_inputNeg[1] = pin_cm1;
    cm_inputNeg[2] = pin_cm2;
    cm_inputNeg[3] = pin_cm3;
    cm_inputNeg[4] = pin_cm4;
}

void CMxCON1_base::set_INpinPos(PinModule *pin_cm0, PinModule *pin_cm1)
{
    Dprintf(("set_INpinPos %s %s %s \n", name().c_str(), PIN_N(pin_cm0), PIN_N(pin_cm1)));
    cm_inputPos[0] = pin_cm0;
    cm_inputPos[1] = pin_cm1;
}


ComparatorModule2::ComparatorModule2(Processor * ) : FVR_ATTACH("comp")
{
    std::fill_n(cmxcon0, 4, nullptr);
    std::fill_n(cmxcon1, 4, nullptr);

    t1gcon[0] = t1gcon[1] = t1gcon[2] = nullptr;
    tmr1l[0] = tmr1l[1] = tmr1l[2] = nullptr;
    eccpas[0] = eccpas[1] = eccpas[2] = nullptr;
    sr_module = nullptr;
    std::fill_n(IN_voltage, 8, 0.0);
    std::fill_n(m_clc, 4, nullptr);
    data_server = new DATA_SERVER(DATA_SERVER::CM);
}


ComparatorModule2::~ComparatorModule2()
{
    for (int i = 0; i < 4; i++)
    {
        delete cmxcon0[i];
        delete cmxcon1[i];

        if (i < 3 && cmxcon1[i] == cmxcon1[i + 1])
        {
            cmxcon1[i + 1] = nullptr;
        }
    }

    delete data_server;
    delete cmout;
}

// this function sets the bits in the CMOUT register and also
// sends the state to the T1GCON class if t1gcon is defined
//
void ComparatorModule2::set_cmout(unsigned int bit, bool value)
{
    int i;

    if (cmout)
    {
        if (value)
        {
            cmout->value.put(cmout->value.get() | (1 << bit));
        }
        else
        {
            cmout->value.put(cmout->value.get() & ~(1 << bit));
        }
    }

    data_server->send_data(value, bit);
    for (i = 0; i < 4; i++)
    {
        if (m_clc[i])
            m_clc[i]->CxOUT_sync(value, bit);
    }
    if (p_cog)
        p_cog->out_Cx(value,bit);

    switch (bit)
    {
    case 0:		//CM1
        for (i = 0; i < 3; i++)
        {
            if (t1gcon[i])
            {
                t1gcon[i]->CM1_gate(value);
            }

            if (eccpas[i])
            {
                eccpas[i]->c1_output(value);
            }
        }

        if (sr_module)
        {
            sr_module->syncC1out(value);
        }

        break;

    case 1:		//CM2
        for (i = 0; i < 3; i++)
        {
            if (t1gcon[i])
            {
                t1gcon[i]->CM2_gate(value);
            }

            if (eccpas[i])
            {
                eccpas[i]->c2_output(value);
            }
        }

        if (sr_module)
        {
            sr_module->syncC2out(value);
        }

        if (ctmu_module)
        {
            ctmu_module->syncC2out(value);
        }

        break;

    default:	//Do nothing other CMs
        break;
    }
}


void ComparatorModule2::set_DAC_volt(double _volt, int cm_index)
{

    IN_voltage[cm_index] = _volt;
    DAC_voltage = _volt;

    for (int i = 0; i < 4; i++)
    {
        if (cmxcon0[i])
        {
            cmxcon0[i]->get();
        }
    }
}


// Called when FVR node is updated
void ComparatorModule2::set_FVR_volt(double _volt, unsigned int chan)
{
    FVR_voltage = _volt;
    IN_voltage[CMxCON1::CM_FVR] = _volt;
    Dprintf(("ComparatorModule2::set_FVR_volt CM_FVR=%u chan=%u %.2f\n", CMxCON1::CM_FVR, chan, FVR_voltage));

    // refresh outputs
    for (int i = 0; i < 4; i++)
    {
        if (cmxcon0[i])
        {
            cmxcon0[i]->get();
        }
    }
}


// set interrupt for comparator cm
void ComparatorModule2::set_if(unsigned int cm)
{
    switch (cm)
    {
    case 0:
        pir_set->set_c1if();
        break;

    case 1:
        pir_set->set_c2if();
        break;

    case 2:
        pir_set->set_c3if();
        break;

    case 3:
        pir_set->set_c4if();
        break;
    }
}
