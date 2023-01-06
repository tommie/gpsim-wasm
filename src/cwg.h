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

COMPLEMENTARY WAVEFORM GENERATOR (CWG) MODULE

*/

#ifndef SRC_CWG_H_
#define SRC_CWG_H_

#include <glib.h>

#include <stdio.h>
#include <string>

#include "registers.h"
#include "trigger.h"
#include "ioports.h"

class COG;
class CWG;
class CWGSignalSource;
class FLTSignalSink;
class Processor;
class TristateControl;

class CWGxCON0 : public sfr_register
{
public:
    CWGxCON0(CWG *pt, Processor *pCpu, const char *pName, const char *pDesc);

    void put(unsigned int new_value) override;
    void set_con0_mask(unsigned int mask) { con0_mask = mask; }

private:
    CWG *pt_cwg;
    unsigned int con0_mask;
};

class CWGxCON1 : public sfr_register
{
public:
    CWGxCON1(CWG *pt, Processor *pCpu, const char *pName, const char *pDesc);

    void put(unsigned int new_value) override;
    void set_con1_mask(unsigned int mask) { con1_mask = mask; }

private:
    CWG *pt_cwg;
    unsigned int con1_mask;
};

class CWGxCON2 : public sfr_register
{
public:
    CWGxCON2(CWG *pt, Processor *pCpu, const char *pName, const char *pDesc);

    void put(unsigned int new_value) override;

private:
    CWG *pt_cwg;
    unsigned int con2_mask;
};

class CWGxDBR : public sfr_register, public TriggerObject
{
public:
    enum
    {
        CWGxDBR0 = 1 << 0,
        CWGxDBR1 = 1 << 1,
        CWGxDBR2 = 1 << 2,
        CWGxDBR3 = 1 << 3,
        CWGxDBR4 = 1 << 4,
        CWGxDBR5 = 1 << 5
    };
    CWGxDBR(CWG *pt, Processor *pCpu, const char *pName, const char *pDesc);

    void callback() override;
    void callback_print() override;
    void new_edge(bool level, double mult);
    void kill_callback();

private:
    CWG *pt_cwg;
    guint64  future_cycle = 0;
    bool     next_level = false;
};

class CWGxDBF : public sfr_register, public TriggerObject
{
public:
    enum
    {
        CWGxDBF0 = 1 << 0,
        CWGxDBF1 = 1 << 1,
        CWGxDBF2 = 1 << 2,
        CWGxDBF3 = 1 << 3,
        CWGxDBF4 = 1 << 4,
        CWGxDBF5 = 1 << 5
    };
    CWGxDBF(CWG *pt, Processor *pCpu, const char *pName, const char *pDesc);

    void callback() override;
    void callback_print() override;
    void new_edge(bool level, double mult);
    void kill_callback();

private:
    CWG *pt_cwg;
    guint64  future_cycle = 0;
    bool     next_level = false;
};

// Complementary waveform generator - 4 clock inputs
class CWG : public apfpin
{
public:
    enum
    {
        //CWG1CON0
        GxCS0  = 1 << 0,
        GxPOLA = 1 << 3,
        GxPOLB = 1 << 4,
        GxOEA  = 1 << 5,
        GxOEB  = 1 << 6,
        GxEN   = 1 << 7,
        //CWG1CON1
        GxIS0    = 1 << 0,
        GxIS1    = 1 << 1,
        GxIS2    = 1 << 2,
        GxASDLA0 = 1 << 4,
        GxASDLA1 = 1 << 5,
        GxASDLB0 = 1 << 6,
        GxASDLB1 = 1 << 7,
        //CWG1CON2
        GxASDFLT = 1 << 0,
        GxASDCLC1 = 1 << 1,
        GxARSEN  = 1 << 6,
        GxASE    = 1 << 7
    };

    enum
    {
        A_PIN   = 0,
        B_PIN
    };

    CWGxCON0 cwg1con0;
    CWGxCON1 cwg1con1;
    CWGxCON2 cwg1con2;
    CWGxDBF  cwg1dbf;
    CWGxDBR  cwg1dbr;

    explicit CWG(Processor *pCpu);
    ~CWG();

