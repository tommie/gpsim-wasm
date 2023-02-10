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

// Operational Amplifier (OPA) Modules



//#define DEBUG
#if defined(DEBUG)
#include <config.h>
#define Dprintf(arg) {printf("%s:%d ",__FILE__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

#include "op_amp.h"

#include <string>
#include "ioports.h"
#include "processor.h"
#include "stimuli.h"
#include "trace.h"

class OPA_stimulus : public stimulus
{
public:
    explicit OPA_stimulus(OPA * arg, const char *cPname, double _Vth, double _Zth)
        : stimulus(cPname, _Vth, _Zth), _opa(arg)
    {
    }

    ~OPA_stimulus()
    {
    }

    void   set_nodeVoltage(double v) override
    {
        if (nodeVoltage != v)
        {
            nodeVoltage = v;
            Dprintf(("set_nodeVoltage %s _opa %p %s v=%.2f\n", name().c_str(), _opa, _opa->name().c_str(), v));
            _opa->get();  // recalculate comparator values
        }
    }

private:
    OPA *_opa;
};

OPA::OPA( Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc),
      OPAinPos(nullptr), OPAinNeg(nullptr), OPAout(nullptr)
{
    mValidBits = 0xd3;
}

OPA::~OPA()
{
}

void OPA::set_pins(PinModule *inPos, PinModule *inNeg, PinModule *Out)
{
    OPAinPos = inPos;
    OPAinNeg = inNeg;
    OPAout = Out;
}

void OPA::put(unsigned int new_value)
{
    unsigned int diff = (new_value ^ value.get()) & mValidBits;

    Dprintf(("OPA::put %s new_value=0x%x diff=0x%x\n", name().c_str(), new_value, diff));
    if (!diff)
        return;

    trace.raw(write_trace.get() | value.get());
    value.put(new_value & mValidBits);
    if (diff & OPAxEN)	// change of enable bit
    {
        if (new_value & OPAxEN)		// turning on OPA
        {
            std::string opa_name = name();
            opa_name.replace(4,3, "out");
            OPAout->AnalogReq(this, true, opa_name.c_str());
            OPAout->getPin()->setDriving(true);
            OPAout->getPin()->set_Vth(2.5);
            OPAout->getPin()->updateNode();
        }
    }
}
