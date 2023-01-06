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


#ifndef SRC_AT_H_
#define SRC_AT_H_

#include <glib.h>

#include "registers.h"
#include "stimuli.h"
#include "ioports.h"
#include "trigger.h"
#include "pir.h"
#include "pie.h"


class ATx; 
class ATxCCy;
class ATx_RECEIVER;
class Processor;
class CLC_BASE;
class ZCDCON;
class PinModule;
class ATSIG_SignalSink;
class ATCCy_SignalSink;
class ComparatorModule2; 

// emulate D flipflop, reset on high level, clk on +edge
class dflipflop
{
public:
    dflipflop(bool st=false, bool lr=false, bool lc=false) :
        state(st), last_reset(lr), last_clk(lc)
    {}

    bool dff(bool in, bool clk, bool reset)
    {
        // reset level
        if (reset)
        {
            state = 0;
            last_reset = reset;
        }
        // positive edge clock
        else if ((clk != last_clk) && clk)
        {
            state = in;
        }
        last_clk = clk;
        return state;
    }

private:
    bool state;
    bool last_reset;
    bool last_clk;
};




class ATxCON0 : public sfr_register
{
public:
    ATxCON0(Processor *pCpu, const char *pName, const char *pDesc, ATx * );

    enum
    {
	EN = 1<<7,
	PREC = 1<<6,
	PS   = (1<<5 | 1<<4),
	POL  = 1<<3,
	APMOD = 1<<1,
	MODE  = 1<<0
    };

    void put(unsigned int new_value);

    

protected:
    ATx  *pt_atx;
};

class ATxCON1 : public sfr_register
{
public:
    ATxCON1(Processor *pCpu, const char *pName, const char *pDesc, ATx *);

    enum
    {
	PHP = 1<<6,
	PRP = 1<<4,
	MPP = 1<<2,
	ACCS = 1<<1,	// read only
	VALID = 1<<0	// read only
    };

    void put(unsigned int new_value);

protected:
    ATx  *pt_atx;
};

class ATxCLK : public sfr_register
{
public:
    ATxCLK(Processor *pCpu, const char *pName, const char *pDesc, ATx *);
    void put(unsigned int new_value);

    enum
    {
	CS0 = 1<<0
    };
protected:
    ATx  *pt_atx;
};

//class ATxSIG : public sfr_register, public SignalSink, public apfpin
class ATxSIG : public sfr_register, public apfpin
{
public:
    ATxSIG(Processor *pCpu, const char *pName, const char *pDesc, ATx *, unsigned int mask = 7);
    ~ATxSIG(){}
    void put(unsigned int new_value);
    void put_value (unsigned int new_value);
    void clc_data_in(bool, int);
    void zcd_data_in(bool, int);
    void cmp_data_in(bool, int);
    void setIOpin(PinModule *pin, int arg);
    void set_inpps(bool state);
    void disable_SSEL();
    void enable_SSEL();

    enum
    {
	ATxINPPS = 0,
	CMP1,
	CMP2,
	ZCD1,
	LC1,
	LC2,
	LC3,
	LC4
    };

protected:
    ATx             *pt_atx = nullptr;
   ATx_RECEIVER     *pt_atx_receiver = nullptr;
   PinModule        *m_PinModule=nullptr;
   bool             sink_active = false;
   ATSIG_SignalSink *sink = nullptr;;
   unsigned int     mask = 7;
   bool		    last_pin_state;
};

class ATxRESH : public sfr_register
{
public:
    ATxRESH(Processor *pCpu, const char *pName, const char *pDesc, ATx *);
    void put(unsigned int new_value); 
protected:
    ATx  *pt_atx;
};

class ATxRESL : public sfr_register, public TriggerObject
{
public:
    ATxRESL(Processor *pCpu, const char *pName, const char *pDesc, ATx *);
    void put(unsigned int new_value); 
    void put_value(unsigned int new_value); 
    void LD_PER_ATxsig();
    void res_start_stop(bool on);
    void callback() override;
    void callback_print() override;

protected:
    guint64	future_cycle = 0;
    guint	res16bit;
    guint	period_counter = 0;
    ATx  	*pt_atx;
};

class ATxMISSH : public sfr_register
{
public:
    ATxMISSH(Processor *pCpu, const char *pName, const char *pDesc, ATx *);
    void put(unsigned int new_value); 
protected:
    ATx  *pt_atx;
};

class ATxMISSL : public sfr_register
{
public:
    ATxMISSL(Processor *pCpu, const char *pName, const char *pDesc, ATx *);
    void put(unsigned int new_value); 
protected:
    ATx  *pt_atx;
};

//read only
class ATxPERH : public sfr_register
{
public:
    ATxPERH(Processor *pCpu, const char *pName, const char *pDesc, ATx *);
    void put(unsigned int new_value) {} 
protected:
    ATx  *pt_atx;
};

//read only
class ATxPERL : public sfr_register
{
public:
    ATxPERL(Processor *pCpu, const char *pName, const char *pDesc, ATx *);
    void put(unsigned int new_value) {} 
protected:
    ATx  *pt_atx;
};

