
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

#include "at.h"
#include <assert.h>     // for assert
#include <iostream>
#include "processor.h"  // for Processor
#include "trace.h"      // for Trace, trace
#include "clc.h"
#include "zcd.h"
#include "comparator.h"

#define DEBUG 0
#if DEBUG == 1
#define RRprint(arg) {fprintf arg;}
#else
#define RRprint(arg) {}
#endif
#if DEBUG == 2
#define Dprintf(arg) {printf("%s:%d %s ",__FILE__,__LINE__, __FUNCTION__); printf arg; }
#else
#define Dprintf(arg) {}
#endif


// capture ATxINPPS input pin
class ATSIG_SignalSink : public SignalSink
{
public:

    ATSIG_SignalSink(ATxSIG *_pt_sig)
        : pt_sig(_pt_sig)
    {}

    virtual void release() override
    {
        delete this;
    }
    void setSinkState(char new3State) override
    {
        pt_sig->set_inpps(new3State == '1' || new3State == 'W');
    }


private:
    ATxSIG *pt_sig;
};

// capture ATiCCyPPS input pin
class ATCCy_SignalSink : public SignalSink
{
public:

    ATCCy_SignalSink(ATxCCy *_pt_CCy)
        : pt_CCy(_pt_CCy)
    {}

    virtual void release() override
    {
        delete this;
    }
    void setSinkState(char new3State) override
    {
        pt_CCy->set_inpps(new3State == '1' || new3State == 'W');
    }


private:
    ATxCCy *pt_CCy;
};

// Catch state changes from other modules
class ATx_RECEIVER : public DATA_RECEIVER
{
public:
    explicit ATx_RECEIVER(ATxSIG *_pt_sig, const char *_name) :
        DATA_RECEIVER(_name), pt_sig(_pt_sig)
    {}
    virtual ~ATx_RECEIVER() {}
    void rcv_data(int v1, int v2) override;

private:
    ATxSIG *pt_sig;

};
void ATx_RECEIVER::rcv_data(int v1, int v2)
{
    switch(v2 & DATA_SERVER::SERV_MASK)
    {
    case DATA_SERVER::CLC:
        pt_sig->clc_data_in(v1, v2 & ~DATA_SERVER::SERV_MASK);
        break;

    case DATA_SERVER::ZCD:
        pt_sig->zcd_data_in(v1, v2 & ~DATA_SERVER::SERV_MASK);
        break;

    case DATA_SERVER::CM:
        pt_sig->cmp_data_in(v1, v2 & ~DATA_SERVER::SERV_MASK);
        break;

    default:
        fprintf(stderr, "ATx_RECEIVER unexpected server 0x%x\n", v2 & DATA_SERVER::SERV_MASK);
        break;
    }

}



ATxCON0::ATxCON0(Processor *pCpu, const char *pName, const char *pDesc, ATx *_atx )
    : sfr_register(pCpu, pName, pDesc),
      pt_atx(_atx)
{
    assert(pt_atx);
}

void ATxCON0::put(unsigned int new_value)
{
    new_value &= (EN|PREC|PS|POL|APMOD|MODE);
    unsigned int diff = value.get() ^ new_value;
    if (diff == 0) return;
    emplace_value_trace<trace::WriteRegisterEntry>();
    value.put(new_value);

    RRprint((stderr, "ATxCON0::put val=0x%x prescale=%d\n", new_value, 1<< ((new_value & PS) >> 4)));
    if (diff & EN)
        pt_atx->start_stop(new_value);
}

ATxCON1::ATxCON1(Processor *pCpu, const char *pName, const char *pDesc, ATx *_atx )
    : sfr_register(pCpu, pName, pDesc),
      pt_atx(_atx)
{
    assert(pt_atx);
}
void ATxCON1::put(unsigned int new_value)
{
    unsigned int old = value.get();
    new_value &= (PHP|PRP|MPP);
    new_value |= (old & (ACCS|VALID));
    emplace_value_trace<trace::WriteRegisterEntry>();
    value.put(new_value);
    if (old) {}
}

ATxCLK::ATxCLK(Processor *pCpu, const char *pName, const char *pDesc, ATx *_atx )
    : sfr_register(pCpu, pName, pDesc),
      pt_atx(_atx)
{
    assert(pt_atx);
}

void ATxCLK::put(unsigned int new_value)
{
    unsigned int old = value.get();
    new_value &= (CS0);
    emplace_value_trace<trace::WriteRegisterEntry>();
    value.put(new_value);
    if (old) {}
}
ATxSIG::ATxSIG(Processor *pCpu, const char *pName, const char *pDesc, ATx *_atx, unsigned int _mask )
    : sfr_register(pCpu, pName, pDesc),
      pt_atx(_atx), mask(_mask)
{
    assert(pt_atx);
    pt_atx_receiver = new ATx_RECEIVER(this, "ATx_SSEL");
}

void ATxSIG::put(unsigned int new_value)
{

    new_value &= mask;
    RRprint((stderr, "ATxSIG::put new=%d old=%d\n", new_value, value.get()));
    if (value.get() ^ new_value)	// bit change
    {
        emplace_value_trace<trace::WriteRegisterEntry>();
        put_value(new_value);
    }
}

