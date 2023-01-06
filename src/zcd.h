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

Zero Crossing Detector (ZCD) MODULE

*/

#ifndef SRC_ZCD_h_
#define SRC_ZCD_h_

#include "registers.h"
#include "ioports.h"

class CLC_BASE;
class InterruptSource;
class PinModule;
class PinMonitor;
class Processor;
class ZCDSignalControl;
class ZCDPinMonitor;
class ZCDSignalSource;
class DATA_SERVER;

// ZCDCON: Register
class ZCDCON : public sfr_register, public apfpin
{
public:
    enum
    {
        ZCDxINTN = 1 << 0,
        ZCDxINTP = 1 << 1,
        ZCDxPOL  = 1 << 4,
        ZCDxOUT  = 1 << 5,
        ZCDxEN   = 1 << 7
    };

    ZCDCON(Processor *pCpu, const char *pName, const char *pDesc);
    ~ZCDCON();

    void put(unsigned int new_value) override;
    void new_state(bool state);
    unsigned int con_mask;
    void setIOpin(PinModule *_pin, int arg = 0) override;
    void releasePin();
    void close_module();
    virtual void setInterruptSource(InterruptSource * _int) { m_Interrupt = _int; }
    DATA_SERVER   *get_zcd_data_server() { return zcd_data_server; }

private:
    PinModule        *m_PinMod[2];
    InterruptSource  *m_Interrupt;
    ZCDSignalControl *m_control;
    ZCDPinMonitor    *m_monitor;
    ZCDSignalSource  *m_source;
    ZCDSignalSource  *m_out_source;
    PinMonitor 	     *m_SaveMonitor;
    DATA_SERVER      *zcd_data_server = nullptr;

    double	     save_Vth;
};

#endif //SRC_ZCD_h_

