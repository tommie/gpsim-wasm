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
*/

// CONFIGURABLE LOGIC CELL (CLC)

#ifndef SRC_CLC_H_
#define SRC_CLC_H_

#include <glib.h>

#include <algorithm>
#include <string>

#include "ioports.h"
#include "processor.h"
#include "registers.h"
#include "trigger.h"

class CLC;
class CLC_BASE;
class CLC_4SEL;
class OSC_SIM;
class InterruptSource;
class NCO;
class INxSignalSink;
class CLCSigSource;
class COG;
class TMRL;
class TMR2;
class ZCDCON;
class ATx;

class CLCxCON : public sfr_register
{
public:
    CLCxCON(CLC_BASE *_clc, Processor *pCpu, const char *pName, const char *pDesc)
        : sfr_register(pCpu, pName, pDesc), m_clc(_clc), write_mask(0xdf), 
	   read_only(0x20)
    {
    }
    void put(unsigned int) override;
    void set_write_mask(unsigned int _write_mask) { write_mask = _write_mask;}

private:
    CLC_BASE *m_clc;
    unsigned int write_mask;
    unsigned int read_only;
};


class CLCxPOL : public sfr_register
{
public:
    CLCxPOL(CLC_BASE *_clc, Processor *pCpu, const char *pName, const char *pDesc)
        : sfr_register(pCpu, pName, pDesc), m_clc(_clc), write_mask(0x8f)
    {
    }

    void put(unsigned int) override;

private:
    CLC_BASE *m_clc;
    unsigned int write_mask;
};


// Select 8 inputs for D1S and D2S
class CLCxSEL0 : public sfr_register
{
public:
    CLCxSEL0(CLC *_clc, Processor *pCpu, const char *pName, const char *pDesc);

    void put(unsigned int) override;

private:
    CLC *m_clc;
    unsigned int write_mask;
};


// Select 8 inputs for D3S and D4S
class CLCxSEL1 : public sfr_register
{
public:
    CLCxSEL1(CLC *_clc, Processor *pCpu, const char *pName, const char *pDesc);
    void put(unsigned int) override;

private:
    CLC *m_clc;
    unsigned int write_mask;
};

// Entire register can select inputs for DxS
class CLCxSELx : public sfr_register
{
public:
    CLCxSELx(CLC_4SEL *_clc, Processor *pCpu, const char *pName, const char *pDesc, unsigned int reg_number);
    void put(unsigned int) override;
    void set_write_mask(unsigned int _write_mask) { write_mask = _write_mask;}

private:
    CLC_4SEL *m_clc;
    unsigned int write_mask;
    unsigned int reg_number;
};


class CLCxGLS0 : public sfr_register
{
public:
    CLCxGLS0(CLC_BASE *_clc, Processor *pCpu, const char *pName, const char *pDesc)
        : sfr_register(pCpu, pName, pDesc), m_clc(_clc)
    {
    }

    void put(unsigned int) override;

private:
    CLC_BASE *m_clc;
};


class CLCxGLS1 : public sfr_register
{
public:
    CLCxGLS1(CLC_BASE *_clc, Processor *pCpu, const char *pName, const char *pDesc)
        : sfr_register(pCpu, pName, pDesc), m_clc(_clc)
    {
    }

    void put(unsigned int) override;

private:
    CLC_BASE *m_clc;
};


class CLCxGLS2 : public sfr_register
{
public:
    CLCxGLS2(CLC_BASE *_clc, Processor *pCpu, const char *pName, const char *pDesc)
        : sfr_register(pCpu, pName, pDesc), m_clc(_clc)
    {
    }

    void put(unsigned int) override;

private:
    CLC_BASE *m_clc;
};


class CLCxGLS3 : public sfr_register
{
public:
    CLCxGLS3(CLC_BASE *_clc, Processor *pCpu, const char *pName, const char *pDesc)
        : sfr_register(pCpu, pName, pDesc), m_clc(_clc)
    {
    }

    void put(unsigned int) override;

private:
    CLC_BASE *m_clc;
};


class CLCDATA : public sfr_register, public apfpin
{
public:
    explicit CLCDATA(Processor *pCpu, const char *pName = nullptr, const char *pDesc = nullptr)
        : sfr_register(pCpu, pName, pDesc)
    {
        std::fill_n(m_clc, 4, nullptr);
    }

    void put(unsigned int /* val */) override {}
    void set_bit(bool bit_val, unsigned int pos);
    void setIOpin(PinModule *pin, int data) override;
    void set_clc(CLC_BASE *clc1, CLC_BASE *clc2 = nullptr, CLC_BASE *clc3 = nullptr, CLC_BASE *clc4 = nullptr)
    {
        m_clc[0] = clc1;
        m_clc[1] = clc2;
        m_clc[2] = clc3;
        m_clc[3] = clc4;
    }

private:
    CLC_BASE *m_clc[4];
};