void ATxSIG::disable_SSEL()
{
    RRprint((stderr, "ATxSIG::disable_SSEL() value=%d m_PinModule=%p\n", value.get(), m_PinModule));
    switch(value.get())
    {
    case ATxINPPS:
        if (m_PinModule)
        {
            RRprint((stderr, "\tATxSIG::disable_SSEL() %s sink_active=%d sink=%p\n", m_PinModule->getPin()->name().c_str(), sink_active, sink));
            if (sink_active)
                m_PinModule->removeSink(sink);
            m_PinModule->getPin()->newGUIname("");
            sink_active = false;
        }
        break;

    case CMP1:
    case CMP2:
        pt_atx->get_cmp_data_server()->detach_data(pt_atx_receiver);
        break;

    case ZCD1:
        pt_atx->get_zcd_data_server()->detach_data(pt_atx_receiver);
        break;

    case LC1:
        pt_atx->get_clc_data_server(0)->detach_data(pt_atx_receiver);
        break;

    case LC2:
        pt_atx->get_clc_data_server(1)->detach_data(pt_atx_receiver);
        break;

    case LC3:
	if (pt_atx->get_clc_data_server(2))
            pt_atx->get_clc_data_server(2)->detach_data(pt_atx_receiver);
        break;

    case LC4:
	if (pt_atx->get_clc_data_server(3))
            pt_atx->get_clc_data_server(3)->detach_data(pt_atx_receiver);
        break;
    }
}
void ATxSIG::enable_SSEL()
{
    RRprint((stderr, "ATxSIG::enable_SSEL() value=%d m_PinModule=%p\n", value.get(), m_PinModule));
    switch(value.get())
    {
    case ATxINPPS:
        if (m_PinModule)
        {
            std::string pin_name = name().substr(0, 3) + "in";
            RRprint((stderr, "\t pin name=%s sink=%p", m_PinModule->getPin()->name().c_str(), sink));
            if (!sink)
		sink = new ATSIG_SignalSink(this);
	    else if (sink_active)
                m_PinModule->removeSink(sink);
            m_PinModule->addSink(sink);
            sink_active = true;
            m_PinModule->getPin()->newGUIname(pin_name.c_str());
            last_pin_state = m_PinModule->getPin()->getState();
            RRprint((stderr, " last_pin_state=%d sink=%p\n", last_pin_state, sink));
        }
        break;

    case CMP1:
    case CMP2:
        pt_atx->get_cmp_data_server()->attach_data(pt_atx_receiver);
        break;

    case ZCD1:
        pt_atx->get_zcd_data_server()->attach_data(pt_atx_receiver);
        break;

    case LC1:
        pt_atx->get_clc_data_server(0)->attach_data(pt_atx_receiver);
        break;

    case LC2:
        pt_atx->get_clc_data_server(1)->attach_data(pt_atx_receiver);
        break;

    case LC3:
	if (pt_atx->get_clc_data_server(2))
            pt_atx->get_clc_data_server(2)->attach_data(pt_atx_receiver);
        break;

    case LC4:
	if (pt_atx->get_clc_data_server(3))
            pt_atx->get_clc_data_server(3)->attach_data(pt_atx_receiver);
        break;
    }
}

void ATxSIG::put_value(unsigned int new_value)
{

    RRprint((stderr, "ATxSIG::put_value new=%d old=%d\n", new_value, value.get()));

    if (new_value == value.get()) return;
    disable_SSEL();

    value.put(new_value);


    if (pt_atx->atx_is_on())
        enable_SSEL();
}


void ATxSIG::set_inpps(bool state)
{
    // ignore state if not an edge
    if (state == last_pin_state) return;

    RRprint((stderr, "ATxSIG::set_inpps state=%d last_pin_state=%d\n", state, last_pin_state));
    last_pin_state = state;
    pt_atx->ATxsig(state);
}

void ATxSIG::setIOpin(PinModule *pin, int arg)
{
    RRprint((stderr, "ATxSIG::setIOpin pin=%p arg=%d ssel=%d ", pin, arg, value.get()));
    RRprint((stderr, " pin name=%s is_on=%d \n", pin->getPin()->name().c_str(), pt_atx->atx_is_on()));

    if (value.get() == ATxINPPS && (pin != m_PinModule) && pt_atx->atx_is_on())
    {
        if (!sink) sink = new ATSIG_SignalSink(this);
        if (sink_active)
            m_PinModule->removeSink(sink);
        pin->addSink(sink);
        sink_active = true;
    }
    m_PinModule = pin;
}
void ATxSIG::clc_data_in(bool v1, int cm)
{
    unsigned int ssel = value.get();
    RRprint((stderr, "%s::clc_data_in ssel=%d v1=%d cm=%d\n", name().c_str(), ssel, v1, cm));

    // check if SSEl matches input
    if ((cm == 0 && ssel == LC1) ||
            (cm == 1 && ssel == LC2) ||
            (cm == 2 && ssel == LC3) ||
            (cm == 3 && ssel == LC4) )
    {
	pt_atx->ATxsig(v1);
        return;             // call not for active case
    }
}

void ATxSIG::zcd_data_in(bool v1, int cm)
{
    unsigned int ssel = value.get();

    RRprint((stderr, "%s zcd_data_in(v1=%d cm=%d) sig=%d\n", name().c_str(), v1, cm, ssel));
    if (ssel == ZCD1)
	pt_atx->ATxsig(v1);
}

void ATxSIG::cmp_data_in(bool v1, int cm)
{
    unsigned int ssel = value.get();

    RRprint((stderr, "%s cm_data_in(v1=%d cm=%d) sig=%d\n", name().c_str(), v1, cm, ssel));

    if ((ssel == CMP1 && cm == 0) || (ssel == CMP2 && cm == 1))
	pt_atx->ATxsig(v1);
}


