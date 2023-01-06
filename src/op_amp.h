/*
   Copyright (C) 2019 Roy R Rankin

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


Operational Amplifier (OPA) Modules

*/

#ifndef SRC_OPA_h_
#define SRC_OPA_h_

#include "registers.h"
class PinModule;
class Processor;

class OPA : public sfr_register
{
public:
    OPA(Processor *pCpu, const char *pName, const char *pDesc);
    ~OPA();

    void set_pins(PinModule *inPos, PinModule *inNeg, PinModule *Out);
    void put(unsigned int _value) override;

private:
    enum
    {
        OPAxCH = 3,
        OPAxUG = 1 << 4,
        OPAxSP = 1 << 6,
        OPAxEN = 1 << 7
    };

    PinModule *	OPAinPos;
    PinModule *	OPAinNeg;
    PinModule *	OPAout;
};

#endif //SRC_OPA_h_

