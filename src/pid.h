/*
   Copyright (C) 2022 Roy R Rankin

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

// Math accelerator with Proportional-Integral- Derivative (PID) MODULE

#ifndef SRC_PID_h_
#define SRC_PID_h_

#include "processor.h"
#include "registers.h"
#include "pir.h"
#include "trigger.h"
#include "trace.h"

class PID;

// Writing to this register triggers PID computation (if enabled)
class PIDxINl : public sfr_register
{
public:

    PIDxINl(PID *_m_pid, Processor *pCpu, const char *pName, const char *pDesc);
    ~PIDxINl() {}

    void put(unsigned int new_value) override;

private:
    PID *m_pid;
};

class PIDxCON : public sfr_register
{
public:

    PIDxCON(Processor *pCpu, const char *pName, const char *pDesc) :
	sfr_register(pCpu, pName, pDesc), write_mask(0x87)
    {}
    void put(unsigned int);
    void set_write_bits(unsigned int mask) { write_mask = mask;}

private:
    unsigned int write_mask;
};

class PID : public TriggerObject
{
public:

    enum 
    {
	EN = 1<<7,
	BUSY = 1<<6,
	MODE2 = 1<<2,
	MODE1 = 1<<1,
	MODE0 = 1<<0,
    };

    PID(Processor *pCpu);
    ~PID();

    PIDxINl pidXinL;
    sfr_register pidXinH;
    sfr_register pidXsetH;
    sfr_register pidXsetL;
    sfr_register pidXk1H;
    sfr_register pidXk1L;
    sfr_register pidXk2H;
    sfr_register pidXk2L;
    sfr_register pidXk3H;
    sfr_register pidXk3L;
    sfr_register pidXoutU;
    sfr_register pidXoutHH;
    sfr_register pidXoutHL;
    sfr_register pidXoutLH;
    sfr_register pidXoutLL;
    sfr_register pidXaccU;
    sfr_register pidXaccHH;
    sfr_register pidXaccHL;
    sfr_register pidXaccLH;
    sfr_register pidXaccLL;
    sfr_register pidXz1U;
    sfr_register pidXz1H;
    sfr_register pidXz1L;
    sfr_register pidXz2U;
    sfr_register pidXz2H;
    sfr_register pidXz2L;
    PIDxCON      pidXcon;
    

    void	callback() override;
    void 	new_pidxinl();
    guint64 	get_OUT();
    gint64 	get_sOUT();
    guint64 	get_ACC();
    gint64	get_sACC();			
    void   	put_ACC(guint64 ACC_val);
    void   	put_OUT(guint64 OUT_val);
    void	put_Z1(guint64);
    void	put_Z2(guint64);
    void	set_pidXdif(InterruptSource *_pidXdif) { pidXdif = _pidXdif;}
    void	set_pidXeif(InterruptSource *_pidXeif) { pidXeif = _pidXeif;}
private:
    InterruptSource *pidXdif;	// pid complete interrupt
    InterruptSource *pidXeif;	// pid overflow interrupt
    gint64 	OUT;
    guint64  	future_cycle = 0;

};


#endif //SRC_PID_h_
