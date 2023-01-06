/*
   Copyright (C) 1998,1999,2000 T. Scott Dattalo

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

#ifndef SRC_TMR0_H_
#define SRC_TMR0_H_

#include <glib.h>

#include "gpsim_classes.h"
#include "registers.h"
#include "stimuli.h"
#include "trigger.h"
#include "ioports.h"

class TMR0_Interface;
class T1GCON;
class ADCON2_TRIG;
class CLC_BASE;
class OPTION_REG;
class Processor;


//---------------------------------------------------------
// TMR0 - Timer
class TMR0 : public sfr_register, public TriggerObject, public SignalSink, public apfpin
{
public:
    TMR0(Processor *, const char *pName, const char *pDesc = nullptr);
    ~TMR0();

    void callback() override;
    void release() override;

    void put(unsigned int new_value) override;
    void put_value(unsigned int new_value) override;
    unsigned int get() override;
    unsigned int get_value() override;
    virtual void start(int new_value, int sync = 0);
    virtual void stop();
    virtual void increment();   // Used when tmr0 is attached to an external clock
    virtual void new_prescale();
    virtual unsigned int get_prescale();
    virtual unsigned int max_counts() { return 256; }
    virtual unsigned int get_option_reg();
    virtual bool get_t0cs();
    virtual bool get_t0se();
    virtual void set_t0if();
    virtual void set_t0xcs(bool _t0xcs) { t0xcs = _t0xcs; }
    virtual bool get_t0xcs() { return t0xcs; }
    void reset(RESET_TYPE r) override;
    void callback_print() override;
    void clear_trigger() override;

    virtual void set_cpu(Processor *, PortRegister *, unsigned int pin, OPTION_REG *);
    virtual void set_cpu(Processor *new_cpu, PinModule *pin, OPTION_REG *);
    void setIOpin(PinModule * pin, int arg = 0) override;

    void setSinkState(char) override;
    virtual void sleep();
    virtual void wake();
    void set_t1gcon(T1GCON *_t1gcon) { m_t1gcon = _t1gcon; }
    void set_adcon2(ADCON2_TRIG *_adcon2) { m_adcon2 = _adcon2; }
    void set_clc(CLC_BASE *_clc, int index) { m_clc[index] = _clc; }

    enum
    {
        STOPPED = 0,
        RUNNING = 1,
        SLEEPING = 2
    };
    unsigned int prescale;
    unsigned int prescale_counter = 0;
    unsigned int old_option = 0;       // Save option register contents here.
    unsigned int state;            // Either on or off right now.

    guint64 synchronized_cycle = 0;
    guint64 future_cycle = 0;
    gint64 last_cycle = 0;   // can be negative ...

    OPTION_REG *m_pOptionReg = nullptr;

    DATA_SERVER     *get_tmr0_server();

protected:
    T1GCON      *m_t1gcon = nullptr;
    ADCON2_TRIG *m_adcon2 = nullptr;
    CLC_BASE    *m_clc[4];

private:
    DATA_SERVER   *tmr0_server = nullptr;
    TMR0_Interface *tmr0_interface = nullptr;
    PinModule      *pin = nullptr;

    bool m_bLastClockedState = false;
    bool t0xcs = false; // clock source is the capacitive sensing oscillator
};

#endif
