/*
   Copyright (C) 2018 Roy R Rankin

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

PERIPHERAL PIN SELECT (PPS) MODULE

*/

#ifndef SRC_PPS_h_
#define SRC_PPS_h_

#include <list>
#include <string>
#include "ioports.h"
#include "registers.h"

class Processor;
class PPS;
class PPSSignalControl;

// xxxPPS: PERIPHERAL xxx INPUT SELECTION
class xxxPPS : public sfr_register
{
public:
    xxxPPS(PPS *pt, Processor *pCpu, const char *pName, const char *pDesc,
           unsigned int _mValidBits, apfpin *_perf_mod, int _arg);

    void put(unsigned int new_value) override;
    unsigned int con_mask;

private:
    PPS *pt_pps;
    apfpin *perf_mod;
    int    arg;
    PinModule *pin;
};

//RxyPPS: PIN Rxy OUTPUT SOURCE SELECTION REGISTER
class RxyPPS : public sfr_register
{
public:
    RxyPPS(PPS *pt, PinModule *_pin, Processor *pCpu, const char *pName, const char *pDesc);

    void put(unsigned int new_value) override;
    unsigned int con_mask;

private:
    PPS 	*pt_pps;
    PinModule   *pin;
};

class PPSLOCK : public sfr_register
{
public:
    enum LOCK_STATE
    {
        IDLE = 0,
        HAVE_55,
        HAVE_AA
    };
    PPSLOCK(PPS *pt, Processor *pCpu, const char *pName, const char *pDesc);

    void put(unsigned int new_value) override;
    unsigned int con_mask;

private:
    PPS 	*pt_pps;
    enum LOCK_STATE  state;
};

class PPS_PinModule : public PinModule
{
public:
    PPS_PinModule(PinModule *_pinmodule, apfpin *_perf_mod, int _arg);
    PPS_PinModule(const PPS_PinModule &) = delete;
    PPS_PinModule& operator =(const PPS_PinModule &) = delete;
    ~PPS_PinModule();

    void updatePinModule() override;
    void setControl(SignalControl *) override;
    void add_pinmod(PinModule *);
    bool rm_pinmod(PinModule *);
    void ReleasePins();

private:
    typedef struct
    {
        PinModule *mod;
        std::string GuiName;
    } st_pin_list;
    std::list<st_pin_list>pin_list;
    apfpin *perf_mod;
    int	    arg;
    PPSSignalControl *pin_drive;

};

class PPS
{
public:
    PPS();
    ~PPS();

    void set_output(RxyPPS *RxyReg, unsigned int old, PinModule *new_pin);
    void pps_config_pin(RxyPPS *RxyReg, PinModule *_pin);
    void set_output_source(unsigned int code, apfpin *pt_mod, int arg);
    void set_ports(PortModule *m_porta,
                   PortModule *m_portb,
                   PortModule *m_portc,
                   PortModule *m_portd = nullptr,
                   PortModule *m_porte = nullptr,
                   PortModule *m_portf = nullptr);

    PinModule * get_input_pin(unsigned int index) {return input_pins[index];}
    bool   pps_lock;
    struct mod_struct
    {
        apfpin	      *perf_mod;
        int	      arg;
        PPS_PinModule *pps_PinMonitor;
    } module_list[256];
private:
    PinModule *    input_pins[48];

};

#endif //SRC_PPS_h_