ATxRESH::ATxRESH(Processor *pCpu, const char *pName, const char *pDesc, ATx *_atx )
    : sfr_register(pCpu, pName, pDesc),
      pt_atx(_atx)
{
    assert(pt_atx);
}
// writing to this register clears valid flag of ATxCON1
void ATxRESH::put(unsigned int new_value)
{
    new_value &= 0x03;
    emplace_value_trace<trace::WriteRegisterEntry>();
    value.put(new_value);
    pt_atx->at_con1.put(pt_atx->at_con1.value.get() & ~ATxCON1::VALID);
}

ATxRESL::ATxRESL(Processor *pCpu, const char *pName, const char *pDesc, ATx *_atx )
    : sfr_register(pCpu, pName, pDesc),
      pt_atx(_atx)
{
    assert(pt_atx);
}
// writing to this register clears valid flag of ATxCON1
void ATxRESL::put(unsigned int new_value)
{
    new_value &= 0xff;
    emplace_value_trace<trace::WriteRegisterEntry>();
    put_value(new_value);
}

void ATxRESL::put_value(unsigned int new_value)
{
    value.put(new_value);
    pt_atx->at_con1.put(pt_atx->at_con1.value.get() & ~ATxCON1::VALID);
    res16bit = (pt_atx->at_resh.value.get() << 8) | new_value;
    RRprint((stderr, "ATxRESL::put_value val=0x%x res16bit=0x%x\n", new_value, res16bit));
}

// start or stop RES counter
void ATxRESL::res_start_stop(bool on)
{
    RRprint((stderr, "ATxRESL::res_start_stop on=%d\n", on));
    if (on)
    {
        uint64_t fc;
        double xclk = pt_atx->ATxclk_freq();
        double cps = get_cycles().instruction_cps();
        unsigned int delta = (res16bit + 1) * cps / xclk;
        if (xclk > cps)
            printf("Warning ATx xclk > FOSC/4 possible lose of accuracy\n");
        delta = (res16bit + 1) * cps / xclk;
        fc = get_cycles().get() + delta;
        RRprint((stderr, "\tATxRESL::res_start_stop now=%ld xclk=%e cps=%e delta=%d fc=%ld future_cycle=%ld\n", get_cycles().get(), xclk, cps, delta, fc, future_cycle));
        if (future_cycle)
            get_cycles().reassign_break(future_cycle, fc, this);
        else
  	{
            get_cycles().set_break(fc, this);
	    future_cycle = fc;
	}
    }
    else
    {
	RRprint((stderr, "ATxRESL::res_start_stop STOP future_cycle=%ld now=%ld\n", future_cycle, get_cycles().get()));
        if (future_cycle)
        {
            get_cycles().clear_break(this);
            future_cycle = 0;
        }
    }
}

// edge of ATxsig
// Load ATxPER from period counter and reset period counter
//
void ATxRESL::LD_PER_ATxsig()
{
    int setpoint_diff;
    RRprint((stderr, "ATxRESL::LD_PER_ATxsig() period_counter=%d remainder=%ld\n", period_counter, future_cycle - get_cycles().get()));
// set/clear ATxCON1::ACCS
    period_counter--;
// 	transfer period_counter ot ATxPER and clear period_counter
    pt_atx->at_perl.put_value(period_counter & 0xff);
    pt_atx->at_perh.put_value(period_counter >> 8);

    setpoint_diff = (int)period_counter - (int)pt_atx->STPT16bit;
    RRprint((stderr, "\t%d(per) - %d(stpt) = %d(err)\n", period_counter, pt_atx->STPT16bit, setpoint_diff));
    pt_atx->at_errl.put_value(setpoint_diff & 0xff);
    pt_atx->at_errh.put_value((setpoint_diff & 0xff00) >> 8);
    period_counter = 0;

//	if PREC then restart divide by ATxRES+1
    if (pt_atx->get_prec())
        res_start_stop(true);
}

void ATxRESL::callback()
{
    if (pt_atx->is_valid())
    {
        int per = pt_atx->at_perl.value.get() +
                  (pt_atx->at_perh.value.get() << 8);
        int diff = period_counter - per;
        // check before period counter is incremented
        if (diff < 0)
            pt_atx->set_accs(true);
        else
            pt_atx->set_accs(false);

        if (pt_atx->get_apmod())  // missed pulse: period > 1.5 ATxPER
             pt_atx->send_missedpulse((diff >  per/2)?true:false);
        else  // missed pulse period_counter == miss
        {
            unsigned int miss = pt_atx->at_missl.value.get() +
                           (pt_atx->at_missh.value.get() << 8);
	    if (period_counter == miss)
	    {
	        RRprint((stderr, "miss=%d period_counter=%d\n", miss, period_counter));
	    }
            pt_atx->send_missedpulse(( period_counter == miss)?true:false);
        }
    }
    ++period_counter;
    future_cycle = 0;
    double xclk = pt_atx->ATxclk_freq();
    double cps = get_cycles().instruction_cps();
    unsigned int delta = (res16bit + 1) * cps / xclk;
    RRprint((stderr, "ATxRESL::callback period_counter=%d delta = %d res16bit=%d\n", period_counter, delta, res16bit));
    future_cycle = get_cycles().get() + delta;;
    get_cycles().set_break(future_cycle, this);
}

void ATxRESL::callback_print()
{
    std::cout << "ATxRESL " << name() << " CallBack ID " << CallBackID << '\n';
}


