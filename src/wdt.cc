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

// Windowed Watchdog Timer (WDT)


#include "wdt.h"

//#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("%s:%d-%s() ",__FILE__,__LINE__,__FUNCTION__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

void WDTCON::put(unsigned int new_value)
{
    unsigned int masked_value = new_value & valid_bits;
    trace.raw(write_trace.get() | value.get());
    value.put(masked_value);

    if (valid_bits > 1)
    {
        cpu_pic->wdt->set_prescale(masked_value >> 1);
    }

    if (cpu_pic->swdten_active())
    {
        cpu_pic->wdt->swdten((masked_value & SWDTEN) == SWDTEN);
    }
}

void WDTCON::reset(RESET_TYPE )
{
    putRV(por_value);
}



void WDTCON0::put(unsigned int new_value)
{
    unsigned int old = value.get();
    if (wdps_readonly)
    {
	 unsigned int rdonlybits = value.get() & WDTPS_mask;
	 new_value = (new_value &  0x01) | rdonlybits;
    }
    else
    {
	
	new_value &= valid_bits;
    }

    if (!(old^new_value)) return;

    trace.raw(write_trace.get() | value.get());
    put_value(new_value);


}
void WDTCON0::put_value(unsigned int new_value)
{
    unsigned int diff = value.get()^new_value;
    value.put(new_value);
    if (diff & WDTPS_mask)
    {
	unsigned int wdtps = (new_value & WDTPS_mask)>>1;
	if (wdtps > 0x12) wdtps = 0;
	win_wdt->set_prescale(wdtps);
    }
    if (diff & SEN)
        win_wdt->swdten(new_value & SEN);
}

void WDTCON0::reset(RESET_TYPE r)
{
    Dprintf(( "WDTCON0::reset r=0x%x\n", r));

    switch (r)
    {
    case POR_RESET:
    case EXIT_RESET:
	//por_value.put(rst_value);
        //value.put(rst_value);
	Dprintf(( "WDTCON0::reset POR or EXIT rst=0x%x 0x%x\n", rst_value, por_value.get()));
        break;

    case MCLR_RESET:
	Dprintf(( "WDTCON0::reset MCLR\n"));
/*
        if (future_cycle)
        {
            get_cycles().clear_break(this);
        }

        future_cycle = 0;
*/
        break;

    default:
	Dprintf(( "WDTCON0::reset ???\n"));
        ;
    }

   
    put_value(rst_value);
    //putRV(RegisterValue(rst_value,0));
}


void WDTCON1::put(unsigned int new_value)
{
    unsigned int old = value.get();
    if (wdtcs_readonly)
    {
	 unsigned int rdonlybits = value.get() & WDTCS_mask;
	 new_value = (new_value &  ~WDTCS_mask) | rdonlybits;
    }
    if (window_readonly)
    {
	 unsigned int rdonlybits = value.get() & WINDOW_mask;
	 new_value = (new_value &  ~WINDOW_mask) | rdonlybits;
    }

    if (!(old^new_value)) return;

    trace.raw(write_trace.get() | value.get());
    put_value(new_value);


}
void WDTCON1::put_value(unsigned int new_value)
{
    value.put(new_value);

    // Window
    unsigned int window = (new_value & WINDOW_mask);
    Dprintf(( "WDTCON1 new window=%d\n", window));
    win_wdt->low_window = 7 - window;

    // clock select
    double timeout;
    unsigned int wdtcs = (new_value & WDTCS_mask)>>WDTCS_shift;
    if (wdtcs == 0)
        timeout = 1./31000.;
    else if (wdtcs == 1)
        timeout = 1./31250.;
    else
    {
        Dprintf(( "WDT::config wdtcs=%d which is a reserved value\n", wdtcs));
        timeout = 1./31250.;
    }
    Dprintf(( "WDTCON1 new clock select=%d freq=%.0fkHz\n", wdtcs, 1./timeout));
    win_wdt->set_timeout(timeout);
}
void WDTCON1::reset(RESET_TYPE r)
{
    Dprintf(( "WDTCON1::reset r=0x%x por_value=0x%x\n", r, por_value.get()));
    put_value(por_value.get());
}


// update precount value and return register value
unsigned int WDTPSL::get()
{
    win_wdt->WDT_counter();
    return value.get();
}
// update precount value and return register value
unsigned int WDTPSH::get()
{
    win_wdt->WDT_counter();
    return value.get();
}
// update precount value and return register value
unsigned int WDTTMR::get()
{
    win_wdt->WDT_counter();
    return value.get();
}
