/*
   Copyright (C) 1998 T. Scott Dattalo
   Copyright (C) 2009,2010,2013 Roy R. Rankin

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

#ifndef SRC_14_BIT_TMRS_H_
#define SRC_14_BIT_TMRS_H_

#include <glib.h>

#include "registers.h"
#include "ioports.h"
#include "ssp.h"
#include "stimuli.h"
#include "trigger.h"

class TMRL;
class TMR2;
class CCPRL;
class CCPCON;
class PWM1CON;
class ADCON0;
class PIR_SET;
class InterruptSource;
class TMR1_Interface;
class CWG;
class CLC_BASE;
class ZCDCON;
class COG;
class TMR246_WITH_HLT;
class TMRx_HLT;
class TMRx_CLKCON;
class ATx;

class _14bit_processor;

class PIR;
class Processor;
class SSP_MODULE;
class INT_SignalSink;   // I/O pin interface
class CCPSignalSource;  // I/O pin interface
class CCPSignalSink;    // I/O pin interface
class Tristate;		      // I/O pin high impedance
class Tx_RST_RECEIVER;
class Tx_CLK_RECEIVER;


//---------------------------------------------------------
// Todo
//
// The timer base classes need to be abstracted one more
// layer. The 18fxxx parts have a new timer, TMR3, that's
// almost but not quite, identical to the 16fxx's TMR1.


//---------------------------------------------------------
// CCPCON - Capture and Compare registers
//---------------------------------------------------------
class CCPRH : public sfr_register
{
public:
    CCPRH(Processor *pCpu, const char *pName, const char *pDesc = nullptr);

    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;
    unsigned int get() override;

    CCPRL *ccprl = nullptr;
    bool  pwm_mode = false;
    unsigned int pwm_value = 0;
};

class CCPRH_HLT : public CCPRH
{
public:
    CCPRH_HLT(Processor *pCpu, const char *pName, const char *pDesc = nullptr) :
	CCPRH(pCpu, pName, pDesc)
    {}
    void put(unsigned int new_value) override;
};

class CCPRL : public sfr_register
{
public:
    CCPRL(Processor *pCpu, const char *pName, const char *pDesc = nullptr);

    void put(unsigned int new_value) override;
    void capture_tmr();
    void start_compare_mode(CCPCON *ref = nullptr);
    void stop_compare_mode();
    bool test_compare_mode();
    void start_pwm_mode();
    void stop_pwm_mode();
    void assign_tmr(TMRL *ptmr);

    CCPRH  *ccprh = nullptr;
    CCPCON *ccpcon = nullptr;
    TMRL   *tmrl = nullptr;
};


//
// Enhanced Capture/Compare/PWM Auto-Shutdown control register
//
class ECCPAS : public sfr_register
{
public:
    ECCPAS(Processor *pCpu, const char *pName, const char *pDesc = nullptr);
    ~ECCPAS();

    /* Bit definitions for the register */
    enum
    {
        PSSBD0  = 1 << 0,	// Pins P1B and P1D Shutdown control bits
        PSSBD1  = 1 << 1,
        PSSAC0  = 1 << 2,	// Pins P1A and P1C Shutdown control bits
        PSSAC1  = 1 << 3,
        ECCPAS0 = 1 << 4,	// ECCP Auto-shutdown Source Select bits
        ECCPAS1 = 1 << 5,
        ECCPAS2 = 1 << 6,
        ECCPASE = 1 << 7	// ECCP Auto-Shutdown Event Status bit
    };

    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;
    void setBitMask(unsigned int bm) { mValidBits = bm; }
    void setIOpin(PinModule *p0, PinModule *p1, PinModule *p2);
    void c1_output(int value);
    void c2_output(int value);
    void set_trig_state(int index, bool state);
    bool shutdown_trigger(int);
    void link_registers(PWM1CON *_pwm1con, CCPCON *_ccp1con);

private:
    PWM1CON        *pwm1con = nullptr;
    CCPCON         *ccp1con = nullptr;
    PinModule      *m_PinModule = nullptr;
    INT_SignalSink *m_sink = nullptr;

    bool trig_state[3];
};


//
// Enhanced PWM control register
//
class PWM1CON : public sfr_register
{
public:
    PWM1CON(Processor *pCpu, const char *pName, const char *pDesc = nullptr);
    ~PWM1CON();

    /* Bit definitions for the register */
    enum
    {
        PDC0    = 1 << 0,	// PWM delay count bits
        PDC1    = 1 << 1,
        PDC2    = 1 << 2,
        PDC3    = 1 << 3,
        PDC4    = 1 << 4,
        PDC5    = 1 << 5,
        PDC6    = 1 << 6,
        PRSEN   = 1 << 7	// PWM Restart Enable bit
    };

    void put(unsigned int new_value) override;
    void setBitMask(unsigned int bm) { mValidBits = bm; }
};


//
// Enhanced PWM Pulse Steering control register
//
class PSTRCON : public sfr_register
{
public:
    PSTRCON(Processor *pCpu, const char *pName, const char *pDesc = nullptr);
    ~PSTRCON();

