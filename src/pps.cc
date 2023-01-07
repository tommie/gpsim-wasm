/*
   Copyright (C) 2018 Roy R. Rankin
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

// PERIPHERAL PIN SELECT (PPS) MODULE


//#define DEBUG
#if defined(DEBUG)
#include "../config.h"
#define Dprintf(arg) {printf("%s:%d ",__FILE__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

#include <assert.h>
#include <stdio.h>

#include "pic-processor.h"
#include "processor.h"
#include "stimuli.h"
#include "trace.h"

#include "pps.h"

// Set  pin drive level high
class PPSSignalControl : public SignalControl
{
public:
    explicit PPSSignalControl(PPS_PinModule *_pps_PinModule)
        : pps_PinModule(_pps_PinModule), state('1')
    {
    }
    ~PPSSignalControl() { }
    char getState() override { return state; }
    virtual void putState(char _state)
    {
        state = _state;
    }
    void release()  override { pps_PinModule->ReleasePins();}
private:
    PPS_PinModule *pps_PinModule;
    char state;
};


PPS_PinModule::PPS_PinModule(PinModule *_pinmodule, apfpin *_perf_mod, int _arg) :
    perf_mod(_perf_mod), arg(_arg)
{
    pin_drive = new PPSSignalControl(this);
    setPin(new IOPIN("PPS"));
    perf_mod->setIOpin(this, arg);

    if (_pinmodule)
    {

        Dprintf(("PPS_PinModule::PPS_PinModule %s arg=0x%x\n", _pinmodule->getPin()->name().c_str(), arg));
        add_pinmod(_pinmodule);
    }
}

PPS_PinModule::~PPS_PinModule()
{
    Dprintf(("~PPS_PinModule\n"));
    if (!pin_list.empty())
    {
        for (auto it = pin_list.begin(); it != pin_list.end(); ++it)
            rm_pinmod(it->mod);
    }
    perf_mod->setIOpin(0, arg);
    delete pin_drive;
}
/*
      A module output pin can be connected to multiple physical pins via PPS.
      This module propagates module output pin GUIname, drive level and direction to
      the physical pins when the module calls updatePinModule() on it's output pin.
*/
void PPS_PinModule::updatePinModule()
{
    std::string PPPgui = getPin()->GUIname();
    // Propagate direction and level to physical pins
    //pin_direction->putState(getControlState());
    pin_drive->putState(getSourceState());
    for (auto it = pin_list.begin(); it != pin_list.end(); ++it)
    {
        // Propagate GUIname to physical pins if it has changed
        std::string gui = (it->mod)->getPin()->GUIname();
        // If PPPgui == "PPS" module has turned off GUIname, reset physical GUIname
        if (!PPPgui.compare("PPS"))
        {
            Dprintf(("PPS_PinModule::updatePinModule restore physical pin %s GUIname from %s to %s\n", (it->mod)->getPin()->name().c_str(), (it->mod)->getPin()->GUIname().c_str(), it->GuiName.c_str()));
            (it->mod)->getPin()->newGUIname(it->GuiName.c_str());
        }
        else if (PPPgui.compare(gui))
        {
            Dprintf(("PPS_PinModule::updatePinModule %s change GUIname to %s\n", (it->mod)->getPin()->name().c_str(), PPPgui.c_str()));
            (it->mod)->getPin()->newGUIname(PPPgui.c_str());
        }
        else
        {

        }
        // Propagate updatePinModule() to physical pins
        (it->mod)->updatePinModule();
    }
}

void PPS_PinModule::setControl(SignalControl *pt)
{
    for (auto it = pin_list.begin(); it != pin_list.end(); ++it)
    {
        Dprintf(("PPS_PinModule::setControl %p %s\n", pt, (it->mod)->getPin()->name().c_str()));
        (it->mod)->setControl(pt);
    }
}

void PPS_PinModule::ReleasePins()
{
}


void PPS_PinModule::add_pinmod(PinModule *pinmod)
{
    // Return if pinmod already known by PPS_PinModule
    for (auto it = pin_list.begin(); it != pin_list.end(); ++it)
    {
        if (pinmod == it->mod)
            return;
    }
    if (pinmod->getPin()->is_newGUIname())
        pin_list.push_back({pinmod, pinmod->getPin()->GUIname()});
    else
        pin_list.push_back({pinmod, pinmod->getPin()->name()});

    pinmod->setSource(pin_drive);
    updatePinModule();


    Dprintf(("PPS_PinModule::add_pinmod pimmod=%s %s isNewGUI=%d\n", pinmod->getPin()->name().c_str(), pinmod->getPin()->GUIname().c_str(), pinmod->getPin()->is_newGUIname()));
    pinmod->getPin()->newGUIname(getPin()->GUIname().c_str());
#ifdef DEBUG
    for (auto it = pin_list.begin(); it != pin_list.end(); ++it)
        Dprintf(("\t\t pin=%s %s\n", (it->mod)->getPin()->name().c_str(), (it->GuiName).c_str()));
#endif
}