    void set_IOpins(PinModule *, PinModule *, PinModule *);
    void oeA();
    void oeB();
    void cwg_con0(unsigned int);
    void cwg_con1(unsigned int);
    void cwg_con2(unsigned int);
    void releasePin(PinModule *pin);
    void releasePinSource(PinModule *pin);
    virtual void out_pwm(bool level, char index);
    virtual void out_NCO(bool level);
    virtual void out_CLC(bool level, char index = 1);
    void input_source(bool level);
    void set_outA(bool level);
    void set_outB(bool level);
    void autoShutEvent(bool on);
    void enableAutoShutPin(bool on);
    virtual void setState(char);
    // methods of apfpin
    void setIOpin(PinModule * pin, int arg) override;

protected:
    bool       pwm_state[4];
    bool	     clc_state[4];
    bool	     nco_state = false;
    unsigned int con0_value = 0;
    unsigned int con1_value = 0;
    unsigned int con2_value = 0;
    bool         shutdown_active = false;

private:
    std::string     Agui;
    std::string     Bgui;
    std::string     FLTgui;
    Processor       *cpu;
    PinModule       *pinA = nullptr;
    PinModule       *pinB = nullptr;
    PinModule       *pinFLT = nullptr;
    TristateControl *Atri = nullptr;
    TristateControl *Btri = nullptr;
    CWGSignalSource *Asrc = nullptr;
    CWGSignalSource *Bsrc = nullptr;
    FLTSignalSink   *FLTsink = nullptr;
    bool             pinAactive = false;
    bool             pinBactive = false;
    bool             srcAactive = false;
    bool             srcBactive = false;
    bool             cwg_enabled = false;
    bool             OEA_state = false;
    bool             OEB_state = false;
    bool             active_next_edge = false;
    bool	     FLTstate = false;
};

class CWG4 : public CWG
{
public:
    explicit CWG4(Processor *pCpu);
    void out_pwm(bool level, char index) override;
    void out_NCO(bool level) override;
};

// COGxCON0: COG CONTROL REGISTER 0
class COGxCON0: public sfr_register
{
public:
    COGxCON0(COG *pt, Processor *pCpu, const char *pName, const char *pDesc);
    void put(unsigned int new_value) override;
    void set_con0_mask(unsigned int _mask) { mask = _mask; }
private:
    COG *pt_cog;
    unsigned int mask;
};

// COGxCON1: COG CONTROL REGISTER 1
class COGxCON1: public sfr_register
{
public:
    COGxCON1(COG *pt, Processor *pCpu, const char *pName, const char *pDesc);

    void put(unsigned int new_value) override;
    void set_con1_mask(unsigned int _mask) { mask = _mask; }
private:
    COG *pt_cog;
    unsigned int mask;
};

// COGxRIS: COG RISING EVENT INPUT SELECTION REGISTER
class COGxRIS: public sfr_register
{
public:
    COGxRIS(COG *pt, Processor *pCpu, const char *pName, const char *pDesc);
    void put(unsigned int new_value) override;
    void set_mask(unsigned int _mask) { mask = _mask; }
private:
    COG *pt_cog;
    unsigned int mask;
};

// COGxRSIM: COG RISING EVENT SOURCE INPUT MODE REGISTE
class COGxRSIM: public sfr_register
{
public:
    COGxRSIM(COG *pt, Processor *pCpu, const char *pName, const char *pDesc);
//	virtual void put(unsigned int new_value);
    void set_mask(unsigned int _mask) { mask = _mask; }
private:
    COG *pt_cog;
    unsigned int mask;
};

// COGxFIS: COG FALLING EVENT INPUT SELECTION REGISTER
class COGxFIS: public sfr_register
{
public:
    COGxFIS(COG *pt, Processor *pCpu, const char *pName, const char *pDesc);
    void put(unsigned int new_value) override;
    void set_mask(unsigned int _mask) { mask = _mask; }
private:
    COG *pt_cog;
    unsigned int mask;
};

// COGxFSIM: COG FALLING EVENT SOURCE INPUT MODE REGISTER
class COGxFSIM: public sfr_register
{
public:
    COGxFSIM(COG *pt, Processor *pCpu, const char *pName, const char *pDesc);
//	virtual void put(unsigned int new_value);
    void set_mask(unsigned int _mask) { mask = _mask; }
private:
    COG *pt_cog;
    unsigned int mask;
};

// COGxASD0: COG AUTO-SHUTDOWN CONTROL REGISTER 0
class COGxASD0: public sfr_register
{
public:
    COGxASD0(COG *pt, Processor *pCpu, const char *pName, const char *pDesc);
    void put(unsigned int new_value) override;
    void set_mask(unsigned int _mask) { mask = _mask; }
private:
    COG *pt_cog;
    unsigned int mask;
};


// COGxASD1: COG AUTO-SHUTDOWN CONTROL REGISTER 1
class COGxASD1: public sfr_register
{
public:
    COGxASD1(COG *pt, Processor *pCpu, const char *pName, const char *pDesc);
//	virtual void put(unsigned int new_value);
    void set_mask(unsigned int _mask) { mask = _mask; }
private:
    COG *pt_cog;
    unsigned int mask;
};