    /* Bit definitions for the register */
    enum
    {
        STRA    = 1 << 0,	// Steering enable bit A
        STRB    = 1 << 1,	// Steering enable bit B
        STRC    = 1 << 2,	// Steering enable bit C
        STRD    = 1 << 3,	// Steering enable bit D
        STRSYNC = 1 << 4,	// Steering Sync bit
    };

    void put(unsigned int new_value) override;
};


//---------------------------------------------------------
// CCPCON - Capture and Compare Control register
//---------------------------------------------------------

class CCPCON : public sfr_register, public TriggerObject, public apfpin
{
public:
    CCPCON(Processor *pCpu, const char *pName, const char *pDesc = nullptr);
    ~CCPCON();

    /* Bit definitions for the register */
    enum
    {
        CCPM0 = 1 << 0,
        CCPM1 = 1 << 1,
        CCPM2 = 1 << 2,
        CCPM3 = 1 << 3,
        CCPY  = 1 << 4,
        CCPX  = 1 << 5,
        P1M0  = 1 << 6,	// CCP1 EPWM Output config bits 16f88x
        P1M1  = 1 << 7
    };

    /* Define the Modes (based on the CCPM bits) */
    enum
    {
        ALL_OFF0 	= 0,
        ALL_OFF1 	= 1,
        ALL_OFF2 	= 2,
        ALL_OFF3 	= 3,
	COM_TOG_CLR 	= 1,
	COM_TOG		= 2,
	CAP_RISE_OR_FALL = 3,
        CAP_FALLING_EDGE = 4,
        CAP_RISING_EDGE  = 5,
        CAP_RISING_EDGE4 = 6,
        CAP_RISING_EDGE16 = 7,
        COM_SET_OUT 	= 8,
        COM_CLEAR_OUT 	= 9,
        COM_INTERRUPT 	= 0xa,
        COM_TRIGGER 	= 0xb,
	PWM_MASK 	= 0x0c,
        PWM0 		= 0xc,
        PWM1 		= 0xd,
        PWM2 		= 0xe,
        PWM3 		= 0xf,
	MODE_MASK 	= 0x0f,
    };

    enum
    {
        CCP_PIN = 0,
        PxB_PIN,
        PxC_PIN,
        PxD_PIN,
	CCP_IN_PIN
    };

    void setBitMask(unsigned int bv) { mValidBits = bv; }
    virtual void new_edge(unsigned int level);
    virtual void compare_match();
    virtual void pwm_match(int level);
    virtual void simple_pwm_output(int level);
    virtual void ccprl2ccprh() 
    {
	ccprl->ccprh->put_value(ccprl->value.get());
    }
    virtual unsigned int input_pin() { return CCP_PIN;}
    void drive_bridge(int level, int new_value);
    void shutdown_bridge(int eccpas);
    void put(unsigned int new_value) override;
    char getState();
    bool test_compare_mode();
    void callback() override;
    void releasePins(int);
    void releaseSink();
    void stop_pwm();
    virtual void compare_start(unsigned int mode, unsigned int old_value);
    void capture_start(unsigned int mode, unsigned int old_value);
    void config_output(unsigned int i, bool newOut, bool newIn);

    void setCrosslinks(CCPRL *, PIR *, unsigned int _mask, TMR2 *, ECCPAS *_eccpas = nullptr);
    void setADCON(ADCON0 *);

    
    void setIOpin(PinModule *pin, int data) override;
    void setIOpin(PinModule *p1, PinModule *p2 = nullptr, PinModule *p3 = nullptr, PinModule *p4 = nullptr);
    void set_tmr2(TMR2 *_tmr2) { tmr2 = _tmr2; }
    virtual bool is_pwm() { return (value.get() & PWM0) == PWM0; }
    virtual unsigned int pwm_duty_cycle()
    {
        return ((value.get() >> 4) & 3) | 4 * ccprl->value.get();
    }
    void set_cwg(CWG *_cwg) { m_cwg = _cwg;}
    void set_cog(COG *_cog) { m_cog = _cog;}
    void set_clc(CLC_BASE *_clc, int i) { m_clc[i] = _clc;}
    virtual void setInterruptSource(InterruptSource * _int) 
    { 
	m_Interrupt = _int; 
    }
    virtual void ccp_out(bool state, bool interrupt){};
    virtual void in_pin_active(bool on_off);
    DATA_SERVER     *get_ccp_server();


    PSTRCON *pstrcon = nullptr;
    PWM1CON *pwm1con = nullptr;
    ECCPAS  *eccpas = nullptr;
    char    index;
    COG     *m_cog;
    CWG     *m_cwg;
    CLC_BASE     *m_clc[4];

    DATA_SERVER   *ccp_output_server = nullptr;

protected:
    PinModule 	  *m_PinModule[5];
    CCPSignalSource *m_source[5];
    bool	  source_active[5];
    CCPSignalSink *m_sink = nullptr;
    Tristate	  *m_tristate = nullptr;
    bool  	  m_bInputEnabled = false;    // Input mode for capture/compare
    bool  	  m_bOutputEnabled = false;   // Output mode for PWM
    char  	  m_cOutputState;
    int   	  edges = 0;
    int		  edge_cnt = 0;
    guint64 	  future_cycle;
    bool 	  delay_source0 = false, delay_source1 = false;
    bool	  pulse_clear = false;
    bool 	  bridge_shutdown = false;