ATxMISSH::ATxMISSH(Processor *pCpu, const char *pName, const char *pDesc, ATx *_atx )
    : sfr_register(pCpu, pName, pDesc),
      pt_atx(_atx)
{
    assert(pt_atx);
}
void ATxMISSH::put(unsigned int new_value)
{
    new_value &= 0xff;
    emplace_value_trace<trace::WriteRegisterEntry>();
    value.put(new_value);
}

ATxMISSL::ATxMISSL(Processor *pCpu, const char *pName, const char *pDesc, ATx *_atx )
    : sfr_register(pCpu, pName, pDesc),
      pt_atx(_atx)
{
    assert(pt_atx);
}
void ATxMISSL::put(unsigned int new_value)
{
    new_value &= 0xff;
    emplace_value_trace<trace::WriteRegisterEntry>();
    value.put(new_value);

    pt_atx->MISS16bit = (pt_atx->at_missh.value.get() << 8) + new_value;
}

ATxPERH::ATxPERH(Processor *pCpu, const char *pName, const char *pDesc, ATx *_atx )
    : sfr_register(pCpu, pName, pDesc),
      pt_atx(_atx)
{
    assert(pt_atx);
}

ATxPERL::ATxPERL(Processor *pCpu, const char *pName, const char *pDesc, ATx *_atx )
    : sfr_register(pCpu, pName, pDesc),
      pt_atx(_atx)
{
    assert(pt_atx);
}

ATxPHSH::ATxPHSH(Processor *pCpu, const char *pName, const char *pDesc, ATx *_atx )
    : sfr_register(pCpu, pName, pDesc),
      pt_atx(_atx)
{
    assert(pt_atx);
}

ATxPHSL::ATxPHSL(Processor *pCpu, const char *pName, const char *pDesc, ATx *_atx )
    : sfr_register(pCpu, pName, pDesc),
      pt_atx(_atx)
{
    assert(pt_atx);
    future_cycle = 0;
}

ATxPHSL::~ATxPHSL()
{
    struct node *n = ccy_head;
    while(n)
    {
	RRprint((stderr, "ATxPHSL::~ATxPHSL() delete node cc%d from linked list %p\n", n->pt_ccy->y, n->pt_ccy));
	ccy_head = n->next;
	free (n);
	n = ccy_head;
    }
}

// edge of ATxperiod (= ATxsig in single mode)
// Zero phase counter
void ATxPHSL::phs_cnt_rst_ATxsig()
{
    RRprint((stderr, "ATxPHSL::phs_cnt_rst_ATxsig() phase=%d\n", get() + (pt_atx->at_phsh.value.get() << 8)));
// Zero ATxPHS and reset divider
    put_value(0);
    pt_atx->at_phsh.put_value(0);
    phs_start_stop(true);
}

// start or stop PHS counter
void ATxPHSL::phs_start_stop(bool on)
{
    RRprint((stderr, "ATxPHSL::phs_start_stop on=%d\n", on));
    if (on)
    {
        uint64_t fc = next_break();
        RRprint((stderr, "\t fc=%ld future_cycle=%ld\n", fc, future_cycle));
        if (future_cycle)
            get_cycles().reassign_break(future_cycle, fc, this);
        else
            get_cycles().set_break(fc, this);
        // zero counter
        put_value(0);
        pt_atx->at_phsh.put_value(0);
    }
    else
    {
        delay_output = false;
        if (future_cycle)
        {
            get_cycles().clear_break(this);
            future_cycle = 0;
        }
    }
}
uint64_t ATxPHSL::next_break()
{
    unsigned int per = pt_atx->at_perl.value.get() | (pt_atx->at_perh.value.get() << 8);
    double xclk = pt_atx->ATxclk_freq();
    double cps = get_cycles().instruction_cps();
    //unsigned int delay = cps/xclk + 0.5;
    unsigned int delay = cps/xclk;
    // need to delay at most 1 instruction cycle
    if (delay == 0) delay = 1;
    //unsigned int delta = (per + 1) * cps / xclk + 0.5 ;
    unsigned int delta = (per + 1) * cps / xclk;
    if (delay_output)
    {
        delta = delay;
    }
    else
    {
        if (delta <= delay)
        {
            printf("Warning ATxPHS next clock is now so adding 1\n");
            delta = 1;
        }
        else
            delta -= delay;
    }
    RRprint((stderr, "ATxPHSL::next_break() now=%ld future_cycle=%ld delay_output=%d delay=%d delta=%d period=%d\n", get_cycles().get(), future_cycle, delay_output, delay, delta, per));
    return (get_cycles().get() + delta);
}
void ATxPHSL::callback()
{

    if (pt_atx->is_valid())
    {
        if (delay_output)
        {
            pt_atx->at_ir0.set_phsif();
            pt_atx->send_phsclk();
        }
        else
        {
            unsigned int phs = value.get() + 1 + (pt_atx->at_phsh.value.get() <<8);

            RRprint((stderr, "ATxPHSL::callback() now=%ld phs=%d\n", get_cycles().get(), phs));

            put_value(phs & 0xff);
            pt_atx->at_phsh.put_value((phs >> 8) & 0xff);
	    match_data(phs);
        }
    }

    delay_output = !delay_output;
    future_cycle = next_break();
    RRprint((stderr, "ATxPHSL::callback now=%ld future_cycle=%ld\n", get_cycles().get(), future_cycle));
    get_cycles().set_break(future_cycle, this);
}


void ATxPHSL::callback_print()
{
    std::cout << "ATxPHSL " << name() << " CallBack ID " << CallBackID << '\n';
}