// COGxSTR: COG STEERING CONTROL REGISTER 1
class COGxSTR: public sfr_register
{
public:
    COGxSTR(COG *pt, Processor *pCpu, const char *pName, const char *pDesc);
    void put(unsigned int new_value) override;
    void set_mask(unsigned int _mask) { mask = _mask; }
private:
    COG *pt_cog;
    unsigned int mask;
};

// COGxDBR: COG RISING EVENT DEAD-BAND COUNT REGISTER
class COGxDBR: public sfr_register
{
public:
    COGxDBR(COG *pt, Processor *pCpu, const char *pName, const char *pDesc);
//	virtual void put(unsigned int new_value);
    void set_mask(unsigned int _mask) { mask = _mask; }
private:
    COG *pt_cog;
    unsigned int mask;
};

// COGxDBF: COG FALLING EVENT DEAD-BAND COUNT REGISTER
class COGxDBF: public sfr_register
{
public:
    COGxDBF(COG *pt, Processor *pCpu, const char *pName, const char *pDesc);
//	virtual void put(unsigned int new_value);
    void set_mask(unsigned int _mask) { mask = _mask; }
private:
    COG *pt_cog;
    unsigned int mask;
};

// COGxBLKR: COG RISING EVENT BLANKING COUNT REGISTER
class COGxBLKR: public sfr_register
{
public:
    COGxBLKR(COG *pt, Processor *pCpu, const char *pName, const char *pDesc);
//	virtual void put(unsigned int new_value);
    void set_mask(unsigned int _mask) { mask = _mask; }
private:
    COG *pt_cog;
    unsigned int mask;
};

// COGxBLKF: COG FALLING EVENT BLANKING COUNT REGISTER
class COGxBLKF: public sfr_register
{
public:
    COGxBLKF(COG *pt, Processor *pCpu, const char *pName, const char *pDesc);
//	virtual void put(unsigned int new_value);
    void set_mask(unsigned int _mask) { mask = _mask; }
private:
    COG *pt_cog;
    unsigned int mask;
};

// COGxPHR: COG RISING EDGE PHASE DELAY COUNT REGISTER
class COGxPHR: public sfr_register
{
public:
    COGxPHR(COG *pt, Processor *pCpu, const char *pName, const char *pDesc);
//	virtual void put(unsigned int new_value);
    void set_mask(unsigned int _mask) { mask = _mask; }
private:
    COG *pt_cog;
    unsigned int mask;
};

// COGxPHF: COG FALLING EDGE PHASE DELAY COUNT REGISTER
class COGxPHF: public sfr_register
{
public:
    COGxPHF(COG *pt, Processor *pCpu, const char *pName, const char *pDesc);
//	virtual void put(unsigned int new_value);
    void set_mask(unsigned int _mask) { mask = _mask; }
private:
    COG *pt_cog;
    unsigned int mask;
};

class COGSignalSource;
class COGTristate;
class COGSink;

class COG: public apfpin, public TriggerObject
{
public:
    explicit COG(Processor *pCpu, const char *pName = "COG");
    COG(const COG &) = delete;
    COG& operator =(const COG &) = delete;
    ~COG();

    void input_event(int index, bool level);
    void drive_bridge(int level, int state);
    void shutdown_bridge();
    void cog_con0(unsigned int);
    void cog_con1(unsigned int);
    void cog_str(unsigned int);
    void cog_asd0(unsigned int, unsigned int);
    void set_inputPin();
    void set_outputPins();
    void setIOpin(PinModule * _pin, int i) override;
    void set_pinIN(PinModule * _pinIN) { pinIN = _pinIN;}
    virtual void out_ccp(bool level, char index);
    virtual void out_pwm(bool level, char index);
    virtual void out_clc(bool level, char index);
    virtual void out_Cx(bool level, char index);
    virtual void cogx_in(char newState);
    void output_pin(int pin, bool set);
    void releasePins(int index) {printf("releasePins %d\n", index);}
    void callback() override;

    enum
    {
        //COGxCON0
        GxEN = 1 << 7,
        GxLD = 1 << 6,        // COGx Load Buffers bit
        GxCS_MASK  = 0x18,     //COGx Clock Selection bits
        GxCS_SHIFT = 3,
        GxMD_MASK  = 0x7,      // COGx Mode Selection bits