    CCPRL         *ccprl = nullptr;
    PIR           *pir = nullptr;
    TMR2          *tmr2 = nullptr;
    ADCON0        *adcon0 = nullptr;
    unsigned int   pir_mask = 0;
    InterruptSource *m_Interrupt = nullptr;
};

//---------------------------------------------------------
// CCPCON_FMT - Capture and Compare Control register
//      FMT bit used to decode ccprx[lh] for the duty cycle(pulse width)
//      and OUT bit is bit 5
//      aka 16f161x
//---------------------------------------------------------

class CCPCON_FMT : public CCPCON
{
public:
    CCPCON_FMT(Processor *pCpu, const char *pName, const char *pDesc = nullptr):
	CCPCON(pCpu, pName, pDesc) { mValidBits = 0x9f;}
    ~CCPCON_FMT(){}
    enum {
	EN  		= 1<<7,
	CCP_OUT 	= 1<<5,
	FMT 		= 1<<4,
	COM_TOG_CLR 	= 1,
	COM_TOG		= 2,
	CAP_RISE_OR_FALL = 3,
	COM_PULSE 	= 0xa,
	COM_PULSE_CLR 	= 0xb,
    };
    
    void put(unsigned int) override;
    unsigned int pwm_duty_cycle() override;
    void ccprl2ccprh() override {}	//ccprh not buffer
    void simple_pwm_output(int level) override;
    void compare_start(unsigned int mode, unsigned int old_value) override;
    void capture_output();
    
    void new_edge(unsigned int  level ) override;
    void ccp_pwm();
    bool is_pwm() override
    { 
	return (value.get() & (PWM0 | EN)) == (PWM0 | EN);
    }
    unsigned int input_pin() override { return CCP_IN_PIN;}
    void compare_match() override;
    void ccp_out(bool state, bool interrupt) override;
    void new_capture_src(unsigned int new_value);

    ComparatorModule2 	*comparator = nullptr;
private:
    unsigned int 	capture_input = 0;
};

class PWMxCON : public CCPCON
{
public:
    PWMxCON(Processor *pCpu, const char *pName, const char *pDesc = nullptr, char _index = 1);

    ~PWMxCON();

    enum
    {
        PWMxEN = 1 << 7,
        PWMxOE = 1 << 6,
        PWMxOUT = 1 << 5,
        PWMxPOL = 1 << 4
    };
    void put(unsigned int new_value) override;
    virtual bool outPinEnabled() { return value.get() & PWMxOE;}
    void put_value(unsigned int new_value) override;
    void pwm_match(int level) override;
    bool is_pwm() override { return value.get() & PWMxEN; }
    void new_edge(unsigned int /* level */ ) override {}
    unsigned int pwm_duty_cycle() override
    {
        return (pwmdch->value.get() << 2) + (pwmdcl->value.get() >> 6);
    }
    void set_pwmdc(sfr_register *_pwmdcl, sfr_register *_pwmdch)
    {
        pwmdcl = _pwmdcl;
        pwmdch = _pwmdch;
    }

    DATA_SERVER     *get_pwmx_server();

    void set_cwg(CWG *_cwg) { m_cwg = _cwg; }
    void set_clc(CLC_BASE *_clc, int i) { m_clc[i] = _clc; }

private:
    sfr_register  *pwmdcl = nullptr;
    sfr_register  *pwmdch = nullptr;
    CWG		  *m_cwg = nullptr;
    CLC_BASE	  *m_clc[4];
    char	  index;
    DATA_SERVER   *pwmx_output_server = nullptr;
};

// Output pin enabled if selected in a RxyPPS register
class PWMxCON_PPS : public PWMxCON
{
public:
    PWMxCON_PPS(Processor *pCpu, const char *pName,
                const char *pDesc=0, char _index=1) :
        PWMxCON(pCpu, pName, pDesc, _index)
    {
    }
    bool outPinEnabled() override { return m_PinModule[0] != nullptr;}
};


class TRISCCP : public sfr_register
{
public:
    TRISCCP(Processor *pCpu, const char *pName, const char *pDesc = nullptr);

    enum
    {
        TT1CK = 1 << 0,
        TCCP  = 1 << 2
    };
    void put(unsigned int value) override;

private:
    bool first;
};


class DATACCP : public sfr_register
{
public:
    DATACCP(Processor *pCpu, const char *pName, const char *pDesc = nullptr);

    enum
    {
        TT1CK = 1 << 0,
        DCCP  = 1 << 2
    };
    void put(unsigned int value) override;

private:
    bool first;
};


class TMR1_Freq_Attribute;


//---------------------------------------------------------
// T1CON - Timer 1 control register

class T1CON : public sfr_register
{
public:
    T1CON(Processor *pCpu, const char *pName, const char *pDesc = nullptr);
    T1CON(const T1CON &) = delete;
    T1CON & operator = (const T1CON &) = delete;
    ~T1CON();

    enum
    {
        TMR1ON  = 1 << 0,
        TMR1CS  = 1 << 1,
        T1SYNC  = 1 << 2,
        T1OSCEN = 1 << 3,
        T1CKPS0 = 1 << 4,
        T1CKPS1 = 1 << 5,
        T1RD16  = 1 << 6,
        TMR1GE  = 1 << 6, // TMR1 Gate Enable used if TMR1L::setGatepin() has been called
        T1GINV  = 1 << 7
    };