// Remove pin module from pin list, return true if list is empty
bool PPS_PinModule::rm_pinmod(PinModule *pinmod)
{
    Dprintf(("PPS_PinModule::rm_port pinmod=%s\n", pinmod->getPin()->name().c_str()));
    for (auto it = pin_list.begin(); it != pin_list.end(); ++it)
    {
        if (it->mod == pinmod)
        {
            pinmod->getPin()->newGUIname((it->GuiName).c_str());
            pinmod->setSource(nullptr);
            pinmod->setControl(nullptr);
            pin_list.erase(it);
            break;
        }
    }
    if (pin_list.empty())
    {
        Dprintf(("PPS_PinModule::rm_port pin_list.empty()\n"));
        return true;
    }
    return false;
}

/*
** Select output pin for module
** module output can connect to multiple pins
*/
RxyPPS::RxyPPS(PPS *pt, PinModule *_pin, Processor *pCpu,
               const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc), pt_pps(pt), pin(_pin)
{
    con_mask = 0x1f;
    Dprintf(("RxyPPS::RxyPPS pt_pps=%p %s pin=%s\n", pt_pps, pName, _pin ? _pin->getPin()->name().c_str() : "???"));
}

void RxyPPS::put(unsigned int new_value)
{
    new_value &= con_mask;
    unsigned int old = value.get ();
    // if no change or pps locked, just return
    if (new_value == old || pt_pps->pps_lock)
        return;
    trace.raw(write_trace.get () | value.get ());
    value.put(new_value);
    Dprintf(("RxyPPS::put() %s new_value=0x%x pin=%p(%s)\n", name().c_str(), new_value, pin, pin->getPin()->name().c_str()));
    pt_pps->set_output(this, old, pin);

}


/*
** xxxPPS Select input for peripheral
*/
xxxPPS::xxxPPS(PPS *pt, Processor *pCpu, const char *pName, const char *pDesc,
               unsigned int _mValidBits, apfpin *_perf_mod, int _arg)
    : sfr_register(pCpu, pName, pDesc), con_mask(0),
      pt_pps(pt), perf_mod(_perf_mod), arg(_arg), pin(nullptr)
{
    mValidBits = _mValidBits;
}

void xxxPPS::put(unsigned int new_value)
{
    new_value &= mValidBits;
    PinModule *input_pin = pt_pps->get_input_pin(new_value);
    trace.raw (write_trace.get () | value.get ());
    value.put (new_value);
    if (!input_pin)
    {
        Dprintf(("xxxPPS::put %s new_value=0x%x input_pin=NULL\n", name().c_str(), new_value));
    }

    if (input_pin != pin)
    {
        pin = input_pin;
        Dprintf(("xxxPPS::put %s new_value=0x%x pin=%s\n", name().c_str(), new_value, pin ? pin->getPin()->name().c_str() : "NULL"));
        perf_mod->setIOpin(pin, arg);
    }
}

PPSLOCK::PPSLOCK(PPS *pt, Processor *pCpu,
                 const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc), pt_pps(pt)
{
    con_mask = 0x01;
    state = IDLE;
}

void PPSLOCK::put(unsigned int new_value)
{
    if (new_value == 0x55)
    {
        state = HAVE_55;
        return;
    }
    if (new_value == 0xAA)
    {
        if (state == HAVE_55)
            state = HAVE_AA;
        else
            state = IDLE;
        return;
    }
    // If pps is locked and PPS1WAY is set in config word,
    // then cannot unlock pps.
    Dprintf(("PPSLOCK::put state=%d pps_lock=%d pps1way=%d\n", state, pt_pps->pps_lock, cpu_pic->get_pps1way()));
    if (state == HAVE_AA && (!pt_pps->pps_lock || !cpu_pic->get_pps1way()))
    {
        if ((new_value ^ value.get()) & con_mask)
        {
            new_value &= con_mask;
            trace.raw (write_trace.get () | value.get ());
            value.put (new_value);
            if (new_value)
                pt_pps->pps_lock = true;
            else
                pt_pps->pps_lock = false;
        }
    }
    state = IDLE;
}

