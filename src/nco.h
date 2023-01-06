/*
   Copyright (C) 2017 Roy R Rankin

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

NUMERICALLY CONTROLLED OSCILLATOR (NCO) MODULE

*/

#ifndef SRC_NCO_H_
#define SRC_NCO_H_

#include <glib.h>

#include <string>

#include "gpsim_classes.h"
#include "ioports.h"
#include "registers.h"
#include "trigger.h"

class NCO;
class NCO_Interface;
class ncoCLKSignalSink;
class NCOSigSource;
class PIR;
class CLC_BASE;
class CWG;
class InterruptSource;
class Processor;
class NCO_DATA_RECEIVER;

class NCOxCON : public sfr_register
{
public:
    NCOxCON(NCO *pt, Processor *pCpu, const char *pName, const char *pDesc);

    void put(unsigned int new_value) override;
    unsigned int con_mask;
    void reset(RESET_TYPE r) override;

private:
    NCO *pt_nco;
};


class NCOxCLK : public sfr_register
{
public:
    NCOxCLK(NCO *pt, Processor *pCpu, const char *pName, const char *pDesc);

    void put(unsigned int new_value) override;
    unsigned int clk_mask;

private:
    NCO *pt_nco;
};


class NCOxACCL : public sfr_register
{
public:
    NCOxACCL(NCO *pt, Processor *pCpu, const char *pName, const char *pDesc);

    void put(unsigned int new_value) override;

private:
    NCO *pt_nco;
};


class NCOxACCH : public sfr_register
{
public:
    NCOxACCH(NCO *pt, Processor *pCpu, const char *pName, const char *pDesc);

    void put(unsigned int new_value) override;

private:
    NCO *pt_nco;
};


class NCOxACCU : public sfr_register
{
public:
    NCOxACCU(NCO *pt, Processor *pCpu, const char *pName, const char *pDesc);

    void put(unsigned int new_value) override;

private:
    NCO *pt_nco;
};


class NCOxINCH : public sfr_register
{
public:
    NCOxINCH(NCO *pt, Processor *pCpu, const char *pName, const char *pDesc);

    void put(unsigned int new_value) override;

private:
    NCO *pt_nco;
};


class NCOxINCL : public sfr_register
{
public:
    NCOxINCL(NCO *pt, Processor *pCpu, const char *pName, const char *pDesc);

    void put(unsigned int new_value) override;

private:
    NCO *pt_nco;
};


class NCO : public TriggerObject, public apfpin
{
public:
    explicit NCO(Processor *pCpu);
    ~NCO();

    virtual int clock_src();
    void sleep(bool on);
    void current_value();
    void set_acc_buf();
    void set_inc_buf();
    void update_ncocon(unsigned int);
    void update_ncoclk(unsigned int);
    void setIOpin(PinModule *pin, int data) override;
    void setIOpins(PinModule *pIN, PinModule *pOUT);
    void setNCOxPin(PinModule *pNCOx);
    void set_clc_data_server(DATA_SERVER *pt) { clc_data_server = pt;}
    void link_nco(bool level, char index);
    void setState(char new3State);
    void oeNCO1(bool on);
    void outputNCO1(bool level);
    void enableCLKpin(bool on);
    void releasePinSource(PinModule *pin);
    void newINCL();
    void NCOincrement();
    void simulate_clock(bool on);
    void callback() override;
    void set_hold_acc(unsigned int acc_val, int index) { acc_hold[index] = acc_val; }
    void set_accFlag(bool newValue) { accFlag = newValue; }
    bool get_accFlag() { return accFlag; }
    void set_clc(CLC_BASE *_clc, int i) { m_clc[i] = _clc; }
    void set_cwg(CWG *_cwg) { m_cwg = _cwg; }
    const char * clk_src_name();

    enum
    {
        // NCOxCON
        NxEN	= 1 << 7,
        NxOE	= 1 << 6,
        NxOUT	= 1 << 5,
        NxPOL	= 1 << 4,
        NxPFM	= 1 << 0,
        // NCOxCLK
        NxPW_mask = 0xe0,
        // the follow are pseudo values returned from clock_src()
        HFINTOSC = 0,
        FOSC = 1,
        LC1_OUT = 2,
        NCO1CLK = 3,

        NCOout_PIN = 0
    };

    NCOxCON  nco1con;
    NCOxCLK  nco1clk;
    NCOxACCH nco1acch;
    NCOxACCL nco1accl;
    NCOxACCU nco1accu;
    NCOxINCH nco1inch;
    NCOxINCL nco1incl;

    PIR		    *pir = nullptr;
    InterruptSource *m_NCOif = nullptr;
    DATA_SERVER     *clc_data_server;
    unsigned int     NxCLKS_mask = 0x03;

private:
    Processor       *cpu;
    PinModule       *pinNCOclk = nullptr;
    std::string	    CLKgui;
    PinModule       *pinNCO1 = nullptr;
    std::string	    NCO1gui;
    NCOSigSource    *NCO1src = nullptr;
    NCO_DATA_RECEIVER   *pt_nco_data_receiver;
    bool	    srcNCO1active = false;
    int		    inc_load = 0;
    unsigned int    inc;
    gint32	    acc = 0;
    unsigned int    acc_hold[3];
    guint64	    future_cycle = 0;
    guint64	    last_cycle = 0;		// Time of last acc update
    NCO_Interface   *nco_interface = nullptr;
    ncoCLKSignalSink   *CLKsink = nullptr;
    bool	    CLKstate = false;
    bool	    NCOoverflow = false;
    bool	    accFlag = false;		// acc buffer needs updating
    unsigned int    pulseWidth = 0;
    CLC_BASE	    *m_clc[4];
    CWG		    *m_cwg = nullptr;
};


// NCO with clock layout ala 10f320
class NCO2 : public NCO
{
public:
    explicit NCO2(Processor *pCpu);

    int clock_src() override;
};

#endif // SRC_NCO_H_