// add node to link list for PHS, CCy match
void ATxPHSL::add_node(ATxCCy *pt_ccy, unsigned int atxccy)
{

    RRprint((stderr, "ATxPHSL::add_node %d pt_ccy=%p atxccy=%d\n", pt_ccy->y, pt_ccy, atxccy));
    struct node *new_node = (struct node *)malloc(sizeof(struct node));
    if (!new_node)
    {
	perror("malloc failed ATxPHSL::add_node");
        assert(new_node);
    }
    new_node->atxccy = atxccy;
    new_node->pt_ccy = pt_ccy;
    new_node->next = ccy_head;
    ccy_head = new_node;
    return;
}

// remove entry from linked list : return true if succesfull
bool ATxPHSL::delete_node(ATxCCy *pt_ccy)
{
    struct node *n = ccy_head;
    struct node *last = ccy_head;
    if (!ccy_head) return false;

    RRprint((stderr, "ATxPHSL::delete_node pt_ccy=%p\n", pt_ccy));

    while(n)
    {
        if (n->pt_ccy == pt_ccy)
        {
            RRprint((stderr, "ATxPHSL::delete_node pt_ccy=%p ccy_head=%p\n", pt_ccy, ccy_head));


            if (n == ccy_head)
                ccy_head = n->next;
            else
                last->next = n->next;
            free(n);
            return true;
        }
        n = n->next;
    }
    return false;
}
// notify CCy module of ccy match, can be more then on match
bool ATxPHSL::match_data(unsigned int atxccy)
{
    struct node *n = ccy_head;
    bool ret = false;
    while(n)
    {
        if (n->atxccy == atxccy)
	{
	RRprint((stderr, "ATxPHSL::match_data n->pt_ccy=%p node %d %d\n", n->pt_ccy, n->atxccy, n->atxccy));
	    ret = true;
	    n->pt_ccy->ccy_compare();
	}
        n = n->next;
    }
    return ret;
}



ATxIE0::ATxIE0(Processor *pCpu, const char *pName, const char *pDesc, ATx *_atx)
    : PIE(pCpu, pName, pDesc),
      pt_atx(_atx)
{
    assert(pt_atx);
}

ATxIR0::ATxIR0(Processor *pCpu, const char *pName, const char *pDesc,
               ATx *_atx, INTCON *_intcon, PIE *_pie, int _valid_bits)
    : PIR(pCpu, pName, pDesc, _intcon, _pie, _valid_bits),
      pt_atx(_atx)
{
    assert(pt_atx);
}

void ATxIR0::put(unsigned int new_value)
{

    new_value = (new_value & writable_bits) | (value.get() & ~writable_bits);
    unsigned int diff = value.get() ^ new_value;


    if (diff)
    {
        RRprint((stderr, "%s %d->%d diff=%d %s=%d\n", name().c_str(), value.get(), new_value, diff, pt_atx->at_ir1.name().c_str(), pt_atx->at_ir1.value.get()));
        emplace_value_trace<trace::WriteRegisterEntry>();
        value.put(new_value);
        // at least 1 IR and IE bit match, set base IR
        if(ir_active())
        {
            pt_atx->set_interrupt();
        }
        //if no IR,IE match need to check IR1 for similar condition
        else if (!pt_atx->at_ir1.ir_active())
        {
            pt_atx->clr_interrupt();
        }
    }


}

ATxIE1::ATxIE1(Processor *pCpu, const char *pName, const char *pDesc, ATx *_atx )
    : PIE(pCpu, pName, pDesc),
      pt_atx(_atx)
{
    assert(pt_atx);
}

ATxIR1::ATxIR1(Processor *pCpu, const char *pName, const char *pDesc,
               ATx *_atx, INTCON *_intcon, PIE *_pie, int _valid_bits)
    : PIR(pCpu, pName, pDesc, _intcon, _pie, _valid_bits),
      pt_atx(_atx)
{
    assert(pt_atx);
}

void ATxIR1::put(unsigned int new_value)
{

    new_value = (new_value & writable_bits) | (value.get() & ~writable_bits);
    unsigned int diff = value.get() ^ new_value;

    RRprint((stderr, "%s %d->%d diff=%d %s=%d\n", name().c_str(), value.get(), new_value, diff, pt_atx->at_ir1.name().c_str(), pt_atx->at_ir1.value.get()));

    if (diff)
    {
        RRprint((stderr, "\t%s=%d active=0x%x\n", pie->name().c_str(), pie->value.get(), ir_active()));
        emplace_value_trace<trace::WriteRegisterEntry>();
        value.put(new_value);
        // at least 1 IR and IE bit match, set base IR
        if(ir_active())
        {
            pt_atx->set_interrupt();
        }
        //if no IR,IE match need to check IR0 for similar condition
        else if (!pt_atx->at_ir0.ir_active())
        {
            pt_atx->clr_interrupt();
        }
    }


}
ATxSTPTH::ATxSTPTH(Processor *pCpu, const char *pName, const char *pDesc, ATx *_atx )
    : sfr_register(pCpu, pName, pDesc),
      pt_atx(_atx)
{
    assert(pt_atx);
}
void ATxSTPTH::put(unsigned int new_value)
{
    new_value &= 0x7f;
    emplace_value_trace<trace::WriteRegisterEntry>();
    value.put(new_value);
}


