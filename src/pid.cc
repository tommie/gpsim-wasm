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

// Math Accelerator with Proportional-Integral- Derivative (PID) Module


//#define DEBUG
#if defined(DEBUG)
#include "../config.h"
#define Dprintf(arg) {printf("%s:%d ",__FILE__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

#include "pid.h"

PID::PID(Processor *pCpu)
    : pidXinL(this, pCpu, "pid1inl", "PID Input Low Register"),
      pidXinH(pCpu, "pid1inh", "PID Input High Register"),
      pidXsetH(pCpu, "pid1seth", "PID Set Point High Register"),
      pidXsetL(pCpu, "pid1setl", "PID Set Point Low Register"),
      pidXk1H(pCpu, "pid1k1h", "PID K1 High Register"),
      pidXk1L(pCpu, "pid1k1l", "PID K1 Low Register"),
      pidXk2H(pCpu, "pid1k2h", "PID K2 High Register"),
      pidXk2L(pCpu, "pid1k2l", "PID K2 Low Register"),
      pidXk3H(pCpu, "pid1k3h", "PID K3 High Register"),
      pidXk3L(pCpu, "pid1k3l", "PID K3 Low Register"),
      pidXoutU(pCpu, "pid1outu", "PID Output Upper Register"),
      pidXoutHH(pCpu, "pid1outhh", "PID Output High High Register"),
      pidXoutHL(pCpu, "pid1outhl", "PID Output High Low Register"),
      pidXoutLH(pCpu, "pid1outlh", "PID Output Low High Register"),
      pidXoutLL(pCpu, "pid1outll", "PID Output Low Low Register"),
      pidXaccU(pCpu, "pid1accu", "PID ACCUMULATOR Upper Register"),
      pidXaccHH(pCpu, "pid1acchh", "PID ACCUMULATOR High High Register"),
      pidXaccHL(pCpu, "pid1acchl", "PID ACCUMULATOR High Low Register"),
      pidXaccLH(pCpu, "pid1acclh", "PID ACCUMULATOR Low High Register"),
      pidXaccLL(pCpu, "pid1accll", "PID ACCUMULATOR Low Low Register"),
      pidXz1U(pCpu, "pid1z1u", "PID Z1 Upper Register"),
      pidXz1H(pCpu, "pid1z1h", "PID Z1 High Register"),
      pidXz1L(pCpu, "pid1z1l", "PID Z1 Low Register"),
      pidXz2U(pCpu, "pid1z2u", "PID Z2 Upper Register"),
      pidXz2H(pCpu, "pid1z2h", "PID Z2 High Register"),
      pidXz2L(pCpu, "pid1z2l", "PID Z2 Low Register"),
      pidXcon(pCpu, "pid1con", "PID Configuration Register"),
      pidXdif(nullptr), pidXeif(nullptr)
{
}

PID::~PID()
{
	if (pidXdif)
	    delete pidXdif;
	if (pidXeif)
	    delete pidXeif;
}

void PID::new_pidxinl()
{
    unsigned int pidxcon = pidXcon.value.get();
    if (future_cycle)
    {
	fprintf(stderr, "***Warning pidXinL called with BUSY set\n");
	get_cycles().clear_break(future_cycle);
	future_cycle = 0;
    }
    if (pidxcon & EN)
    {
	if (pidxcon & MODE2) // PID
	{
	    if ((pidxcon & (MODE0 | MODE1 | MODE2)) == 5)
	    {
		gint64 in  = (gint16)(pidXinL.value.get() | pidXinH.value.get() << 8);
		gint64 set = (gint16)(pidXsetL.value.get() | pidXsetH.value.get() << 8);
		gint64 z0 = set - in;
		gint64 k1 = (gint16)(pidXk1L.value.get() | pidXk1H.value.get() << 8);
		gint64 k2 = (gint16)(pidXk2L.value.get() | pidXk2H.value.get() << 8);
		gint64 k3 = (gint16)(pidXk3L.value.get() | pidXk3H.value.get() << 8);
		gint64 z1 = (gint8)pidXz1U.value.get();
		z1 = (z1 << 16) | pidXz1L.value.get() | (pidXz1H.value.get() << 8);
		gint64 z2 = (gint8)pidXz2U.value.get();
		z2 = (z2 << 16) | pidXz2L.value.get() | (pidXz2H.value.get() << 8);

		pidXcon.value.put(pidxcon | BUSY);

		OUT = z0 * k1 + z1 * k2 + z2 * k3 + get_sOUT();
		put_Z2(z1);
		put_Z1(z0);
		future_cycle = get_cycles().get() + 9;
		get_cycles().set_break(future_cycle, this);
	    }
	    else
	    {
		fprintf(stderr, "%s reseved mode=%d\n", pidXcon.name().c_str(),
				 pidxcon & (MODE0 | MODE1 | MODE2));
	    }
	}
	else		    // Add and multiply
	{
	    if (pidxcon & MODE1) // inputs are signed
	    {
		gint64 sA, sB, sC, sACC;
		pidXcon.value.put(pidxcon | BUSY);
		sA = (gint16)(pidXinL.value.get() | pidXinH.value.get() << 8);
		sB = (gint16)(pidXsetL.value.get() | pidXsetH.value.get() << 8);
		sC = (gint16)(pidXk1L.value.get() | pidXk1H.value.get() << 8);
		OUT = ((sA + sB) * sC);
		sACC = get_sACC();
	        if (pidxcon & MODE0)	// add accumilator
		{
		    OUT += sACC;
		}
		put_ACC(OUT);
		future_cycle = get_cycles().get() + 9;
		get_cycles().set_break(future_cycle, this);
		
	    }
	    else		// inputs are unsigned
	    {
		guint64 uA, uB, uC, ACC;
		pidXcon.value.put(pidxcon | BUSY);
		uA = pidXinL.value.get() | pidXinH.value.get() << 8;
		uB = pidXsetL.value.get() | pidXsetH.value.get() << 8;
		uC = pidXk1L.value.get() | pidXk1H.value.get() << 8;
		OUT = (uA + uB) * uC;
		ACC = get_ACC();
	        if (pidxcon & MODE0)	// add accumilator
		{
		    OUT += ACC;
		}
		put_ACC(OUT);
		future_cycle = get_cycles().get() + 9;
		get_cycles().set_break(future_cycle, this);
	    }
	}
    }
}