    TMRL       *tmrl = nullptr;
    TMR1_Freq_Attribute *freq_attribute;
    Processor  *cpu;

    unsigned int get() override;

    // For (at least) the 18f family, there's a 4X PLL that effects the
    // the relative timing between gpsim's cycle counter (which is equivalent
    // to the cumulative instruction count) and the external oscillator. In
    // all parts, the clock source for the timer is fosc, the external oscillator.
    // However, for the 18f parts, the instructions execute 4 times faster when
    // the PLL is selected.

    virtual unsigned int get_prescale();

    virtual unsigned int get_tmr1cs() { return ((value.get() & TMR1CS) ? 2 : 0); }
    virtual bool get_tmr1on() { return (value.get() & TMR1ON); }
    virtual bool  get_t1oscen() { return (value.get() & T1OSCEN); }
    virtual bool get_tmr1GE() { return (value.get() & TMR1GE); }
    virtual bool get_t1GINV() { return (value.get() & T1GINV); }
    virtual bool get_t1sync() { return (value.get() & T1SYNC); }
    void put(unsigned int new_value) override;

    double t1osc();
};


class T1GCon_GateSignalSink;
class T1CON_G;

// Timer 1 gate control Register
class T1GCON : public sfr_register, public apfpin
{
public:
    T1GCON(Processor *pCpu, const char *pName, const char *pDesc = nullptr, T1CON_G *t1con_g = nullptr);
    ~T1GCON();

    enum
    {
        T1GSS0	= 1 << 0,		// Gate source select bits
        T1GSS1	= 1 << 1,
        T1GVAL  = 1 << 2,		// Current state bit
        T1GGO	= 1 << 3,		// Gate Single-Pulse Acquisition Status bit
        T1GSPM	= 1 << 4, 	// Gate Single-Pulse Mode bit
        T1GTM	= 1 << 5,		// Gate Toggle Mode bit
        T1GPOL	= 1 << 6,		// Gate Polarity bit
        TMR1GE  = 1 << 7,		// Gate Enable bit
    };


    void put(unsigned int new_value) override;
    void setIOpin(PinModule *pin, int arg = 0) override { (void) arg; setGatepin(pin); }
    virtual void setGatepin(PinModule *);
    virtual void PIN_gate(bool);
    virtual void T0_gate(bool);
    virtual void T2_gate(bool);
    virtual void CM1_gate(bool);
    virtual void CM2_gate(bool);
    virtual void new_gate(bool);
    void set_WRmask(unsigned int mask) { write_mask = mask; }
    void set_tmrl(TMRL *_tmrl) { tmrl = _tmrl; }
    virtual void setInterruptSource(InterruptSource * _int) { m_Interrupt = _int; }
    virtual InterruptSource * getInterruptSource() { return m_Interrupt; }
    virtual bool get_tmr1GE() { return (value.get() & TMR1GE); }
    bool get_t1GPOL() { return (value.get() & T1GPOL); }
    virtual bool tmr1_isON();
    virtual void on_or_off(int new_state);

private:
    T1GCon_GateSignalSink *sink = nullptr;
    unsigned int write_mask;
    TMRL	*tmrl = nullptr;
    T1CON_G	*t1con_g;
    InterruptSource *m_Interrupt = nullptr;
    bool 	PIN_gate_state = false;
    bool 	T0_gate_state = false; // can also be Tx = PRx where x=2,4,6
    bool 	CM1_gate_state = false;
    bool 	CM2_gate_state = false;
    bool 	last_t1g_in = false;
    bool	t1g_in_val = false;   ///< post toggle but pre-SPM - between t1g_in and T1GVAL on block diagram
    bool	wait_trigger = false;
    PinModule	*gate_pin = nullptr;
};


//
// T1CON_G	combines T1CON and T1GCON into one virtual register
//
class T1CON_G : public T1CON
{
public:
    T1CON_G(Processor *pCpu, const char *pName, const char *pDesc = nullptr);
    ~T1CON_G();

    enum
    {
        TMR1ON  = 1 << 0,
        T1SYNC  = 1 << 2,
        T1OSCEN = 1 << 3,
        T1CKPS0 = 1 << 4,
        T1CKPS1 = 1 << 5,
        TMR1CS0  = 1 << 6,
        TMR1CS1  = 1 << 7,
    };

    void t1_cap_increment();

    // RRR unsigned int get();

    // For (at least) the 18f family, there's a 4X PLL that effects the
    // the relative timing between gpsim's cycle counter (which is equivalent
    // to the cumulative instruction count) and the external oscillator. In
    // all parts, the clock source for the timer is fosc, the external oscillator.
    // However, for the 18f parts, the instructions execute 4 times faster when
    // the PLL is selected.

    unsigned int get_prescale() override
    {
        return (value.get() & (T1CKPS0 | T1CKPS1)) >> 4;
    }

    unsigned int get_tmr1cs() override
    {
        return (value.get() & (TMR1CS1 | TMR1CS0)) >> 6;
    }
    bool get_tmr1on() override { return value.get() & TMR1ON; }
    bool get_t1oscen() override { return value.get() & T1OSCEN; }
    bool get_t1sync() override { return value.get() & T1SYNC; }
    void put(unsigned int new_value) override;
    bool get_tmr1GE() override { return t1gcon.get_tmr1GE(); }
    bool get_t1GINV() override { return true; }