        //COGxCON1
        GxRDBS   = 1 << 7,  // COGx Rising Event Dead-band Timing Source
        GxFDBS   = 1 << 6,  // COGx Falling Event Dead-band Timing Source select bit
        GxPOLD   = 1 << 3,  // COGxD Output Polarity Control bit
        GxPOLC   = 1 << 2,  // COGxC Output Polarity Control bit
        GxPOLB   = 1 << 1,  // COGxB Output Polarity Control bit
        GxPOLA   = 1 << 0,  // COGxA Output Polarity Control bit

        //COGxRIS: COG RISING EVENT INPUT SELECTION REGISTER
        //COGxRSIM: COG RISING EVENT SOURCE INPUT MODE REGISTER
        //COGxFIS: COG FALLING EVENT INPUT SELECTION REGISTER
        //COGxFSIM: COG FALLING EVENT SOURCE INPUT MODE REGISTER
        //COGxASD0: COG AUTO-SHUTDOWN CONTROL REGISTER 0
        GxASE    = 1 << 7,  // Auto-Shutdown Event Status bit
        GxARSEN  = 1 << 6,  // Auto-Restart Enable bit
        GxASDBD_MSK = 0x30, // COGxB and COGxD Auto-shutdown Override Level
        GxASDBD_SFT = 4,
        GxASDAC_MSK = 0xc0, // COGxA and COGxC Auto-shutdown Override Level
        GxASDAC_SFT = 2,

        // COGxASD1: COG AUTO-SHUTDOWN CONTROL REGISTER 1
        GxAS3E   = 1 << 3,  // COGx Auto-shutdown Source Enable bit 3
        GxAS2E   = 1 << 2,  // COGx Auto-shutdown Source Enable bit 2
        GxAS1E   = 1 << 1,  // COGx Auto-shutdown Source Enable bit 1
        GxAS0E   = 1 << 0,  // COGx Auto-shutdown Source Enable bit 0

        //COGxSTR: COG STEERING CONTROL REGISTER 1
        GxSDATD  = 1 << 7,  // COGxD Static Output Data bit
        GxSDATC  = 1 << 6,  // COGxC Static Output Data bit
        GxSDATB  = 1 << 5,  // COGxB Static Output Data bit
        GxSDATA  = 1 << 4,  // COGxA Static Output Data bit
        GxSSTRD  = 1 << 3,  // COGxD Steering Control bit
        GxSSTRC  = 1 << 2,  // COGxC Steering Control bit
        GxSSTRB  = 1 << 1,  // COGxB Steering Control bit
        GxSSTRA  = 1 << 0,  // COGxA Steering Control bit

        // COGxDBR: COG RISING EVENT DEAD-BAND COUNT REGISTER
        // COGxDBF: COG FALLING EVENT DEAD-BAND COUNT REGISTER
        // COGxBLKR: COG RISING EVENT BLANKING COUNT REGISTER
        // COGxBLKF: COG FALLING EVENT BLANKING COUNT REGISTER
        // COGxPHR: COG RISING EDGE PHASE DELAY COUNT REGISTER
        // COGxPHF: COG FALLING EDGE PHASE DELAY COUNT REGISTER
    };

    enum {
        FALLING,
        RISING
    };

#define FIRST_STATE 0
#define PHASE_STATE 1
#define LAST_STATE  2

#define GXASE (1<<7)

    COGxCON0 cogxcon0;
    COGxCON1 cogxcon1;
    COGxRIS  cogxris;
    COGxRSIM cogxrsim;
    COGxFIS  cogxfis;
    COGxFSIM cogxfsim;
    COGxASD0 cogxasd0;
    COGxASD1 cogxasd1;
    COGxSTR  cogxstr;
    COGxDBR  cogxdbr;
    COGxDBF  cogxdbf;
    COGxBLKR cogxblkr;
    COGxBLKF cogxblkf;
    COGxPHR  cogxphr;
    COGxPHF  cogxphf;

private:
    Processor       *cpu;
    std::string	    name_str;
    PinModule       *m_PinModule[4];	// Output Pins
    COGSignalSource *m_source[4];
    bool            source_active[4];
    PinModule       *pinIN;     // Input source
    COGSink	    *cogSink;
    COGTristate     *m_tristate;
    guint64	    set_cycle;
    guint64	    reset_cycle;
    guint64	    phase_cycle;
    bool            delay_source0, delay_source1;
    bool            bridge_shutdown;
    std::string	    name() { return name_str;}
    bool	    input_set;
    bool	    input_clear;
    bool	    full_forward;
    bool	    push_pull_level;
    bool	    active_high[4];
    bool	    steer_ctl[4];
    guint8	    auto_shut_src;
    guint8      phase_val[2];
    guint8      deadband_val[2];
    guint8      blank_val[2];
};
#endif // SRC_CWG_h__