ATxSTPTL::ATxSTPTL(Processor *pCpu, const char *pName, const char *pDesc, ATx *_atx )
    : sfr_register(pCpu, pName, pDesc),
      pt_atx(_atx)
{
    assert(pt_atx);
}
void ATxSTPTL::put(unsigned int new_value)
{
    new_value &= 0xff;
    emplace_value_trace<trace::WriteRegisterEntry>();
    value.put(new_value);

    pt_atx->STPT16bit = (pt_atx->at_stpth.value.get() << 8) + new_value;
}

ATxERRH::ATxERRH(Processor *pCpu, const char *pName, const char *pDesc, ATx *_atx )
    : sfr_register(pCpu, pName, pDesc),
      pt_atx(_atx)
{
    assert(pt_atx);
}

ATxERRL::ATxERRL(Processor *pCpu, const char *pName, const char *pDesc, ATx *_atx )
    : sfr_register(pCpu, pName, pDesc),
      pt_atx(_atx)
{
    assert(pt_atx);
}

ATxCCyH::ATxCCyH(Processor *pCpu, const char *pName, const char *pDesc, ATxCCy *_atx_cc )
    : sfr_register(pCpu, pName, pDesc),
      pt_ccy(_atx_cc)
{
    assert(pt_ccy);
}
// read only neglect put
void ATxCCyH::put(unsigned int new_value) {}
unsigned int ATxCCyH::get()
{
    return get_value();
}
unsigned int ATxCCyH::get_value()
{
    RRprint((stderr, "ATxCCyH::get_value() %s 0x%x\n", name().c_str(), (pt_ccy->CCy16bit)));
    unsigned int ret = ((pt_ccy->CCy16bit) >> 8) & 0xff;
    value.put(ret);
    return ret;
}

ATxCCyL::ATxCCyL(Processor *pCpu, const char *pName, const char *pDesc, ATxCCy *_atx_cc)
    : sfr_register(pCpu, pName, pDesc),
      pt_ccy(_atx_cc)
{
    assert(pt_ccy);
}
// read only neglect put
void ATxCCyL::put(unsigned int new_value)
{
    // read only if CCyMODE == 1
    if (pt_ccy->cc_ccon.value.get() & ATxCCONy::CCyMODE)
	return;

    emplace_value_trace<trace::WriteRegisterEntry>();
    put_value(new_value);

}
void ATxCCyL::put_value(unsigned int new_value)
{

    value.put(new_value);
    pt_ccy->CCy16bit = new_value + (pt_ccy->cc_cch.value.get() << 8);
    if (pt_ccy->cc_ccon.value.get() & ATxCCONy::CCyEN &&
        !(pt_ccy->cc_ccon.value.get() & ATxCCONy::CCyMODE)
	)
    {
	RRprint((stderr, "ATxCCyL::put_value %s=%d node\n", name().c_str(), new_value));
	pt_ccy->pt_atx->at_phsl.add_node(pt_ccy, pt_ccy->CCy16bit);
    }
}
unsigned int ATxCCyL::get()
{
    RRprint((stderr, "ATxCCyL::get() %s %d\n", name().c_str(), (pt_ccy->CCy16bit)));
    return get_value();
}
unsigned int ATxCCyL::get_value()
{
    RRprint((stderr, "ATxCCyL::get_value() %s 0x%x\n", name().c_str(), (pt_ccy->CCy16bit)));
    unsigned int ret = (pt_ccy->CCy16bit) & 0xff;
    value.put(ret);
    return ret;
}


ATxCSELy::ATxCSELy(Processor *pCpu, const char *pName, const char *pDesc, ATxCCy *_atx_ccy)
    : sfr_register(pCpu, pName, pDesc),
      pt_ccy(_atx_ccy)
{
    assert(pt_ccy);
}


void ATxCSELy::put(unsigned int new_value)
{
    unsigned int old = value.get();
    new_value &= mask;
    if (new_value == old) return;

    emplace_value_trace<trace::WriteRegisterEntry>();
    value.put(new_value);
}

ATxCCONy::ATxCCONy(Processor *pCpu, const char *pName, const char *pDesc, ATxCCy *_atx_ccy)
    : sfr_register(pCpu, pName, pDesc),
      pt_ccy(_atx_ccy)
{
    assert(pt_ccy);
}
void ATxCCONy::put(unsigned int new_value)
{
    unsigned int old = value.get();
    new_value &= mask;
    if (new_value == old) return;

    emplace_value_trace<trace::WriteRegisterEntry>();
    value.put(new_value);
     if (old & CCyEN)
     {
	if (old & CCyMODE)
	    pt_ccy->disable_IOpin();
	else
	    pt_ccy->pt_atx->at_phsl.delete_node(pt_ccy);
     }
    if (new_value & CCyEN)
    {
	if (new_value & CCyMODE)
            pt_ccy->enable_IOpin();
	else
	    pt_ccy->pt_atx->at_phsl.add_node(pt_ccy, pt_ccy->CCy16bit);
     }

}