// Read only
class ATxPHSH : public sfr_register
{
public:
    ATxPHSH(Processor *pCpu, const char *pName, const char *pDesc, ATx *);
    void put(unsigned int new_value) {;} 
protected:
    ATx  *pt_atx;
};

// Read only
class ATxPHSL : public sfr_register, public TriggerObject
{
public:
    ATxPHSL(Processor *pCpu, const char *pName, const char *pDesc, ATx *);
    ~ATxPHSL();
    void    put(unsigned int new_value) {;} 
    void    phs_cnt_rst_ATxsig();
    void    phs_start_stop(bool on);
    void    callback() override;
    void    callback_print() override;
    guint64 next_break();
    void    add_node(ATxCCy *_pt_ccy, unsigned int _atxccy);
    bool    delete_node(ATxCCy *_pt_ccy);
    bool    match_data(unsigned int _atxccy);

protected:
    // linked list to hold ATxCCy values for CCL compare mode 
    struct node {
        unsigned int atxccy;
	ATxCCy *pt_ccy;
        struct node *next;
    };
    struct node *ccy_head = nullptr; 

    guint64	future_cycle;
    guint64	zero_cycle;
    ATx         *pt_atx;
    bool	delay_output = false;
};

class ATxIE0 : public PIE
{
public:
    ATxIE0(Processor *pCpu, const char *pName, const char *pDesc, ATx *);
protected:
    ATx  *pt_atx;
};

class ATxIR0 : public PIR
{
public:
    ATxIR0(Processor *pCpu, const char *pName, const char *pDesc, ATx *, 
	INTCON *, PIE *, int );
    
    void put(unsigned int new_value)  override;
    unsigned int ir_active() {return value.get() & pie->value.get();}

    enum
    {
	PERIF  = 1<<0,
	MISSIF = 1<<1,
	PHSIF  = 1<<2
    };
    void set_perif() { put(get() | PERIF);}
    void set_missif() { put(get() | MISSIF);}
    void set_phsif() { put(get() | PHSIF);}

protected:
    ATx  *pt_atx;
};

class ATxIE1 : public PIE
{
public:
    ATxIE1(Processor *pCpu, const char *pName, const char *pDesc, ATx *);


protected:
    ATx  *pt_atx;
};

class ATxIR1 : public PIR
{
public:
    ATxIR1(Processor *pCpu, const char *pName, const char *pDesc, ATx *, 
		INTCON * , PIE *, int);
    void put(unsigned int new_value)  override;
    unsigned int ir_active() {return value.get() & pie->value.get();}

    enum
    {
	CC1IF = 1<<0,
	CC2IF = 1<<1,
	CC3IF = 1<<2
    };

    void set_cc1if() { put(get() | CC1IF);}
    void set_cc2if() { put(get() | CC2IF);}
    void set_cc3if() { put(get() | CC3IF);}

protected:
    ATx  *pt_atx;
};

class ATxSTPTH : public sfr_register
{
public:
    ATxSTPTH(Processor *pCpu, const char *pName, const char *pDesc, ATx *);
    void put(unsigned int new_value);
protected:
    ATx  *pt_atx;
};

class ATxSTPTL : public sfr_register
{
public:
    ATxSTPTL(Processor *pCpu, const char *pName, const char *pDesc, ATx *);
    void put(unsigned int new_value);
protected:
    ATx  *pt_atx;
};

// Read only register
class ATxERRH : public sfr_register
{
public:
    ATxERRH(Processor *pCpu, const char *pName, const char *pDesc, ATx *);
    void put(unsigned int new_value) {} 
protected:
    ATx  *pt_atx;
};

// Read only register
class ATxERRL : public sfr_register
{
public:
    ATxERRL(Processor *pCpu, const char *pName, const char *pDesc, ATx *);
    void put(unsigned int new_value) {} 
protected:
    ATx  *pt_atx;
};

class ATxCCONy : public sfr_register
{
public:
    ATxCCONy(Processor *pCpu, const char *pName, const char *pDesc, ATxCCy *);
    void put(unsigned int new_value);

    enum
    {
	CCyMODE = 1<<0,
	CAPyP   = 1<<3,
	CCyPOL  = 1<<4,
	CCyEN   = 1<<7
    };

protected:
    ATxCCy *pt_ccy;
    unsigned int mask = 0x99;
};

// Capture input select register
class ATxCSELy : public sfr_register
{
public:
    ATxCSELy(Processor *pCpu, const char *pName, const char *pDesc, ATxCCy *);

    void put(unsigned int new_value);    

protected:
    ATxCCy  *pt_ccy;
    unsigned int mask = 0x3;
};

class ATxCCyH : public sfr_register
{
public:
    ATxCCyH(Processor *pCpu, const char *pName, const char *pDesc, ATxCCy *);
    unsigned int get() override;
    unsigned int get_value() override;
    void put(unsigned int new_value) override;
protected:
    ATxCCy  *pt_ccy;
};