    TMRL  *tmrl = nullptr;
    TMR1_Freq_Attribute *freq_attribute = nullptr;
    T1GCON t1gcon;
};


//---------------------------------------------------------
// TMRL & TMRH - Timer 1
class TMRH : public sfr_register
{
public:
    TMRH(Processor *pCpu, const char *pName, const char *pDesc = nullptr);

    void put(unsigned int new_value) override;
    unsigned int get() override;
    unsigned int get_value() override;

    TMRL *tmrl = nullptr;
};


class TMR1CapComRef;

class TMRL : public sfr_register, public TriggerObject, public SignalSink, public apfpin
{
public:
    TMRL(Processor *pCpu, const char *pName, const char *pDesc = nullptr);
    ~TMRL();

    TMRH  *tmrh = nullptr;
    T1CON *t1con = nullptr;

    unsigned int
    prescale,
    prescale_counter,
    break_value;
    unsigned int value_16bit = 0;         /* Low and high concatenated */

    double ext_scale;			// scale clock frequency
    bool   have_lfintosc = false;	// tmrxcs = 3 is lfintosc ie 31khz

    TMR1CapComRef * compare_queue = nullptr;

    guint64 synchronized_cycle = 0;
    guint64 future_cycle = 0;
    gint64 last_cycle = 0;

    void callback() override;
    void callback_print() override;

    void set_ext_scale();

    void release() override;

    void put(unsigned int new_value) override;
    unsigned int get() override;
    unsigned int get_value() override;
    virtual unsigned int get_low_and_high();
    virtual void on_or_off(int new_state);
    virtual void current_value();
    virtual void new_clock_source();
    void update() override;
    virtual void clear_timer();
    void setSinkState(char) override;
    void setIOpin(PinModule *pin, int arg = 0) override;
    virtual void setGatepin(PinModule *);
    virtual void IO_gate(bool);
    virtual void compare_gate(bool);
    virtual void setInterruptSource(InterruptSource *);
    virtual InterruptSource * getInterruptSource() { return m_Interrupt; }
    virtual void sleep();
    virtual void wake();

    void set_compare_event(unsigned int value, CCPCON *host);
    void clear_compare_event(CCPCON *host);

    void set_T1GSS(bool arg);
    virtual void increment();   // Used when TMR1 is attached to an external clock
    CLC_BASE       *m_clc[4];
    DATA_SERVER     *get_tmr135_server();

private:
    char	   tmr_number=1;
    DATA_SERVER	    *tmr135_overflow_server = nullptr;
    TMR1_Interface *tmr1_interface = nullptr;
    char m_cState;
    bool m_GateState = false;		// Only changes state if setGatepin() has been called
    bool m_compare_GateState;
    bool m_io_GateState;
    bool m_bExtClkEnabled = false;
    bool m_sleeping = false;
    bool m_t1gss;			// T1 gate source
    // true - IO pin controls gate,
    // false - compare controls gate
    InterruptSource *m_Interrupt = nullptr;
};


class PR2 : public sfr_register
{
public:
    PR2(Processor *pCpu, const char *pName, const char *pDesc = nullptr);

    void put(unsigned int new_value) override;

    TMR2 *tmr2 = nullptr;
};


//---------------------------------------------------------
// T2CON - Timer 2 control register

class T2CON : public sfr_register
{
public:
    T2CON(Processor *pCpu, const char *pName, const char *pDesc = nullptr);

    enum
    {
        T2CKPS0 = 1 << 0,
        T2CKPS1 = 1 << 1,
        TMR2ON  = 1 << 2,
        TOUTPS0 = 1 << 3,
        TOUTPS1 = 1 << 4,
        TOUTPS2 = 1 << 5,
        TOUTPS3 = 1 << 6
    };

    inline unsigned int get_t2ckps0() { return (value.get() & T2CKPS0); }

    inline unsigned int get_t2ckps1() { return (value.get() & T2CKPS1); }

    virtual unsigned int get_tmr2on() { return (value.get() & TMR2ON); }

    virtual unsigned int get_post_scale()
    {
        return ((value.get() & (TOUTPS0 | TOUTPS1 | TOUTPS2 | TOUTPS3)) >> 3)+1;
    }

    virtual unsigned int get_pre_scale()
    {
        //  ps1:ps0 prescale
        //   0   0     1
        //   0   1     4
        //   1   x     16
        if (value.get() & T2CKPS1) { return 16; }
        else if (value.get() & T2CKPS0) { return 4; }
        else { return 1; }
    }

    void put(unsigned int new_value) override;

    TMR2 *tmr2 = nullptr;
};


//---------------------------------------------------------
// T2CON_64 - Timer 2 control register with prescale including 64

class T2CON_64 : public T2CON
{
public:
    T2CON_64(Processor *pCpu, const char *pName, const char *pDesc = nullptr)
        : T2CON(pCpu, pName, pDesc)
    {
    }