ATx::ATx(Processor *pCpu, INTCON *_intcon) :
    at_con0(pCpu, "", "Angular Timer Control 0 Register", this),
    at_con1(pCpu, "", "Angular Timer Control 1 Register", this),
    at_clk(pCpu, "", "Angular Timer Clock Selection Register", this),
    at_sig(pCpu, "", "Angular Timer Input Signal Selection Register", this),
    at_resh(pCpu, "", "Angular Timer Resolution High Register", this),
    at_resl(pCpu, "", "Angular Timer Resolution Low Register", this),
    at_missh(pCpu, "", "Angular Timer Missing Pulse Delay High Register", this),
    at_missl(pCpu, "", "Angular Timer Missing Pulse Delay Low Register", this),
    at_perh(pCpu, "", "Angular Timer Measured Period High Register", this),
    at_perl(pCpu, "", "Angular Timer Measured Period Low Register", this),
    at_phsh(pCpu, "", "Angular Timer Phase Counter High Register", this),
    at_phsl(pCpu, "", "Angular Timer Phase Counter Low Register", this),
    at_stpth(pCpu, "", "Angular Timer Set Point High Register", this),
    at_stptl(pCpu, "", "Angular Timer Set Point Low Register", this),
    at_errh(pCpu, "", "Angular Timer Set Point Error Value High Register", this),
    at_errl(pCpu, "", "Angular Timer Set Point Error Value Low Register", this),
    at_ie0(pCpu, "", "Angular Timer Interrupt Enable 0 Register", this),
    at_ir0(pCpu, "", "Angular Timer Interrupt Flag 0 Register", this,
           _intcon, &at_ie0, 0x7),
    at_ie1(pCpu, "", "Angular Timer Interrupt Enable 1 Register", this),
    at_ir1(pCpu, "", "Angular Timer Interrupt Flag 1 Register", this,
           _intcon, &at_ie1, 0x7),
    at_cc1(pCpu, this,1),
    at_cc2(pCpu, this, 2),
    at_cc3(pCpu, this, 3),
    cpu(pCpu)

{
    atx_data_server = new DATA_SERVER(DATA_SERVER::AT1);
    std::fill_n(m_clc, 4, nullptr);
}

// implimentation of Figure 31-3 in p16f1614 datasheet
bool ATx::multi_pulse(bool missed_pulse, bool Atx_in, bool &atsig, bool &atper)
{
    bool d1o = dff_d1;
    bool d3o = dff_d3;
    dff_d1 = ff1.dff(true, Atx_in, dff_d1);
    dff_d3 = ff3.dff(true, missed_pulse, dff_r3);
    dff_d4 = ff4.dff(d3o, d1o, false);
    dff_r3 = (!dff_d1 & dff_d4);
    atsig = (dff_d1 & !dff_d3);
    atper = (dff_d1 & dff_d3);
    if (atsig || atper)
    {
        return true;
    }
    return false;
}
// ATxsig determine active edge,
//    if active,
// 	set VALID flag after require delay
// 	set AT1PERIF bit
// 	distribute ATxsig to required functions
void ATx::ATxsig(bool state)
{
    bool atx_in = state^(bool)(at_con0.value.get() & ATxCON0::POL);
    bool atsig = true;
    bool atper = true;;
    bool is_on = at_con0.value.get() & ATxCON0::EN;


    RRprint((stderr, "ATx::ATxsig is_on=%d at_con0=0x%x state=%d atx_in=%d mode=%d\n", is_on, at_con0.value.get(), state, atx_in, at_con0.value.get() & ATxCON0::MODE));
    if (!is_on) return;
    if (at_con0.value.get() & ATxCON0::MODE) //multi pulse
    {
	if (!multi_pulse(false, atx_in, atsig, atper))
	    return;
    }
    // ignore non-active edge
    if (!atx_in)
        return;

    if (atsig)
    {
        if (valid_cnt < 2)
            valid_cnt++;
         else
         {
             if (!(at_con1.get() & ATxCON1::VALID))
                 at_con1.put_value(at_con1.value.get() | ATxCON1::VALID);
         }
         at_resl.LD_PER_ATxsig();
    }
    if (atper)
    {
         at_phsl.phs_cnt_rst_ATxsig();
        send_perclk();
        at_ir0.set_perif();
    }
}

// set or clear ACCS (acceleration sign bit) bit in ATxCON1
void ATx::set_accs(bool accs)
{
    if ((at_con1.value.get() & ATxCON1::ACCS) != accs)
    {
        RRprint((stderr, "ATx::set_accs (acceleration sign bit)  %d\n", accs));
        if (accs)
            at_con1.put_value(at_con1.value.get() | ATxCON1::ACCS);
        else
            at_con1.put_value(at_con1.value.get() & ~ATxCON1::ACCS);
    }
}

// clock frequency after prescaler
double ATx::ATxclk_freq()
{
    double freq;
    int	   prescale = 1 << ((at_con0.value.get() & ATxCON0::PS) >> 4);

    if (at_clk.value.get())
        freq = 16e6;
    else
        freq = cpu->get_frequency();

    return freq/prescale;
}

DATA_SERVER * ATx::get_zcd_data_server()
{
    if (!m_zcd)
    {
        fprintf(stderr, "***ERROR ATX::get_zcd_data_server() m_zcd not defined\n");
        assert(m_zcd);
    }
    return m_zcd->get_zcd_data_server();
}
DATA_SERVER * ATx::get_cmp_data_server()
{
    if (!m_cmp)
    {
        fprintf(stderr, "***ERROR ATX::get_cmp_data_server() m_cmp not defined\n");
        assert(m_cmp);
    }
    return m_cmp->get_CM_data_server();
}
DATA_SERVER * ATx::get_clc_data_server(unsigned int cm)
{
    if (!m_clc[cm])
    {
        fprintf(stderr, "***ERROR ATx::get_clc_data_server m_clc[%u] not defined\n", cm);
	return nullptr;
    }
    return m_clc[cm]->get_CLC_data_server();
}


void  ATx::start_stop(bool on)
{
    //turn SSEL input on/off
    on?at_sig.enable_SSEL():at_sig.disable_SSEL();

    // clear VALID flag on stop or start
    at_con1.value.put(at_con1.value.get() & ~ATxCON1::VALID);
    valid_cnt = 0;
    RRprint((stderr, "ATx::start_stop on=%d\n", on));
    at_resl.res_start_stop(on);
}