class ATxCCyL : public sfr_register
{
public:
    ATxCCyL(Processor *pCpu, const char *pName, const char *pDesc, ATxCCy *);
    unsigned int get() override;
    unsigned int get_value() override;
    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;
protected:
    ATxCCy  *pt_ccy;
};

// Capture Compare module part of Angular Timer module
class ATxCCy : public apfpin
{
public:
    explicit ATxCCy(Processor *pCpu, ATx *_ATx, int _y);
    void     setIOpin(PinModule *pin, int arg);
    void     set_inpps(bool state);
    void     enable_IOpin();
    void     disable_IOpin();
    void     ccy_compare();

    ATxCSELy	cc_csel;
    ATxCCyL	cc_ccl;
    ATxCCyH	cc_cch;
    ATxCCONy    cc_ccon;

    unsigned int CCy16bit = 0x000;
    InterruptSource *m_Interrupt = nullptr;
    ATx	  *pt_atx;
    int   y;
protected:
   PinModule        *m_PinModule=nullptr;
   bool             sink_active = false;
   bool		    last_pin_state;
   ATCCy_SignalSink *sink = nullptr;;
};

// Angular Timer Module
class ATx
{
public:

    ATx(Processor *pCpu, INTCON *_intcon);
    unsigned int STPT16bit = 0; //time set point
    unsigned int MISS16bit = 0;	//missing phase delay

    // DATA_SERVER signal codes
    enum
    {
	PERCLK	= 0x100,
	MISSPUL = 0x200,
	PHSCLK  = 0x300,
	CMP1	= 0x400,
	CMP2	= 0x500,
	CMP3	= 0x600,
	ATxMask = 0xf00,
    };

    ATxCON0	at_con0;
    ATxCON1	at_con1;
    ATxCLK	at_clk;
    ATxSIG	at_sig;
    ATxRESH	at_resh;
    ATxRESL	at_resl;
    ATxMISSH	at_missh;
    ATxMISSL	at_missl;
    ATxPERH	at_perh;
    ATxPERL	at_perl;
    ATxPHSH	at_phsh;
    ATxPHSL	at_phsl;
    ATxSTPTH	at_stpth;
    ATxSTPTL	at_stptl;
    ATxERRH	at_errh;
    ATxERRL	at_errl;
    ATxIE0	at_ie0;
    ATxIR0	at_ir0;
    ATxIE1	at_ie1;
    ATxIR1	at_ir1;

    ATxCCy	at_cc1;
    ATxCCy	at_cc2;
    ATxCCy	at_cc3;

    ZCDCON       *get_zcd(){return m_zcd;}
    void         set_zcd(ZCDCON *zcd) { m_zcd = zcd;}
    void	 set_clc(int cm, CLC_BASE *clc){m_clc[cm-1] = clc;}
    void	 set_cmp(ComparatorModule2 *cmp) { m_cmp = cmp;}
    ComparatorModule2 *get_cmp() {return m_cmp; }
    CLC_BASE     *get_clc(int n) { return m_clc[n];}
    DATA_SERVER  *get_atx_data_server() { return atx_data_server;}
    DATA_SERVER  *get_zcd_data_server();
    DATA_SERVER  *get_clc_data_server(unsigned int cm);
    DATA_SERVER  *get_cmp_data_server();
    bool	 atx_is_on() { return (at_con0.value.get() & ATxCON0::EN);}
    void         start_stop(bool on);
    void	 ATxsig(bool state);
    void	 send_perclk();
    void	 send_phsclk();
    void	 send_missedpulse(bool out);
    bool	 is_valid() { return (at_con1.value.get() & ATxCON1::VALID);}
    bool 	 get_mode() {return at_con0.value.get() & ATxCON0::MODE;}
    bool 	 get_apmod() {return at_con0.value.get() & ATxCON0::APMOD;}
    void 	 set_accs(bool acss);
    bool	 get_prec() { return at_con0.value.get() & ATxCON0::PREC; }

    double	ATxclk_freq();
    void	set_pir(PIR *_pir, unsigned int bit)
   		{
		    base_pir=_pir; 
		    base_pir_mask = 1<<bit;
		}

    void	set_interrupt() {base_pir->setInterrupt(base_pir_mask);}
    void	clr_interrupt() 
		    {base_pir->put_value(base_pir->value.get() & ~base_pir_mask);}
    PIR		*base_pir;
    unsigned int base_pir_mask;

private:
    Processor	*cpu;
    CLC_BASE    *m_clc[4];
    ZCDCON      *m_zcd = nullptr;
    ComparatorModule2      *m_cmp = nullptr;
    unsigned int valid_cnt;
    DATA_SERVER *atx_data_server = nullptr;
    bool        dff_d1 = false;
    bool        dff_d3 = false;
    bool        dff_d4 = false;
    bool        dff_r3 = false;
    bool        multi_pulse(bool missed_pulse, bool Atx_in, bool &atsig, bool &atper);
    dflipflop    ff1;
    dflipflop    ff3;
    dflipflop    ff4;

};
#endif //SRC_AT_H_