class CLC_DATA_RECEIVER;
class ZCD_DATA_RECEIVER;

class CLC_BASE : public apfpin
{
public:
    enum
    {
        // CLCxCON
        LCxEN   = (1 << 7),
        LCxOE   = (1 << 6),
        LCxOUT  = (1 << 5),
        LCxINTP = (1 << 4),
        LCxINTN = (1 << 3),
        LCxMODE = 0x7,

        // CLCxPOL
        LCxPOL  = (1 << 7),
    };

    // Give names to input options
    // set_dxs_data is used to relate this names to the 
    // actual input select in CLCxSELn
    enum data_in
    {
        UNUSED = 0,
        LC1,
        LC2,
        LC3,
        LC4,
        CLCxIN0,	// 5
        CLCxIN1,
        CLCxIN2,
        CLCxIN3,
        PWM1,
        PWM2,		//10
        PWM3,
        PWM4,
        NCOx,
        FOSCLK,
        LFINTOSC,	//15
        HFINTOSC,
        FRC_IN,
        T0_OVER,
        T1_OVER,
        T2_MATCH,	//20
        T4_MATCH,
        T6_MATCH,
        C1OUT,
        C2OUT,
        ZCD_OUT,	//25
        COG1A,
        COG1B,
        CCP1,
        CCP2,
        MSSP_SCK,	//30
        MSSP_SDO,
        TX_CLK,
        UART_DT,
        IOCIF,
        ADCRC,		//35
        AT1_CMP1,
        AT1_CMP2,
        AT1_CMP3,
        SMT1_MATCH,
        SMT2_MATCH,     //40
        T3_OVER,
        T5_OVER,
        AT1_MISSPULSE,
        AT1_PERCLK,
        AT1_PHSCLK,     //45
        TX,
        RX,
        SCK,
        SDO

    };

    enum
    {
        CLCout_PIN = 0,
        CLCin0_PIN,
        CLCin1_PIN,
        CLCin2_PIN,
        CLCin3_PIN
    };

    CLC_BASE(Processor *_cpu, unsigned int _index, CLCDATA *_clcdata);
    ~CLC_BASE();

    void set_dxs_data(int input, int size, data_in *data);
    bool CLCenabled() { return clcxcon.value.get() & LCxEN; }
    virtual bool out_enabled() {return clcxcon.value.get() & LCxOE; }
    void setCLCxPin(PinModule *alt_pin);
    void enableINxpin(int, bool);
    void setIOpin(PinModule *pin, int data) override;
    virtual void D1S(int select) { DxS_data[0] = dxs_data[0][select];}
    virtual void D2S(int select) { DxS_data[1] = dxs_data[1][select];}
    virtual void D3S(int select) { DxS_data[2] = dxs_data[2][select];}
    virtual void D4S(int select) { DxS_data[3] = dxs_data[3][select];}
    void t0_overflow();
    void t135_overflow(int timer_number);
    void t1_overflow();
    void t246_match(char);
    void osc_out(bool level, int kind);
    void out_pwm(bool level, int id);
    void NCO_out(bool level);
    void CxOUT_sync(bool output, int cm);
    void ZCDx_out(bool level);
    void ATx_out(bool level, int v2);
    void set_cog(COG *_cog) { p_cog = _cog;}
    void set_clcPins(PinModule *_CLCx, PinModule *IN0, PinModule *IN1,
                     PinModule *IN2 = nullptr, PinModule *IN3 = nullptr)
    {
        pinCLCxIN[0] = IN0;
        pinCLCxIN[1] = IN1;
        pinCLCxIN[2] = IN2;
        pinCLCxIN[3] = IN3;
        pinCLCx = _CLCx;
    }
    void set_zcd(ZCDCON *_zcd) { m_zcd = _zcd;}
    void set_atx(ATx *_atx) {m_at1 = _atx;}
    void set_clc(  CLC_BASE *clc1, CLC_BASE *clc2 = nullptr, 
	           CLC_BASE *clc3 = nullptr, CLC_BASE *clc4 = nullptr
	        )
    {
        m_clc[0] = clc1;
        m_clc[1] = clc2;
        m_clc[2] = clc3;
        m_clc[3] = clc4;
    }
    DATA_SERVER  *get_CLC_data_server() { return clc_data_server;}
    void setState(char new3State, int index);
    void releasePinSource(PinModule *pin);
    void oeCLCx(bool on);
    void update_clccon(unsigned int diff);
    void config_inputs(bool on);
    void compute_gates();
    void cell_function();
    bool cell_1_in_flipflop();
    bool cell_2_in_flipflop();
    bool cell_sr_latch();
    bool JKflipflop();
    bool transparent_D_latch();
    void clc_lcxupdate(bool bit_val, unsigned int pos);
    virtual void setInterruptSource(InterruptSource * _int)
    {
        m_Interrupt = _int;
    }
    void outputCLC(bool out);
    void set_tmr246(TMR2 *pt, int index) {p_tmr246[index] = pt;}
    void set_tmr135(TMRL *t1, TMRL *t2=nullptr, TMRL *t5=nullptr)
	{ p_tmr135[0]=t1; p_tmr135[1]=t2; p_tmr135[2] = t5;}