    unsigned int get_pre_scale() override
    {
        //  ps1:ps0 prescale
        //   0   0      1
        //   0   1      4
        //   1   0     16
        //   1   1     64
        switch (value.get() & (T2CKPS1 |  T2CKPS0))
        {
        case 0:
            return 1;
            break;

        case 1:
            return 4;
            break;

        case 2:
            return 16;
            break;

        case 3:
            return 64;
            break;
        }

        return 1;	// just to shutup compiler
    }
};

//---------------------------------------------------------
// T2CON_128 - Timer 2 control register with prescale including 128

class T2CON_128 : public T2CON
{
public:
    T2CON_128(Processor *pCpu, const char *pName, const char *pDesc = nullptr, TMR246_WITH_HLT *_pt_hlt = nullptr)
        : T2CON(pCpu, pName, pDesc),
	  pt_hlt(_pt_hlt)
    {
    }

    enum
    {
	TMR2ON = 1<<7,
	CKPS_mask = 0x70,
        CKPS_shift = 4,
	OUTPS_mask = 0x0f
    };

    void put(unsigned int) override;
    unsigned int get_pre_scale() override;
    unsigned int get_post_scale() override;
    unsigned int get_tmr2on() override { return (value.get() & TMR2ON); }

    TMR246_WITH_HLT *pt_hlt;
};


#define MAX_PWM_CHANS   5
class TMR2_Interface;

//---------------------------------------------------------
// TMR2 - Timer
class TMR2 : public sfr_register, public TriggerObject
{
protected:
    CCPCON * ccp[MAX_PWM_CHANS];

public:
    TMR2(Processor *pCpu, const char *pName, const char *pDesc = nullptr);
    ~TMR2();

    /* Define the way in which the tmr2 callback function may be updated. */
    enum TMR2_UPDATE_TYPES
    {
        TMR2_WRAP        = 1 << 0,	 // wrap TMR2
        TMR2_PR2_UPDATE  = 1 << 1,     // update pr2 match
        TMR2_PWM1_UPDATE = 1 << 2,     // wrt ccp1
        // PWM must be last as a variable number of channels follows
        TMR2_ANY_PWM_UPDATE = 0xfc,    // up to six PWM channels
        TMR2_DONTCARE_UPDATE = 0xff,    // whatever comes next
	TMR2_PAUSE	 = 1 << 8,	// waiting for pause
	TMR2_RESET	 = 1 << 9
    };

    int 	pwm_mode = 0;
    int 	update_state;
    int 	last_update = 0;

    bool	 enabled = true; // allow external control of timer
    bool	 running = false;
    bool	 use_clk = true;	// emulate clock frequency
    unsigned int prescale;		// counts before timer is incremented
    unsigned int prescale_counter = 0;	// value of prescale counter
    unsigned int break_value = 0;	// relative cycle counter to timer zero
    unsigned int duty_cycle[MAX_PWM_CHANS];     /* for ccp channels */
    int 	 post_scale = 0;	
    // zero_cycle cycle when tmr is first zero.
    // if tmr is paused or prescale changed, zero_cycle must be ajusted 
    gint64 	 zero_cycle = 0;	// cycles since timer was zero 
    guint64	 future_cycle = 0;	// cycles to next break point
    unsigned int last_delta = 0;	// Used for gate pause
    double	 clk_ratio = 1.0;	// Instruction cycles per clock tick

    PR2  	 *pr2 = nullptr;
    PIR_SET 	 *pir_set = nullptr;
    T2CON 	 *t2con = nullptr;
    TMRx_HLT     *tx_hlt = nullptr;
    TMRx_CLKCON  *tmrx_clkcon = nullptr;
    SSP_MODULE	 *ssp_module[2];
    T1GCON	 *m_txgcon = nullptr;
    CLC_BASE     *m_clc[4];

    void 	  callback() override;
    void 	  callback_print() override;
    void	 set_clk_ratio(double ratio) { clk_ratio = ratio;}
    double	 get_clk_ratio() { return clk_ratio;}
    void	 set_enable(bool on, bool zero = false);
    virtual unsigned int max_counts() { return 256; }
    void 	 put(unsigned int new_value) override;
    unsigned int get() override;
    void 	 on_or_off(int new_state);
    void	 reset_edge();
    void	 reset_value(bool on);
    void	 zero_tmr246();
    void 	 new_pre_post_scale();
    void 	 new_pr2(unsigned int new_value);
    void 	 current_value();
    void 	 update(int ut = TMR2_DONTCARE_UPDATE);
    void 	 pwm_dc(unsigned int dc, unsigned int ccp_address);
    void 	 stop_pwm(unsigned int ccp_address);
    void	 pr2_match();
    void	 new_t2_edge();
    unsigned int get_value() override;
    virtual void setInterruptSource(InterruptSource * _int) { m_Interrupt = _int; }
    virtual InterruptSource * getInterruptSource() { return m_Interrupt; }
    char         tmr_number;		// should be '2', '4' or '6'
    bool 	 add_ccp(CCPCON * _ccp);
    bool	 rm_ccp(CCPCON *_ccp);
    unsigned int next_break();

    InterruptSource *m_Interrupt = nullptr;
    void	 out_clc(bool level, char index);
    void	 clc_data(bool v1, int cm);
    virtual void increment();   // Used when TMR2 is attached to an external clock
    bool	 count_from_zero();