PPS::PPS() // : module_list(256, {nullptr, 0})
{
    for(int i = 0; i < 256; i++)
    {
        module_list[i].perf_mod = nullptr;
        module_list[i].pps_PinMonitor = nullptr;
    }
    for (int i = 0; i < 48; i++)
        input_pins[i] = nullptr;

    pps_lock = false;	// PPS initially unlocked
}

PPS::~PPS()
{
}

void PPS::set_output_source(unsigned int code, apfpin *pt_mod, int arg)
{
    assert (code < 254);

    if (module_list[code].perf_mod)
    {
        Dprintf(("PPS::set_output_source overwriting %p with %p at 0x%x\n", module_list[code].perf_mod, pt_mod, code));
    }

    module_list[code].perf_mod = pt_mod;
    module_list[code].arg = arg;
    module_list[code].pps_PinMonitor = new PPS_PinModule(
            nullptr,
            module_list[code].perf_mod,
            module_list[code].arg);
}


void PPS::set_output(RxyPPS *pt_RxyPPS, unsigned int old, PinModule *pinmodule)
{
    int reg_value = pt_RxyPPS->value.get();


    Dprintf(("reg=%s pin=%p(%s) arg=0x%x old=0x%x\n", pt_RxyPPS->name().c_str(), pinmodule, pinmodule->getPin()->name().c_str(), reg_value, old));

    if (!reg_value && old)
    {
        if (module_list[old].pps_PinMonitor)
        {
            if (module_list[old].pps_PinMonitor->rm_pinmod(pinmodule))
            {
                delete module_list[old].pps_PinMonitor;
                module_list[old].pps_PinMonitor = nullptr;
            }
        }
        return;
    }

    if (!module_list[reg_value].perf_mod)
    {
        fprintf(stderr, "PPS::set_output RxyPPS=%s 0x%x is not known\n", pt_RxyPPS->name().c_str(), reg_value);
        return;
    }


    if (!module_list[reg_value].pps_PinMonitor)
    {
        module_list[reg_value].pps_PinMonitor = new PPS_PinModule(
            pinmodule,
            module_list[reg_value].perf_mod,
            module_list[reg_value].arg);
    }
    else
    {
        module_list[reg_value].pps_PinMonitor->add_pinmod(pinmodule);
    }
}


void PPS::pps_config_pin(RxyPPS *RxyReg, PinModule *_pin)
{
    (void)RxyReg;
    (void)_pin;
    Dprintf(("PPS::config_pin RxyReg=%s pin=%p %s\n", RxyReg->name().c_str(), _pin, _pin->getPin()->name().c_str()));
}

/*
**  This function is used to configure the pin selection for the
**   xxxPPS registers.
*/
void PPS::set_ports(
    PortModule *m_porta,
    PortModule *m_portb,
    PortModule *m_portc,
    PortModule *m_portd,
    PortModule *m_porte,
    PortModule *m_portf)
{
    int i;
    for (i = 0; i < 8; i++)
    {
        if (m_porta && m_porta->getPin(i))
        {
            Dprintf(("PPS::set_ports m_porta i=%d index=%x %s\n", i, i, m_porta->getPin(i)->name().c_str()));
            input_pins[i] = &(*m_porta)[i];
        }
        if (m_portb && m_portb->getPin(i))
        {
            Dprintf(("PPS::set_ports m_portb i=%d index=%x %s\n", i, i + 8, m_portb->getPin(i)->name().c_str()));
            input_pins[i + 8] = &(*m_portb)[i];
        }
        if (m_portc && m_portc->getPin(i))
        {
            Dprintf(("PPS::set_ports m_portc i=%d index=%x %s\n", i, i + 0x10, m_portc->getPin(i)->name().c_str()));
            input_pins[i + 0x10] = &(*m_portc)[i];
        }
        if (m_portd && m_portd->getPin(i))
        {
            Dprintf(("PPS::set_ports m_portd i=%d index=%x %s\n", i, i + 0x18, m_portd->getPin(i)->name().c_str()));
            input_pins[i + 0x18] = &(*m_portd)[i];
        }
        if (m_porte && m_porte->getPin(i))
        {
            Dprintf(("PPS::set_ports m_porte i=%d index=%x %s\n", i, i + 0x20, m_porte->getPin(i)->name().c_str()));
            input_pins[i + 0x20] = &(*m_porte)[i];
        }
        if (m_portf && m_portf->getPin(i))
        {
            Dprintf(("PPS::set_ports m_portf i=%d index=%x %s\n", i, i + 0x28, m_portf->getPin(i)->name().c_str()));
            input_pins[i + 0x28] = &(*m_portf)[i];
        }
    }
}