    unsigned int  index;
    CLCxCON  	clcxcon;
    CLCxPOL  	clcxpol;
    CLCxGLS0 	clcxgls0;
    CLCxGLS1 	clcxgls1;
    CLCxGLS2 	clcxgls2;
    CLCxGLS3 	clcxgls3;
    CLCDATA  	*clcdata;
    OSC_SIM  	*frc = nullptr;
    OSC_SIM  	*lfintosc = nullptr;
    OSC_SIM  	*hfintosc = nullptr;
    NCO      	*p_nco = nullptr;
    data_in	 DxS_data[4];
    CLC_BASE    *m_clc[4];
    ZCDCON	*m_zcd = nullptr;
    ATx		*m_at1 = nullptr;
    PinModule    *pinCLCx = nullptr;

private:
    TMRL	 *p_tmr135[3];
    bool	 attached_tmr135[3];
    TMR2	 *p_tmr246[3];
    COG		 *p_cog = nullptr;
    CLCSigSource *CLCxsrc = nullptr;
    std::string   CLCxgui;
    bool          srcCLCxactive = false;
    INxSignalSink *INxsink[4];
    int		  INxactive[4];
    bool	  INxstate[4];
    PinModule     *pinCLCxIN[4];
    std::string   INxgui[4];
    bool	  pwmx_level[4];
    bool	  CMxOUT_level[4];
    bool	  frc_level = false;
    bool	  NCO_level = false;
    bool	  ZCD_level = false;
    bool	  lcxdT[4];		// incoming data
    bool	  lcxg[4];		// Data gate output
    InterruptSource *m_Interrupt = nullptr;
    bool	  Doutput = false;
    bool	  Dclock = false;
    bool	  FRCactive = false;
    bool	  LFINTOSCactive = false;
    bool	  HFINTOSCactive = false;
    data_in      *dxs_data[4];
    int		  dxs_data_length[4];
    DATA_SERVER   *clc_data_server = nullptr;
    CLC_DATA_RECEIVER  *clc_data_receiver[4];
    CLC_DATA_RECEIVER  *t246_data_receiver[3];
    CLC_DATA_RECEIVER  *t135_data_receiver = nullptr;
    CLC_DATA_RECEIVER  *zcd_data_receiver = nullptr;
    CLC_DATA_RECEIVER  *atx_data_receiver = nullptr;
};

// CLC with 2 SEL registers
class CLC : public CLC_BASE
{
public:
    CLC(Processor *_cpu, unsigned int _index, CLCDATA *_clcdata);
    ~CLC() {}


    CLCxSEL0 clcxsel0;
    CLCxSEL1 clcxsel1;
};

// PIC16F170x CLC has 4 SEL registers rather than 2 and no OE bit
// output uses PPS
class CLC_4SEL : public CLC_BASE
{
public:
    CLC_4SEL(Processor *_cpu, unsigned int _index, CLCDATA *_clcdata);

    bool out_enabled() override {return pinCLCx != nullptr;}
    CLCxSELx clcxsel0;
    CLCxSELx clcxsel1;
    CLCxSELx clcxsel2;
    CLCxSELx clcxsel3;
};


// RC internal clocks  used with ADC, CLC
class OSC_SIM : public TriggerObject
{
public:
    OSC_SIM(double _freq, int _data_in);

    void start_osc_sim(bool on);

    void set_clc(CLC_BASE *clc1, CLC_BASE *clc2 = nullptr,
                 CLC_BASE *clc3 = nullptr, CLC_BASE *clc4 = nullptr)
    {
        m_clc[0] = clc1;
        m_clc[1] = clc2;
        m_clc[2] = clc3;
        m_clc[3] = clc4;
    }
    void callback() override;

private:
    double 	frequency;
    int		data_in;
    int		active = 0;
    CLC_BASE 	*m_clc[4];
    bool 	level = false;
    int  	next_cycle = 0;
    guint64 	future_cycle = 0;
    gint64  	adjust_cycles;
};


#endif // SRC_CLC_H_