    enum
    {
	MATCH     = 0x100,
	POSTSCALE = 0x200,
	MASK	  = 0x300,
    };

    DATA_SERVER     *get_tmr246_server();
    TMR2_Interface *tmr2_interface = nullptr;
    DATA_SERVER	    *tmr246_server = nullptr;
};


//---------------------------------------------------------
//
// TMR2_MODULE
//
//

class TMR2_MODULE
{
public:
    TMR2_MODULE();

    void initialize(T2CON *t2con, PR2 *pr2, TMR2  *tmr2);

    _14bit_processor *cpu = nullptr;
    char * name_str = nullptr;
    T2CON *t2con = nullptr;
    PR2   *pr2 = nullptr;
    TMR2  *tmr2 = nullptr;
};

class TMRx_RST : public sfr_register, public TriggerObject
{
public:

    enum RST {
	T2INPPS = 0x0,
	C1OUT_sync,
	C2OUT_sync,
	CCP1_out,
	CCP2_out,
	TMR2_postscale,
	TMR4_postscale,
	TMR6_postscale,
	ZCD1_out,
	LC1_out = 0x9,
        LC2_out = 0xa,
	LC3_out = 0xb,
	LC4_out = 0xc,
	PWM3_out = 0xd,
	PWM4_out = 0xe
    };

    enum ACTION { NOP = 0, START, RESET, STOP, STOP_ZERO};
    TMRx_RST(TMR246_WITH_HLT *tmrx_hlt, Processor *pCpu, const char *pName, const char *pDesc = nullptr);
    ~TMRx_RST();
    void put(unsigned int new_value);
    void clc_data_rst(bool v1, int v2);
    void zcd_data_rst(bool v1, int v2);
    void TMRx_ers(bool state);
    bool get_TMRx_ers() { return TMRx_ers_state;}

private:
    Tx_RST_RECEIVER *pt_rst_receiver = nullptr;
    TMR246_WITH_HLT *tmrx_hlt;
    bool	TMRx_ers_state = true;
    void 	callback();
    void	set_delay();
    guint64     future_cycle = 0;
    ACTION 	action = NOP;
};
class TMRx_HLT : public sfr_register
{
public:

    enum
    {
	PSYNC = (1<<7),
	CKPOL = (1<<6),
	CKSYNC = (1<<5),
	MODE40 = 0x1f
    };
    TMRx_HLT(TMR246_WITH_HLT *tmrx_hlt, Processor *pCpu, const char *pName, const char *pDesc = nullptr);
    ~TMRx_HLT();
    void put(unsigned int new_value);
    bool get_ckpol() { return value.get() & CKPOL; }

private:
    TMR246_WITH_HLT *tmrx_hlt;
};

class CLKCON_SignalSink;

class TMRx_CLKCON : public sfr_register,  public TriggerObject,  public SignalSink, public apfpin
{
public:

    enum CLK_SRC {
        FOSC4	= 0,
        FOSC	= 1,
        HFINTOSC = 2,
        LFINTOSC = 3,
	ZCD1_out = 4,
        MFINTOSC = 5,
	T2INPPS = 6,
	LC1_out = 7,
        LC2_out = 8,
	LC3_out = 9,
	LC4_out = 0xa,
	AT1_PERCLK = 0xb
    };
    TMRx_CLKCON(TMR246_WITH_HLT *tmrx_hlt, Processor *pCpu, const char *pName, const char *pDesc = nullptr);
    ~TMRx_CLKCON();
    void    put(unsigned int new_value);
    void    clc_data_clk(bool v1, int cm);
    void    zcd_data_clk(bool v1, int cm);
    void    at1_data_clk(bool v1, int cm);
    bool    get_ckpol();
    int     old_data_clk = -1;
    void     setIOpin(PinModule *pin, int arg = 0) override ;
    void     setSinkState(char) override;
    void     release() override {}

private:
    TMR246_WITH_HLT *tmrx_hlt;
    Tx_CLK_RECEIVER *pt_clk_receiver = nullptr;
    PinModule       *m_PinModule = nullptr;
    bool 	    sink_active = false;
    bool 	    last_state = false;
};


// T2,4,6 type timer as Hardware Limit Timer
class TMR246_WITH_HLT : public TriggerObject
{
public:
    TMR246_WITH_HLT(Processor *pCpu, char time_number);
    ~TMR246_WITH_HLT();

    enum
    {
        TMR2ON = 1<<7,
        CKPS_mask = 0x70,
        CKPS_shift = 4,
        OUTPS_mask = 0x0f
    };
    enum UPDATE_TYPES
    {
        WRAP        = 1 << 0,	 // wrap TMR2
        PR2_UPDATE  = 1 << 1,     // update pr2 match
        PWM1_UPDATE = 1 << 2,     // wrt ccp1
        ANY_PWM_UPDATE = 0xfc,    // up to six PWM channels
        DONTCARE_UPDATE = 0xff    // whatever comes next
    };

    void	tmr_on(bool);
    void 	set_pr2_buf() {pr2_buf = pr246.value.get();}
    virtual unsigned int max_counts() { return 256; }
    void	current_value();
    void 	new_pre_post_scale();
    virtual void setInterruptSource(InterruptSource * _int) 
			{ m_Interrupt = _int; }
    unsigned int get_prescale() { return pre_scale;}
    unsigned int get_postscale() { return post_scale;}

