/*
   Copyright (C) 2022 Roy R. Rankin

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


#ifndef SRC_WDT_H_
#define SRC_WDT_H_
#include "gpsim_classes.h"
#include "registers.h"
#include "pic-processor.h"

class sfr_register;
class WDT;

class WDTCON : public sfr_register
{
public:
    unsigned int valid_bits;

    enum
    {
        WDTPS3 = 1 << 4,
        WDTPS2 = 1 << 3,
        WDTPS1 = 1 << 2,
        WDTPS0 = 1 << 1,
        SWDTEN = 1 << 0
    };

    WDTCON(Processor *pCpu, const char *pName, const char *pDesc, unsigned int bits)
        : sfr_register(pCpu, pName, pDesc), valid_bits(bits)
    {
    }
    void put(unsigned int new_value) override;
    void reset(RESET_TYPE r) override;
};


//WATCHDOG TIMER CONTROL REGISTER 0
class WDTCON0: public sfr_register
{
public:
    WDTCON0(WDT *_win_wdt, Processor *pCpu, const char *pName, const char *pDesc, unsigned int bits)
        : sfr_register(pCpu, pName, pDesc), valid_bits(bits), win_wdt(_win_wdt)
    {
    }
    enum
    {
	WDTPS_mask = 0x3e,
	SEN	   = 1,
    };
    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;
    void reset(RESET_TYPE r) override;
    void set_por(unsigned int por) { rst_value = por;}

    bool	 wdps_readonly = true;	
private:
    unsigned int valid_bits;
    unsigned int rst_value = 0;
    WDT	*win_wdt;
};

//WATCHDOG TIMER CONTROL REGISTER 1
class WDTCON1: public sfr_register
{
public:
    enum
    {
	WDTCS_mask    = 0x70,
	WDTCS_shift   = 4,
	WINDOW_mask   = 0x07,
    };

    WDTCON1(WDT *_win_wdt, Processor *pCpu, const char *pName, const char *pDesc, unsigned int bits)
        : sfr_register(pCpu, pName, pDesc), valid_bits(bits), win_wdt(_win_wdt)
    {
    }
    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;
    void reset(RESET_TYPE r) override;

   bool         wdtcs_readonly = true;
   bool         window_readonly = true;
private:
    unsigned int valid_bits;
    WDT	*win_wdt;
};

//WDT PRESCALE SELECT LOW BYTE REGISTER (READ ONLY)
class WDTPSL: public sfr_register
{
public:
    WDTPSL(WDT *_win_wdt, Processor *pCpu, const char *pName, const char *pDesc)
        : sfr_register(pCpu, pName, pDesc), win_wdt(_win_wdt)
    {
    }
    void put(unsigned int new_value) {}
    unsigned int get();
private:
    WDT	*win_wdt;
};

//WDT PRESCALE SELECT HIGH BYTE REGISTER (READ ONLY)
class WDTPSH: public sfr_register
{
public:

    WDTPSH(WDT *_win_wdt, Processor *pCpu, const char *pName, const char *pDesc)
        : sfr_register(pCpu, pName, pDesc), win_wdt(_win_wdt)
    {
    }
    void put(unsigned int new_value) {}
    unsigned int get();

private:
    WDT	*win_wdt;
};

//WDT Timer Register (READ ONLY)
class WDTTMR: public sfr_register
{
public:
    enum
    {
	WDTTMR_mask = 0x78,
	WDTTMR_shift = 3,
	STATE	     = 1<<2,
	PSCNTU_mask  = 0x3,
    };
    WDTTMR(WDT *_win_wdt, Processor *pCpu, const char *pName, const char *pDesc)
        : sfr_register(pCpu, pName, pDesc), win_wdt(_win_wdt)
    {
    }
    void put(unsigned int new_value) {}
    unsigned int get();
private:
    WDT	*win_wdt;
};
#endif //SRC_WDT_H_