void PID::callback()
{
    put_OUT(OUT);
    pidXcon.value.put(pidXcon.value.get() & ~BUSY);
    if (pidXdif)
	pidXdif->Trigger();
    future_cycle = 0;
}

// Collect OUT registers into single unsigned value
guint64 PID::get_OUT()
{
    guint64 ret = pidXoutU.value.get();
    ret = (ret << 8) + pidXoutHH.value.get();
    ret = (ret << 8) + pidXoutHL.value.get();
    ret = (ret << 8) + pidXoutLH.value.get();
    ret = (ret << 8) + pidXoutLL.value.get();
    return ret;
}
// Collect ACC registers into single unsigned value
guint64 PID::get_ACC()
{
    guint64 ret = pidXaccU.value.get();
    ret = (ret << 8) + pidXaccHH.value.get();
    ret = (ret << 8) + pidXaccHL.value.get();
    ret = (ret << 8) + pidXaccLH.value.get();
    ret = (ret << 8) + pidXaccLL.value.get();
    return ret;
}
// Collect ACC registers into single signed value
gint64 PID::get_sACC()
{
    guint64 acc = get_ACC();
    gint64 ret;

    if (acc & ((guint64)1)<<34) // top bit set (negative number)
    {
	acc |= 0xfffffff800000000;
    }
    ret = acc;
    return ret;
}
// Collect OUT registers into single signed value
gint64 PID::get_sOUT()
{
    guint64 out = get_OUT();
    gint64 ret;

    if (out & ((guint64)1)<<35) // top bit set (negative number)
    {
	out |= 0xfffffff000000000;
    }
    ret = out;
    return ret;
}
void PID::put_ACC(guint64 acc)
{
    pidXaccLL.put(acc & 0xff);
    acc >>= 8;
    pidXaccLH.put(acc & 0xff);
    acc >>= 8;
    pidXaccHL.put(acc & 0xff);
    acc >>= 8;
    pidXaccHH.put(acc & 0xff);
    acc >>= 8;
    pidXaccU.put(acc & 0x07);
}

void PID::put_OUT(guint64 out)
{
    // test for overflow either signed or unsigned out
    guint64 high = out & (guint64)~0xfffffff;
    if (high != 0 && high != (guint64)~0xffffff)
    {
	fprintf(stderr, "*** Warning pidXout overflow\n");
	if (pidXeif)
	    pidXeif->Trigger();
    }
    pidXoutLL.put(out & 0xff);
    out >>= 8;
    pidXoutLH.put(out & 0xff);
    out >>= 8;
    pidXoutHL.put(out & 0xff);
    out >>= 8;
    pidXoutHH.put(out & 0xff);
    out >>= 8;
    pidXoutU.put(out & 0x0f);
}

void PID::put_Z1(guint64 z1)
{
    pidXz1L.put(z1 & 0xff);
    z1 >>= 8;
    pidXz1H.put(z1 & 0xff);
    z1 >>= 8;
    pidXz1U.put(z1 & 0x01);
}

void PID::put_Z2(guint64 z2)
{
    pidXz2L.put(z2 & 0xff);
    z2 >>= 8;
    pidXz2H.put(z2 & 0xff);
    z2 >>= 8;
    pidXz2U.put(z2 & 0x01);
}

	

PIDxINl::PIDxINl(PID *_m_pid, Processor *pCpu, 
		const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc), m_pid(_m_pid)
{
}

void PIDxINl::put(unsigned int new_value)
{
    if (new_value ^ value.get())
    {
	trace.raw(write_trace.get() | value.get());
	value.put(new_value);
    }
    m_pid->new_pidxinl();
}

void PIDxCON::put(unsigned int new_value)
{
    unsigned int fixed_bits = value.get() & ~write_mask;
    trace.raw(write_trace.get() | value.get());
    new_value = (new_value & write_mask) | fixed_bits;
    value.put(new_value);
}