    DATA_SERVER *get_cm_data_server(unsigned int cm);
    DATA_SERVER *get_zcd_data_server();
    DATA_SERVER *get_at1_data_server();
    DATA_SERVER *get_tmr246_server(int t_number);
    DATA_SERVER *get_pwm_server(int index);
    DATA_SERVER *get_ccp_server(int index);
    void        set_clc(CLC_BASE *clc1, CLC_BASE *clc2=nullptr, 
		        CLC_BASE *clc3 = nullptr, CLC_BASE *clc4 = nullptr); 
    void	set_zcd(ZCDCON *zcd) { m_zcd = zcd;}
    ZCDCON	*get_zcd(){return m_zcd;}
    void	set_at1(ATx *_at1) {m_at1 = _at1;}
    ATx		*get_at1(){return m_at1;}
    void	set_tmr246(TMR246_WITH_HLT *t2, TMR246_WITH_HLT *t4, TMR246_WITH_HLT *t6);
    void	set_pt_pwm(PWMxCON *pt1, PWMxCON *pt2, PWMxCON *pt3, PWMxCON *pt4);
    void	set_m_ccp(CCPCON *p1, CCPCON *p2, CCPCON *p3=nullptr, CCPCON *p4=nullptr, CCPCON *p5=nullptr);
		    

    PR2   	 pr246;
    T2CON_128 	 t246con;
    TMR2  	 tmr246;
    TMRx_HLT     t246HLT;
    TMRx_CLKCON  t246CLKCON;
    TMRx_RST     t246RST;
    char	 tmr_number;
  
private:
    Processor   *pCpu;
    int 	 pwm_mode = 0;
    unsigned int pre_scale = 1;
    unsigned int post_scale = 1;
    unsigned int pr2_buf = 0;
    guint64      last_cycle = 0;
    guint64      future_cycle = 0;
    CCPCON 	*m_ccp[MAX_PWM_CHANS];
    unsigned int duty_cycle[MAX_PWM_CHANS];     /* for ccp channels */
    int 	 last_update = 0;
    int 	 update_state;
    CLC_BASE    *m_clc[4];
    ZCDCON	*m_zcd = nullptr;
    ATx		*m_at1 = nullptr;
    SSP_MODULE	*ssp_module[2];
    InterruptSource *m_Interrupt = nullptr;
    T1GCON	*m_txgcon = nullptr;
    char 	name[10];
    TMR246_WITH_HLT *m_tmr246[3];
    PWMxCON 	*m_pwm[4];
};

//---------------------------------------------------------
//
// TMR1_MODULE
//
//

class TMR1_MODULE
{
public:
    TMR1_MODULE();

    void initialize(T1CON *t1con, PIR_SET *pir_set);

    _14bit_processor *cpu;
    char * name_str;
    T1CON *t1con;
    PIR_SET  *pir_set;
};


// Select either T2, T4 or T6 for each CCP/PWM and PWM
class CCPTMRS14 : public sfr_register
{
public:
    CCPTMRS14(Processor *pCpu, const char *pName, const char *pDesc = nullptr);

    enum
    {
        C1TSEL0 = 1 << 0,   // CCP1 (pwm1)
        C1TSEL1 = 1 << 1,
        C2TSEL0 = 1 << 2,   // CCP2 (pwm2)
        C2TSEL1 = 1 << 3,
        P3TSEL0 = 1 << 4,   // PWM3
        P3TSEL1 = 1 << 5,
        P4TSEL0 = 1 << 6,   // PWM4
        P4TSEL1 = 1 << 7,
    };

    void set_tmr246(TMR2 *_t2, TMR2 *_t4, TMR2 *_t6)
    {
        t2 = _t2;
        t4 = _t4;
        t6 = _t6;
    }
    void set_ccp(CCPCON *_c1, CCPCON *_c2, CCPCON *_c3, CCPCON *_c4)
    {
        ccp[0] = _c1;
        ccp[1] = _c2;
        ccp[2] = _c3;
        ccp[3] = _c4;
    }
    void put(unsigned int value) override;

    TMR2 *t2 = nullptr;
    TMR2 *t4 = nullptr;
    TMR2 *t6 = nullptr;
    CCPCON *ccp[4];
};

class CCP_CLC_RECEIVER;

// CCP Capture input source
class CCPxCAP : public sfr_register
{
public:

    enum
    {
	CCPxPin = 0,
	C1_out,
	C2_out,
	IOC_int,
	LC1_out,
	LC2_out
    };
    CCPxCAP(Processor *pCpu, const char *pName, const char *pDesc = nullptr, CCPCON_FMT *_ccp_fmt = nullptr);
    ~CCPxCAP();
    void put(unsigned int new_value);
    DATA_SERVER *get_clc_data_server(unsigned int n_clc);
    DATA_SERVER *get_cm_data_server();
    void clc_data_ccp(bool state, unsigned int m_clc);
    void set_comparator(ComparatorModule2 *_pt_cm){ pt_cm = _pt_cm;}

private:
    CCPCON_FMT	*ccp_fmt;
    ComparatorModule2 *pt_cm = nullptr;
    CCP_CLC_RECEIVER *pt_clc_receiver = nullptr;
    
};

#endif