// Output perclk to other modules
void ATx::send_perclk()
{
    int state = !(at_con1.value.get() & ATxCON1::PRP);
    RRprint((stderr, "ATx::send_perclk state=%d\n", state));
    atx_data_server->send_data(state, PERCLK | DATA_SERVER::AT1);
    atx_data_server->send_data(!state, PERCLK | DATA_SERVER::AT1);
}
// Output missedpulse to other modules
void ATx::send_missedpulse(bool out)
{
    bool atsig, atper;
    static bool last_state = false;
    bool state = out ^ (at_con1.value.get() & ATxCON1::MPP);
    if (state == last_state) return;
    RRprint((stderr, "ATx::send_missedpulse state=%d\n", state));
    atx_data_server->send_data(state, MISSPUL | DATA_SERVER::AT1);
    if(multi_pulse(true, false, atsig, atper))
	fprintf(stderr, "Warning ATx::send_missedpulse multi_pulse returned true\n");
    last_state = state;
}

// Output phsclk to other modules
void ATx::send_phsclk()
{
    int state = !(at_con1.value.get() & ATxCON1::PHP);
    RRprint((stderr, "ATx::send_phsclk state=%d\n", state));
    atx_data_server->send_data(state, PHSCLK | DATA_SERVER::AT1);
    atx_data_server->send_data(!state, PHSCLK | DATA_SERVER::AT1);
}


ATxCCy::ATxCCy(Processor *pCpu, ATx *_ATx, int _y) :
    cc_csel(pCpu, "", "Angular Timer Capture Input Select Register", this),
    cc_ccl(pCpu, "", "Angular Timer Capture/Compare Low Register", this),
    cc_cch(pCpu, "", "Angular Timer Capture/Compare High Register", this),
    cc_ccon(pCpu, "", "Angular Timer Capture/Compare Control Register", this),
    pt_atx(_ATx), y(_y)

{
}

void ATxCCy::set_inpps(bool state)
{
    // ignore state if not an edge
    if (state == last_pin_state) return;

    bool cap = cc_ccon.value.get() & ATxCCONy::CAPyP;

    RRprint((stderr, "ATxCCy::set_inpps atcc%d state=%d last_pin_state=%d CAPyP=%d\n", y, state, last_pin_state, cap));

    last_pin_state = state;

    if (state != cap)	// have active edge
    {
        cc_cch.value.put(pt_atx->at_phsh.value.get());
        cc_ccl.put_value(pt_atx->at_phsl.value.get());
        pt_atx->at_ir1.put(pt_atx->at_ir1.get() | (1<<(y-1)));
        int state = !(cc_ccon.value.get() & ATxCCONy::CCyPOL);
        pt_atx->get_atx_data_server()->send_data(state, ((3+y)<<8) | DATA_SERVER::AT1);
        pt_atx->get_atx_data_server()->send_data(!state,((3+y)<<8) | DATA_SERVER::AT1);
    }
}

void ATxCCy::enable_IOpin()
{
    if (m_PinModule)
    {
        char pin_name[10];
        sprintf(pin_name, "at1cc%d", y);
        RRprint((stderr, "pin name=%s->%s sink=%p\n", m_PinModule->getPin()->name().c_str(), pin_name, sink));
        if (!sink) sink = new ATCCy_SignalSink(this);
        if (!sink_active) m_PinModule->addSink(sink);
        sink_active = true;
        m_PinModule->getPin()->newGUIname(pin_name);
        last_pin_state = m_PinModule->getPin()->getState();
        RRprint((stderr, "\tATxCCy::put_value last_pin_state=%d sink=%p\n", last_pin_state, sink));
    }
}

void ATxCCy::disable_IOpin()
{
    if (m_PinModule)
    {
        if (sink_active)
        {
            m_PinModule->removeSink(sink);
            m_PinModule->getPin()->newGUIname("");
        }
        sink_active = false;
    }
}

void ATxCCy::ccy_compare()
{
    if (pt_atx->at_con1.value.get() & ATxCON1::VALID)
    {
        RRprint((stderr, "ATxCCy::ccy_compare() node matched atcc%d\n", y));
        pt_atx->at_ir1.put(pt_atx->at_ir1.get() | (1<<(y-1)));
        int state = !(cc_ccon.value.get() & ATxCCONy::CCyPOL);
        pt_atx->get_atx_data_server()->send_data(state, ((3+y)<<8) | DATA_SERVER::AT1);
        pt_atx->get_atx_data_server()->send_data(!state,((3+y)<<8) | DATA_SERVER::AT1);
    }
}

void ATxCCy::setIOpin(PinModule *pin, int arg)
{
    char name[10];
    sprintf(name, "atcc%d", y);
    RRprint((stderr, "ATxCCy::setIOpin %s pin=%p arg=%d atxccony=%d ", name, pin, arg, cc_ccon.value.get()));
    RRprint((stderr, " pin name=%s is_on=%d \n", pin->getPin()->name().c_str(), pt_atx->atx_is_on()));

    if (pin != m_PinModule)
    {
        if (sink_active)
            disable_IOpin();

        m_PinModule = pin;
        if (pt_atx->atx_is_on() &&
                (cc_ccon.value.get() & ATxCCONy::CCyEN) &&
                (cc_ccon.value.get() & ATxCCONy::CCyMODE))
        {
            enable_IOpin();
        }
    }
}
