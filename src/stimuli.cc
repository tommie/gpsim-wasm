/*
   Copyright (C) 1998 T. Scott Dattalo
   Copyright (C) 2006,2015 Roy R Rankin

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


#include <stdio.h>
#include <iostream>
#include <string>
#include <list>
#include <sstream>

#include <math.h>
#include <string.h>
#include <cassert>

#include "errors.h"
#include "gpsim_interface.h"
#include "gpsim_object.h"
#include "gpsim_time.h"
#include "ioports.h"
#include "stimuli.h"
#include "symbol.h"
#include "ui.h"
#include "value.h"

class Processor;


//#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("%s:%d-%s() ",__FILE__,__LINE__,__FUNCTION__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

static char num_nodes = 'a';
static int num_stimuli = 1;
void  gpsim_set_break_delta(uint64_t delta, TriggerObject *f = nullptr);

extern Processor *active_cpu;
/*
 * stimulus.cc
 *
 * This file contains some rudimentary infrastructure to support simulating
 * the environment outside of the pic. Simple net lists interconnecting pic
 * I/O pins and various signal generators may be created.
 *
 * Details:
 * There are two basic concepts behind the stimulus code: nodes and stimuli.
 * The nodes are like wires and the stimuli are like sources and loads. The
 * nodes define the interconnectivity between the stimuli. In most cases there
 * will be only two stimuli connected by one node. For example, you may wish
 * to simulate the effects of a clock input connected to porta.0 . In this case,
 * the stimuli would be the external clock and the pic I/O pin.
 */

//------------------------------------------------------------------

void Stimulus_Node::new_name(const char *cPname)
{
    std::cout << " Warning ignoring stimulus node name change from "
              << name() << " to " << cPname << '\n';
}

void Stimulus_Node::new_name(std::string &rName)
{
    new_name(rName.c_str());
}

double Stimulus_Node::get_nodeVoltage()
{
    if (future_cycle > cap_start_cycle) // RC calculation in progress, get current value
        callback();
    return voltage;
}

void dump_stimulus_list()
{
    std::cout << "Stimulus List\n";
    std::cout << "  implement -- stimuli.cc dump_stimulus_list\n";

    /*
    Symbol_Table &ST = get_symbol_table();
    Symbol_Table::stimulus_symbol_iterator it;
    Symbol_Table::stimulus_symbol_iterator itEnd = ST.endStimulusSymbol();
    for(it = ST.beginStimulusSymbol(); it != itEnd; it++) {
      stimulus *t = (*it)->getStimulus();
      if(t) {
        cout << t->name();
        //if(t->snode)
        // cout << " attached to " << t->snode->name();
        t->show();
        cout << '\n';
      }
    }
    */
}

std::string Stimulus_Node::toString()
{
    std::string out = name() + " : " + showType();

    for (stimulus *pt = stimuli; pt; pt = pt->next)
    {
        out += "\n\n " + pt->name() + pt->toString();
    }

    return out;
}

//========================================================================

Stimulus_Node::Stimulus_Node(const char *n)
    : TriggerObject(nullptr),
      cap_start_cycle(0), future_cycle(0), settlingTimeStep(0)
{
    warned  = 0;
    voltage = 0;
    Cth = 0.0;
    Zth = 0.0;
    current_time_constant = 0.0;
    delta_voltage = 0.0;
    minThreshold = 0.1; // volts
    initial_voltage = 0.0;
    DCVoltage = 0.0;
    bSettling = false;
    stimuli = 0;
    nStimuli = 0;

    if (n)
        gpsimObject::new_name(n);
    else
    {
        char name_str[100];
        snprintf(name_str, sizeof(name_str), "node%d", num_nodes);
        num_nodes++;    // %%% FIX ME %%%
        gpsimObject::new_name(name_str);
    }

    globalSymbolTable().addSymbol(this);

    gi.node_configuration_changed(this);
}

Stimulus_Node::~Stimulus_Node()
{
    //cout << "~Stimulus_Node\n";
    stimulus *sptr = stimuli;

    while (sptr)
    {
        sptr->detach(this);
        sptr = sptr->next;
    }

    globalSymbolTable().removeSymbol(this);
}

Stimulus_Node * Stimulus_Node::construct(const char * psName)
{
    gpsimObject *psn = globalSymbolTable().find(psName);

    //cout << "constructing stimulus node " << psName << endl;
    if (psn)
    {
        std::cout << "Warning ignoring node creation. A symbol with the name `"
                  << psName << "' is already in the sybmol table.\n";
        return nullptr;
    }

    return new Stimulus_Node(psName);
}

//
// Add the stimulus 's' to the stimulus list for this node
//

void Stimulus_Node::attach_stimulus(stimulus *s)
{
    if (!s)
        return;

    stimulus *sptr;

    warned = 0;

    if (stimuli)
    {
        sptr = stimuli;
        bool searching = 1;
        int nTotalStimuliConnected = 1;

        while (searching)
        {
            if (s == sptr)
                return;      // The stimulus is already attached to this node.

            nTotalStimuliConnected++;
            if (sptr->next == 0)
            {
                sptr->next = s;
                // s->next = 0;  This is done below
                searching = 0;
            }
            sptr = sptr->next;
        }

        nStimuli = nTotalStimuliConnected;
    }
    else
    {
        stimuli = s;     // This is the first stimulus attached to this node.
        nStimuli = 1;
    }

    // If we reach this point, then it means that the stimulus that we're
    // trying to attach has just been placed at the end of the the stimulus
    // list for this node. So we need to 0 terminate the singly-linked list.

    s->next = 0;

    // Now tell the stimulus to attach itself to the node too
    // (If it hasn't already.)

    s->attach(this);

    gi.node_configuration_changed(this);
}

//
// Search for the stimulus 's' in the stimulus list for this node.
// If it is found, then remove it from the list.
//

void Stimulus_Node::detach_stimulus(stimulus *s)
{
    stimulus *sptr;

    if (!s)          // You can't remove a non-existant stimulus
        return;

    if (stimuli)
    {
        if (s == stimuli)
        {
            // This was the first stimulus in the list.

            stimuli = s->next;
            s->detach(this);
            nStimuli--;

        }
        else
        {

            sptr = stimuli;

            do
            {
                if (s == sptr->next)
                {
                    sptr->next = s->next;
                    s->detach(this);
                    nStimuli--;
                    //gi.node_configuration_changed(this);
                    return;
                }

                sptr = sptr->next;
            }
            while (sptr);
        }
    }
}

//------------------------------------------------------------------------
//
// Stimulus_Node::update(uint64_t current_time)
//
// update() is called whenever a stimulus attached to this node changes
// states.

void Stimulus_Node::update(uint64_t /* current_time */ )
{
    // So far, 'update' only applies to the current time.
    update();
}

//------------------------------------------------------------------------
// refresh() - compute the Thevenin voltage and Thevenin impedance
//
void Stimulus_Node::refresh()
{
    if (stimuli)
    {
        stimulus *sptr = stimuli;

        initial_voltage = get_nodeVoltage();

        switch (nStimuli)
        {

        case 0:
            // hmm, strange nStimuli is 0, but the stimuli pointer is non null.
            break;

        case 1:
            // Only one stimulus is attached.
            DCVoltage = sptr->get_Vth();   // RP - was just voltage
            Zth =  sptr->get_Zth();
            break;

        case 2:
            // 2 stimuli are attached to the node. This is the typical case
            // and we'll optimize for it.
        {
            stimulus *sptr2 = sptr ? sptr->next : nullptr;
            if (!sptr2)
                break;     // error, nStimuli is two, but there aren't two stimuli

            double V1, Z1, C1;
            double V2, Z2, C2;
            sptr->getThevenin(V1, Z1, C1);
            sptr2->getThevenin(V2, Z2, C2);
            DCVoltage = (V1 * Z2  + V2 * Z1) / (Z1 + Z2);
            Zth = Z1 * Z2 / (Z1 + Z2);
            Cth = C1 + C2;

        }
        break;

        default:
        {
            /*
              There are 3 or more stimuli connected to this node. Recall
              that these are all in parallel. The Thevenin voltage and
              impedance for this is:

              Thevenin impedance:
              Zt = 1 / sum(1/Zi)

              Thevenin voltage:

              Vt = sum( Vi / ( ((Zi - Zt)/Zt) + 1) )
              = sum( Vi * Zt /Zi)
              = Zt * sum(Vi/Zi)
            */

            double conductance = 0.0;	// Thevenin conductance.
            Cth = 0;
            DCVoltage = 0.0;

            //cout << "multi-node summing:\n";
            while (sptr)
            {

                double V1, Z1, C1;
                sptr->getThevenin(V1, Z1, C1);
                /*
                cout << " N: " <<sptr->name()
                << " V=" << V1
                << " Z=" << Z1
                << " C=" << C1 << endl;
                */

                double Cs = 1 / Z1;
                DCVoltage += V1 * Cs;
                conductance += Cs;
                Cth += C1;
                sptr = sptr->next;
            }
            Zth = 1.0 / conductance;
            DCVoltage *= Zth;
        }
        }

        current_time_constant = Cth * Zth;
        Dprintf(("%s DCVoltage %.3f voltage %.3f Cth=%.2e Zth=%2e time_constant %fsec or %" PRINTF_INT64_T_MODIFIER "d cycles now=%" PRINTF_INT64_T_MODIFIER "d \n",name().c_str(), DCVoltage, voltage, Cth, Zth, current_time_constant, (uint64_t)(current_time_constant*get_cycles().instruction_cps()), get_cycles().get()));
        if (((uint64_t)(current_time_constant * get_cycles().instruction_cps()) < 5) ||
                (fabs(DCVoltage - voltage) < minThreshold))
        {
            if (verbose)
                std::cout << "Stimulus_Node::refresh " << name() << " use DC " <<
                          DCVoltage << " as current_time_constant=" <<
                          current_time_constant << '\n';

            if (future_cycle)	// callback is active
            {
                get_cycles().clear_break(this);
            }

            voltage = DCVoltage;
            future_cycle = 0;

        }
        else
        {
            settlingTimeStep = calc_settlingTimeStep();
            voltage = initial_voltage;

            if (verbose)
                std::cout << "Stimulus_Node::refresh " << name() << " settlingTimeStep="
                          << settlingTimeStep << " voltage=" << voltage << " Finalvoltage="
                          << DCVoltage << '\n';
            /*
            If future_cycle is not 0 we are in the middle of an RC
            calculation, but an input condition has changed.
            */

            if (future_cycle && (get_cycles().get() > cap_start_cycle))
            {
                callback();
            }
            else
            {
                if (future_cycle) get_cycles().clear_break(this);
                cap_start_cycle = get_cycles().get();
                future_cycle = cap_start_cycle + settlingTimeStep;
                get_cycles().set_break(future_cycle,this);
#ifdef DEBUG
                get_cycles().dump_breakpoints();
#endif
            }
        }

    }
}

uint64_t Stimulus_Node::calc_settlingTimeStep()
{
    /* Select a time interval where the voltage does not change more
       than about 0.125 volts in each step(unless timestep < 1).
       First we calculate dt_dv = CR/V with dt in cpu cycles to
       determine settling time step
    */
    uint64_t TimeStep;
    double dv = fabs(DCVoltage - voltage);

    // avoid divide by zero
    if (dv < 0.000001)
        dv = 0.000001;

    double dt_dv = get_cycles().instruction_cps() * current_time_constant / dv;
    TimeStep = (uint64_t) (0.125 * dt_dv);
    TimeStep = (TimeStep) ? TimeStep : 1;

    Dprintf(("%s dt_dv = %.2f TimeStep 0x%" PRINTF_INT64_T_MODIFIER "x now 0x%" PRINTF_INT64_T_MODIFIER "x\n", __FUNCTION__, dt_dv, TimeStep, get_cycles().get()));

    return TimeStep;
}

//------------------------------------------------------------------------
// updateStimuli
//
// drive all the stimuli connected to this node.

void Stimulus_Node::updateStimuli()
{
    stimulus *sptr = stimuli;

    while (sptr)
    {
        sptr->set_nodeVoltage(voltage);
        sptr = sptr->next;
    }
}

void Stimulus_Node::update()
{
    if (stimuli)
    {
        refresh();
        updateStimuli();
    }
}

void Stimulus_Node::set_nodeVoltage(double v)
{
    voltage = v;
    updateStimuli();
}

//------------------------------------------------------------------------
void Stimulus_Node::callback()
{
    if (verbose)
        callback_print();

    initial_voltage = voltage;
    double Time_Step;
    double expz;
    //
    // increase time step as capacitor charges more slowly as final
    // voltage is approached.
    //

    //
    // The following is an exact calculation, assuming no circuit
    // changes,  regardless of time step.
    //
    Time_Step = (get_cycles().get() - cap_start_cycle) /
                (get_cycles().instruction_cps() * current_time_constant);
    expz = exp(-Time_Step);
    voltage = DCVoltage - (DCVoltage - voltage) * expz;

    if (verbose)
        std::cout << "\tVoltage was " << initial_voltage << "V now "
                  << voltage << "V\n";

    if (fabs(DCVoltage - voltage) < minThreshold)
    {
        voltage = DCVoltage;
        if (future_cycle)
            get_cycles().clear_break(this);
        future_cycle = 0;
        if (verbose)
            std::cout << "\t" << name() <<
                      " Final voltage " << DCVoltage << " reached at "
                      << get_cycles().get() << " cycles\n";
        Dprintf(("%s DC Voltage %.2f reached at 0x%" PRINTF_INT64_T_MODIFIER "x cycles\n", name().c_str(), DCVoltage, get_cycles().get()));
    }
    else if(get_cycles().get() >= future_cycle) // got here via break
    {
        settlingTimeStep = calc_settlingTimeStep();
        cap_start_cycle = get_cycles().get();
        get_cycles().clear_break(this);
        future_cycle = cap_start_cycle + settlingTimeStep;
        get_cycles().set_break(future_cycle, this);
#ifdef DEBUG
        get_cycles().dump_breakpoints();
#endif
        if (verbose)
            std::cout << "\tBreak reached at " << cap_start_cycle <<
                      " cycles, next break set for "
                      << future_cycle << " delta=" << settlingTimeStep << '\n';
    }
    else	// updating value before break don't increase step size
    {
        cap_start_cycle = get_cycles().get();
        get_cycles().reassign_break(future_cycle,
                                    cap_start_cycle + settlingTimeStep, this);
        future_cycle = get_cycles().get() + settlingTimeStep;
        if (verbose)
            std::cout << "\tcallback called at " << cap_start_cycle <<
                      " cycles, next break set for " << future_cycle << " delta="
                      << settlingTimeStep << '\n';
    }

    updateStimuli();
}

//------------------------------------------------------------------------
void Stimulus_Node::callback_print()
{
    std::cout << "Node: " << name();
    TriggerObject::callback_print();
}

//------------------------------------------------------------------------
stimulus::stimulus(const char *cPname, double _Vth, double _Zth)
    : Value(cPname, "", nullptr), snode(nullptr), next(nullptr),
      bDrivingState(false), bDriving(false),
      Vth(_Vth), Zth(_Zth),
      Cth(0.0), // Farads
      nodeVoltage(0.0) // volts
{
}

void stimulus::new_name(const char *cPname)
{
    globalSymbolTable().removeSymbol(this);
    gpsimObject::new_name(cPname);
    globalSymbolTable().addSymbol(this);

    stimulus *psn = dynamic_cast<stimulus *>(globalSymbolTable().find(name()));
    if (psn)
    {
        if (psn == this)
            ; //cout << "Successfully added " << name() << " to symbol table\n";
        else
            std::cout << "Successfully added " << name() << " but it's not equal to this node\n";
    }
    else
        std::cout << "Failed to add " << name() << " to symbol table\n";

    /*
    const char *cPoldName = name().c_str();
    if(name_str.empty() && cPname != NULL && *cPname != 0) {
      // Assume never in symbol table.
      // Every named stimulus goes into the symbol table.
      gpsimObject::new_name(cPname);
      symbol_table.add_stimulus(this,bClearableSymbol);
      return;
    }
    if(symbol_table.Exist(cPoldName)) {
      // The symbol is in the symbol table. Since the
      // symbol table is ordered we need to let the
      // symbol table rename the object to maintain
      // ordering. Yuk.
      // Note that rename() will call stimulus::new_name()
      // after the symbol is removed. This recursive
      // call will then enter the branch that calls
      // gpsimObject::new_name(). The simulus with
      // its new name is added into the symbol table.
      symbol_table.rename(cPoldName,cPname);
    }
    else {
      gpsimObject::new_name(cPname);
    }
    */
}

void stimulus::new_name(std::string &rName)
{
    new_name(rName.c_str());
}

stimulus::~stimulus()
{
    if (snode)
        snode->detach_stimulus(this);

    globalSymbolTable().removeSymbol(this);
    //cout << "Removing " << name() << " from ST\n";
}

void stimulus::show()
{
    GetUserInterface().DisplayMessage(toString().c_str());
}

std::string stimulus::toString()
{
    std::ostringstream s;

    s << " stimulus ";
    if (snode)
        s << " attached to " << snode->name();


    s << '\n'
      << " Vth=" << get_Vth() << "V"
      << " Zth=" << get_Zth() << " ohms"
      << " Cth=" << get_Cth() << "F"
      << " nodeVoltage= " << get_nodeVoltage() << "V"
      << '\n'
      << " Driving=" << (getDriving()?"OUT":"IN")
      << " drivingState=" << getDrivingState()
      << " drivenState=" << getDrivenState()
      << " bitState=" << getBitChar();

    return s.str();
}

void stimulus::attach(Stimulus_Node *s)
{
    detach(snode);
    snode = s;
}

void stimulus::detach(Stimulus_Node *s)
{
    if (snode == s)
        snode = nullptr;
}

void stimulus::getThevenin(double &v, double &z, double &c)
{
    v = get_Vth();
    z = get_Zth();
    c = get_Cth();
}

//========================================================================
//
PinMonitor::PinMonitor()
{
}

PinMonitor::~PinMonitor()
{
    // Release all of the sinks:
    for (SignalSink *ssi : sinks)
    {
        //Dprintf(("release sink %p\n", ssi));
        fflush(stdout);
        (*ssi).release();
    }

    for (AnalogSink *asi : analogSinks)
    {
        (*asi).release();
    }
}

void PinMonitor::addSink(SignalSink *new_sink)
{
    if (new_sink)
    {
        //cout << "Adding sink: " << new_sink << endl;
        sinks.push_back(new_sink);
    }
}

void PinMonitor::removeSink(SignalSink *pSink)
{
    if (pSink)
    {
        sinks.remove(pSink);
    }
}

void PinMonitor::addSink(AnalogSink *new_sink)
{
    if (new_sink)
        analogSinks.push_back(new_sink);
}

void PinMonitor::removeSink(AnalogSink *pSink)
{
    if (pSink)
        analogSinks.remove(pSink);
}
//========================================================================


square_wave::square_wave(unsigned int p, unsigned int dc, unsigned int ph, const char *n)
{
    //cout << "creating sqw stimulus\n";

    if (n)
        new_name(n);
    else
    {
        char name_str[100];
        snprintf(name_str, sizeof(name_str), "s%d_square_wave", num_stimuli);
        num_stimuli++;
        new_name(name_str);
    }

    period = p;   // cycles
    duty   = dc;  // # of cycles over the period for which the sq wave is high
    phase  = ph;  // phase of the sq wave wrt the cycle counter
    time   = 0;   // simulation time
    snode = 0;
    next = 0;
}

double square_wave::get_Vth()
{
    uint64_t current_time = get_cycles().get();

    if (verbose & 1)
        std::cout << "Getting new state of the square wave.\n";

    if (((current_time + phase) % period) <= duty)
        return Vth;
    else
        return 0.0;
}


//========================================================================
//
// triangle_wave

triangle_wave::triangle_wave(unsigned int p, unsigned int dc, unsigned int ph, const char *n)
{
    //cout << "creating sqw stimulus\n";

    if (n)
        new_name(n);
    else
    {
        char name_str[100];
        snprintf(name_str, sizeof(name_str), "s%d_triangle_wave", num_stimuli);
        num_stimuli++;
        new_name(name_str);
    }

    if (p == 0)  //error
        p = 1;

    // copy the square wave stuff
    period = p;   // cycles
    duty   = dc;  // # of cycles over the period for which the sq wave is high
    phase  = ph;  // phase of the sq wave wrt the cycle counter
    time   = 0;   // simulation time
    snode  = 0;
    next   = 0;

    //cout << "duty cycle " << dc << " period " << p << " drive " << drive << '\n';

    // calculate the slope and the intercept for the two lines comprising
    // the triangle wave:

    if (duty)
        m1 = Vth / duty;
    else
        m1 = Vth / period;   // m1 will not be used if the duty cycle is zero

    b1 = 0;

    if (period != duty)
        m2 = Vth / (duty - period);
    else
        m2 = Vth;

    b2 = -m2 * period;

    //cout << "m1 = " << m1 << " b1 = " << b1 << '\n';
    //cout << "m2 = " << m2 << " b2 = " << b2 << '\n';
}

double triangle_wave::get_Vth()
{
    uint64_t current_time = get_cycles().get();

    //cout << "Getting new state of the triangle wave.\n";

    uint64_t t = (current_time + phase) % period;

    double ret_val;

    if (t <= duty)
        ret_val = b1 + m1 * t;
    else
        ret_val = b2 + m2 * t;

    //  cout << "Triangle wave: t = " << t << " value = " << ret_val << '\n';
    return ret_val;
}

//========================================================================
//
// Event

Event::Event()
    : current_state(0)
{
}

//========================================================================
//
void Event::callback()
{
    // If there's a node attached to this stimulus, then update it.
    if (snode)
        snode->update();

    // If the event is inactive.

    if (current_state == 0)
    {
        get_cycles().set_break_delta(1, this);
        current_state = 1;
    }
    else
    {
        current_state = 0;
    }
}

void source_stimulus::callback_print()
{
    std::cout << "stimulus " << name() << " CallBack ID " << CallBackID << '\n';
}


void source_stimulus::callback()
{
    std::cout << "shouldn't be called\n";
}

void source_stimulus::show()
{
    stimulus::show();
}

void source_stimulus::put_period(Value *pValue)
{
    if (pValue)
        pValue->get_as(period);
}

void source_stimulus::put_duty(Value *pValue)
{
    if (pValue)
        pValue->get_as(duty);
}

void source_stimulus::put_phase(Value *pValue)
{
    if (pValue)
        pValue->get_as(phase);
}

void source_stimulus::put_initial_state(Value *pValue)
{
    if (pValue)
        pValue->get_as(initial_state);
}

void source_stimulus::put_start_cycle(Value *pValue)
{
    if (pValue)
        pValue->get_as(start_cycle);
}

//========================================================================
//
IOPIN::IOPIN(const char *_name,
             double _Vth,
             double _Zth,
             double _ZthWeak,
             double _ZthFloating
            )

    : stimulus(_name, _Vth, _Zth),
      gui_name_updated(false),
      bDrivenState(false),
      cForcedDrivenState('Z'),
      m_monitor(nullptr),
      ZthWeak(_ZthWeak), ZthFloating(_ZthFloating),
      l2h_threshold(2.0),       // PICs are CMOS and use CMOS-like thresholds
      h2l_threshold(1.0),
      Vdrive_high(4.4),
      Vdrive_low(0.6),
      schmitt_level(false)
{
    if (verbose)
        std::cout << "IOPIN default constructor\n";
    is_analog = false;
}

void IOPIN::set_digital_threshold(double vdd)
{
    if (schmitt_level)
    {
        set_l2h_threshold(0.8 * vdd);
        set_h2l_threshold(0.2 * vdd);
    }
    else
    {
        set_l2h_threshold(vdd > 4.5 ? 2.0 : 0.25 * vdd + 0.8);
        set_h2l_threshold(vdd > 4.5 ? 0.8 : 0.15 * vdd);
    }
    Vdrive_high = vdd - 0.6;
    Vdrive_low = 0.6;
}

void IOPIN::setMonitor(PinMonitor *new_pinMonitor)
{
    if (m_monitor && new_pinMonitor)
        std::cout << "IOPIN already has a monitor!\n";
    else
        m_monitor = new_pinMonitor;
}

// True = set schmitt trigger levels, False = set TTL levels
void IOPIN::set_schmitt_level(bool _schmitt, double vdd)
{
    if (_schmitt != schmitt_level)
    {
        schmitt_level = _schmitt;
        set_digital_threshold(vdd);
    }
}

IOPIN::~IOPIN()
{
    if (m_monitor)
        ((PinModule *)m_monitor)->clrPin();
}

void IOPIN::get_as(char *return_str, int len)
{
    if (return_str)
    {
        if (get_direction() == DIR_OUTPUT)
            strncpy(return_str, IOPIN::getDrivingState() ? "1" : "0", len);
        else
            strncpy(return_str, IOPIN::getState() ? "1" : "0", len);
    }
}

void IOPIN::attach(Stimulus_Node *s)
{
    snode = s;
}

void IOPIN::show()
{
    stimulus::show();
}

/*
    This is required in this class so get_Vth(), get_Zth() and get_Cth()
    in this class are used to compute Thevenin voltage and Thevenin impedance
*/

void IOPIN::getThevenin(double &v, double &z, double &c)
{
    v = get_Vth();
    z = get_Zth();
    c = get_Cth();
}
//--------------------
// set_nodeVoltage()
//
//
void IOPIN::set_nodeVoltage(double new_nodeVoltage)
{
    if (verbose & 1)
        std::cout << name() << " set_nodeVoltage old=" << nodeVoltage << " new=" << new_nodeVoltage << '\n';

    nodeVoltage = new_nodeVoltage;

    if (nodeVoltage < h2l_threshold)
    {
        // The voltage is below the low threshold
        setDrivenState(false);
    }
    else if (nodeVoltage > l2h_threshold)
    {

        // The voltage is above the high threshold
        setDrivenState(true);

    }
    else
    {
        // The voltage is between the low and high thresholds,
        // so do nothing
    }

    //setDrivenState(getBitChar());
    if (m_monitor)
        m_monitor->set_nodeVoltage(nodeVoltage);
}

//------------------------------------------------------------
// putState - called by peripherals when they wish to
// drive an I/O pin to a new state.

void IOPIN::putState(bool new_state)
{
    if (new_state != bDrivingState)
    {
        bDrivingState = new_state;
        Vth = bDrivingState ? Vdrive_high : Vdrive_low;

        if (verbose & 1)
            std::cout << name() << " putState= " << (new_state ? "high\n" : "low\n");

        // If this pin is tied to a node, then update the node.
        // Note that when the node is updated, then the I/O port
        // (if there is one) holding this I/O pin will get updated.
        // If this pin is not tied to a node, then try to update
        // the I/O port directly.

        if (snode)
            snode->update();
    }
    if (m_monitor)
        m_monitor->putState(new_state ? '1' : '0');
}

void IOPIN::putState(double new_Vth)
{
    if (new_Vth != Vth)
    {
        Vth = new_Vth;

        if (Vth <= 0.3)
            bDrivingState = false;
        else
            bDrivingState = true;

        if (verbose & 1)
            std::cout << name() << " putState=" << new_Vth << '\n';

        // If this pin is tied to a node, then update the node.
        if (snode)
            snode->update();
    }
    if (m_monitor)
        m_monitor->putState(bDrivingState ? '1' : '0');
}

//------------------------------------------------------------
bool IOPIN::getState()
{
    return getDriving() ? getDrivingState() : getDrivenState();
}

void IOPIN::setDrivingState(bool new_state)
{
    bDrivingState = new_state;

    if (m_monitor)
        m_monitor->setDrivingState(bDrivingState ? '1' : '0');

    if (verbose & 1)
        std::cout << name() << " setDrivingState= " << (new_state ? "high\n" : "low\n");
}

void IOPIN::setDrivingState(char new3State)
{
    bDrivingState = (new3State == '1' || new3State == 'W');

    if (m_monitor)
        m_monitor->setDrivingState(new3State);
}

bool IOPIN::getDrivingState()
{
    return bDrivingState;
}

bool IOPIN::getDrivenState()
{
    return bDrivenState;
}

//------------------------------------------------------------------------
// setDrivenState
//
// An stimulus attached to this pin is driving us to a new state.
// This state will be recorded and propagate up to anything
// monitoring this pin.

void IOPIN::setDrivenState(bool new_state)
{
    bDrivenState = new_state;

    if (verbose & 1)
        std::cout << name() << " setDrivenState= " << (new_state ? "high\n" : "low\n");

    // Propagate the new state to those things monitoring this pin.
    // (note that the 3-state value is what's propagated).
    if (m_monitor && !is_analog)
    {
        m_monitor->setDrivenState(getBitChar());
        if (verbose & 16)
            std::cout << name() << " setting state of monitor to " << getBitChar() << '\n';
    }
}

//------------------------------------------------------------------------
// forceDrivenState() - allows the 'driven state' to be manipulated whenever
// there is no snode attached. The primary purpose of this is to allow the
// UI to toggle I/O pin states.
//
void IOPIN::forceDrivenState(char newForcedState)
{
    if (cForcedDrivenState != newForcedState)
    {
        cForcedDrivenState = newForcedState;

        bDrivenState = cForcedDrivenState == '1' || cForcedDrivenState == 'W';

        if (m_monitor)
        {
            m_monitor->setDrivenState(getBitChar());
            m_monitor->updateUI();
        }
    }
}

char IOPIN::getForcedDrivenState()
{
    return cForcedDrivenState;
}

void IOPIN::toggle()
{
    putState((bool) (getState() ^ true));
}

/*************************************
 *  int IOPIN::get_Vth()
 *
 * If this iopin has a stimulus attached to it then
 * the voltage will be dictated by the stimulus. Otherwise,
 * the voltage is determined by the state of the ioport register
 * that is inside the pic. For an input (like this), the pic code
 * that is being simulated can not change the state of the I/O pin.
 * However, the user has the ability to modify the state of
 * this register either by writing directly to it in the cli,
 * or by clicking in one of many places in the gui.
 */
double IOPIN::get_Vth()
{
    return Vth;
}

char IOPIN::getBitChar()
{
    if (!snode)
        return getForcedDrivenState();      // RCP - Changed to match IO_bi_directional
//  was  return 'Z';  // High impedance - unknown state.

    if (snode->get_nodeZth() > ZthFloating)
        return 'Z';

    if (snode->get_nodeZth() > ZthWeak)
        return getDrivenState() ? 'W' : 'w';

    return getDrivenState() ? '1' : '0';
}

void IOPIN::newGUIname(const char *s)
{
    if (s)
    {
        gui_name_updated = true;
        gui_name = s;
    }
}

// FIXME: Should not cast away constness
std::string &IOPIN::GUIname() const
{
    return (std::string &)gui_name;
}

//========================================================================
//
IO_bi_directional::IO_bi_directional(const char *_name,
                                     double _Vth,
                                     double _Zth,
                                     double _ZthWeak,
                                     double _ZthFloating,
                                     double _VthIn,
                                     double _ZthIn)
    : IOPIN(_name, _Vth, _Zth, _ZthWeak, _ZthFloating),
      ZthIn(_ZthIn), VthIn(_VthIn)
{
}

/*
    This is required in this class so get_Vth(), get_Zth() and get_Cth()
    in this class are used to compute Thevenin voltage and Thevenin impedance
*/

void IO_bi_directional::getThevenin(double &v, double &z, double &c)
{
    v = get_Vth();
    z = get_Zth();
    c = get_Cth();
}

void IO_bi_directional::set_nodeVoltage(double new_nodeVoltage)
{
    IOPIN::set_nodeVoltage(new_nodeVoltage);
}

double IO_bi_directional::get_Vth()
{
    if (getDriving())
        return getDrivingState() ? Vth : 0.0;

    //return getDrivingState() ? VthIn : 0;
    return VthIn;
}

double IO_bi_directional::get_Zth()
{
    return getDriving() ? Zth : ZthIn;
}

/*
   getBitChar() returns bit status as follows
     Input pin
	1> Pin considered floating,
	   return 'Z'
	2> Weak Impedance on pin,
	   return 'W" if high or 'w' if low
	3> Pin being driven externally
	   return '1' node voltage high '0' if low
     Output pin
	1> Node voltage opposite driven value
	   return 'X' if node voltage high or 'x' if inode voltage low
	2> Node voltage same as driven value
	   return '1' node voltage high '0' if low
*/

char IO_bi_directional::getBitChar()
{
    if (!snode && !getDriving())
        return getForcedDrivenState();

    if (snode)
    {
        if (!getDriving())		// input pin
        {
            if (snode->get_nodeZth() > ZthFloating)
                return 'Z';

            if (snode->get_nodeZth() > ZthWeak)
                return getDrivenState() ? 'W' : 'w';
        }
        else if (getDrivenState() != getDrivingState())
            return getDrivenState() ? 'X' : 'x';
    }

    return getDrivenState() ? '1' : '0';
}


//---------------
//::update_direction(unsigned int new_direction)
//
//  This is called when a new value is written to the tris register
// with which this bi-direction pin is associated.

void IO_bi_directional::update_direction(unsigned int new_direction, bool refresh)
{
    setDriving(new_direction ? true : false);

    // If this pin is not associated with an IO Port, but it's tied
    // to a stimulus, then we need to update the stimulus.
    if (refresh && snode)
        snode->update();
}

void IO_bi_directional::putState(bool new_state)
{
    IOPIN::putState(new_state);
}

void IO_bi_directional::putState(double new_Vth)
{
    VthIn = new_Vth;
    IOPIN::putState(new_Vth);
}

IO_bi_directional_pu::IO_bi_directional_pu(const char *_name,
        double _Vth,
        double _Zth,
        double _ZthWeak,
        double _ZthFloating,
        double _VthIn,
        double _ZthIn,
        double _Zpullup)
    : IO_bi_directional(_name, _Vth, _Zth, _ZthWeak,
                        _ZthFloating, _VthIn, _ZthIn),
      Zpullup(_Zpullup)
{
    Vpullup = Vth;
    bPullUp = false;
}

IO_bi_directional_pu::~IO_bi_directional_pu()
{
}

/*
    This is required in this class so get_Vth(), get_Zth() and get_Cth()
    in this class are used to compute Thevenin voltage and Thevenin impedance
*/

void IO_bi_directional_pu::getThevenin(double &v, double &z, double &c)
{
    v = get_Vth();
    z = get_Zth();
    c = get_Cth();
}

void IO_bi_directional_pu::set_is_analog(bool flag)
{
    if (is_analog != flag)
    {
        is_analog = flag;

        if (snode)
            snode->update();
        else if (!getDriving())
            setDrivenState(bPullUp && ! is_analog);
    }
}

void IO_bi_directional_pu::update_pullup(char new_state, bool refresh)
{
    bool bNewPullupState = new_state == '1' || new_state == 'W';
    if (bPullUp != bNewPullupState)
    {
        bPullUp = bNewPullupState;
        if (refresh)
        {
            // If there is a node attached to the pin, then we already
            // know the driven state. If there is no node attached and
            // this pin is configured as an input, then let the drivenState
            // be the same as the pullup state.
            if (snode)
                snode->update();
            else if (!getDriving())
                setDrivenState(bPullUp && ! is_analog);
        }
    }
}

double IO_bi_directional_pu::get_Zth()
{
    return getDriving() ? Zth : ((bPullUp && ! is_analog) ? Zpullup : ZthIn);
}

double IO_bi_directional_pu::get_Vth()
{
    /**/
    if (verbose & 1)
        std::cout << " " << name() << " get_Vth PU "
                  << " Direction=" << (getDriving()?"OUT":"IN")
                  << " DrivingState=" << getDrivingState()
                  << " bDrivenState=" << bDrivenState
                  << " Vth=" << Vth
                  << " VthIn=" << VthIn
                  << " bPullUp=" << bPullUp
                  << " is_analog=" << is_analog << '\n';
    /**/

    // If the pin is configured as an output, then the driving voltage
    // depends on the pin state. If the pin is an input, and the pullup resistor
    // is enabled, then the pull-up resistor will 'drive' the output. The
    // open circuit voltage in this case will be Vth (the thevenin voltage,
    // which is assigned to be same as the processor's supply voltage).

    if (getDriving())
        return getDrivingState() ? Vth : 0.0;
    else
        return (bPullUp && ! is_analog) ? Vpullup : VthIn;
}

/*
   getBitChar() returns bit status as follows
     Input pin
	1> Pin considered floating,
	   return 'Z'
	2> Weak Impedance on pin,
	   return 'W" if high or 'w' if low
	3> Pin being driven externally
	   return '1' node voltage high '0' if low
     Output pin
	1> Node voltage opposite driven value
	   return 'X' if node voltage high or 'x' if inode voltage low
	2> Node voltage same as driven value
	   return '1' node voltage high '0' if low
*/

char IO_bi_directional_pu::getBitChar()
{
    if (!snode && !getDriving() )
    {
        char cForced = getForcedDrivenState();
        return (cForced == 'Z' && bPullUp) ? 'W' : cForced;
    }

    if (snode)
    {
        if (!getDriving())		// input pin
        {
            if (snode->get_nodeZth() > ZthFloating)
                return 'Z';

            if (snode->get_nodeZth() > ZthWeak)
                return getDrivenState() ? 'W' : 'w';
        }
        else if (getDrivenState() != getDrivingState())
            return getDrivenState() ? 'X' : 'x';
    }

    return getDrivenState() ? '1' : '0';
}
/*
   This class adds open_collector(drain) pin functionality.
   when OpenDrain is true(default) the pin does not source current then output is high
   ODCON register sets or clears the OpenDrain flag
*/
IO_open_collector::IO_open_collector(const char *_name)
    : IO_bi_directional_pu(_name), OpenDrain(true)
{
}

/*
    This is required in this class so get_Vth(), get_Zth() and get_Cth()
    in this class are used to compute Thevenin voltage and Thevenin impedance
*/

void IO_open_collector::getThevenin(double &v, double &z, double &c)
{
    v = get_Vth();
    z = get_Zth();
    c = get_Cth();
}

double IO_open_collector::get_Vth()
{
    /**/
    if (verbose & 1)
        std::cout << name() << " get_Vth OC"
                  << " Direction=" << (getDriving()?"OUT":"IN")
                  << " DrivingState=" << getDrivingState()
                  << " bDrivenState=" << bDrivenState
                  << " Vth=" << Vth
                  << " VthIn=" << VthIn
                  << " Vpullup=" << Vpullup
                  << " bPullUp=" << bPullUp << '\n';
    /**/

    if (getDriving())
        return getDrivingState() ? Vth : 0.0;

    return bPullUp ? Vpullup : VthIn;
}

double IO_open_collector::get_Zth()
{
    float Z;
    if (OpenDrain)
    {
        if (getDriving() && !getDrivingState())
            Z = Zth;
        else
            Z = bPullUp ? Zpullup : ZthIn;
    }
    else
    {
        if (getDriving())
        {
            Z = Zth;
        }
        else
            Z =  bPullUp ? Zpullup : ZthIn;
    }
    if (verbose)
    {
        std::cout << name() << " get_Zth OC"
                  << " Direction=" << (getDriving()?"OUT":"IN")
                  << " DrivingState=" << getDrivingState()
                  << " bDrivenState=" << bDrivenState
                  << " OpenDrain=" << OpenDrain
                  << " Zth=" << Zth
                  << " ZthIn=" << ZthIn
                  << " Z=" << Z
                  << " Zpullup=" << Zpullup
                  << " bPullUp=" << bPullUp << '\n';
    }
    return Z;
}

char IO_open_collector::getBitChar()
{
    if (!snode && !getDriving())
    {
        char cForced = getForcedDrivenState();
        return (cForced == 'Z' && bPullUp) ? 'W' : cForced;
    }

    if (snode)
    {

        if (snode->get_nodeZth() > ZthFloating)
            return bPullUp ? 'W' : 'Z';

        if (getDriving() && getDrivenState() && !getDrivingState())
            return 'X';

        if (snode->get_nodeZth() > ZthWeak)
            return getDrivenState() ? 'W' : 'w';
        else
            return getDrivenState() ? '1' : '0';
    }

    return getDrivingState() ? 'W' : '0';
}


//========================================================================

//========================================================================
//
// ValueStimulus
//
// A Value stimulus is a stream of data that can change values at
// arbitrary times. An array called 'transition_cycles' stores the times
// and an array 'values' stores the values.
//   When first initialized, the stimulus is driven to its initial state.
// A break point is set on the cycle counter for the next cpu cycle that
// the stimulus is expected to change values. When the break occurs,
// the current state is updated to the next value  and then a break is set
// for the next expect change. This cycle occurs until all of the values
// have been generated. When the end is reached, the asynchronous stimulus
// will restart from the beginning. The member variable 'period' describes
// the magnitude of the rollover (if it's zero then there is no rollover).
//

ValueStimulus::ValueStimulus(const char *n)
    : source_stimulus()
{
    initial.time = 0;
    initial.v = 0;
    current = 0;
    next_sample.time = 0;
    next_sample.v = nullptr;

    if (n)
    {
        new_name(n);
    }
    else
    {
        char name_str[100];
        snprintf(name_str, sizeof(name_str), "s%d_asynchronous_stimulus", num_stimuli);
        num_stimuli++;
        new_name(name_str);
    }
}

void ValueStimulus::get_as(char *return_str, int len)
{
    if (return_str)
    {
        double  d = get_Vth();
        snprintf(return_str, len, "%.2fV", d);
    }
}

ValueStimulus::~ValueStimulus()
{
    delete initial.v;
    delete current;

    for (sample_iterator = samples.begin();
            sample_iterator != samples.end();
            ++sample_iterator)
    {
        delete (*sample_iterator).v;
    }
}

void ValueStimulus::show()
{
    // print the electrical stuff
    stimulus::show();

    std::cout << "\n  states = " << samples.size() << '\n';

    std::list<ValueStimulusData>::iterator si;

    for (si = samples.begin(); si != samples.end(); ++si)
    {
        //double d;
        //(*si).v->get_as(d);
        std::cout << "    t=" << std::dec << (*si).time
                  <<  ",v=" << (*si).v->toString()
                  << '\n';
    }

    if (initial.v)
        std::cout << "  initial=" << initial.v->toString() << '\n';

    std::cout
            << "  period=" << period << '\n'
            << "  start_cycle=" << start_cycle << '\n'
            << "  Next break cycle=" << future_cycle << '\n';
}

void ValueStimulus::callback()
{
    uint64_t current_cycle = future_cycle;

    current = next_sample.v;

    if (verbose & 1)
        std::cout << "asynchro cycle " << current_cycle << "  state " << current->toString() << '\n';

    // If there's a node attached to this stimulus, then update it.
    if (snode)
        snode->update();

    ValueStimulusData *n = getNextSample();

    if (n)
    {
        next_sample = *n;

        if (verbose & 1)
        {
            std::cout << "  current_sample (" << next_sample.time << ","
                      << next_sample.v->toString() << ")\n";
            std::cout << " start cycle " << start_cycle << '\n';
        }

        // get the cycle when the data will change next

        future_cycle = next_sample.time + start_cycle;

        if (future_cycle <= current_cycle)
        {

            // There's an error in the data. Set a break on the next simulation cycle
            // and see if it can be resolved.

            future_cycle = current_cycle + 1;
        }

        get_cycles().set_break(future_cycle, this);
    }
    else
        future_cycle = 0;

    if (verbose & 1)
        std::cout << "  next transition = " << future_cycle << '\n';
}

void ValueStimulus::put_initial_state(Value *pValue)
{
    if (pValue && !initial.v)
    {
        initial.time = 0;
        initial.v = pValue->copy();
    }
}

void ValueStimulus::put_data(ValueStimulusData &data_point)
{
    ValueStimulusData *sample = new ValueStimulusData;
    sample->time = data_point.time;
    sample->v = data_point.v;
    samples.push_back(*sample);
}

double ValueStimulus::get_Vth()
{
    double v = initial_state;
    if (current)
    {
        try
        {
            current->get_as(v);
            if (digital && v > 0.0)
                v = 5.0;
        }

        catch (Error &err)
        {
            std::cout << "Warning stimulus: " << name() << " failed on: " << err.what() << '\n';
        }
    }
    return v;
}

void ValueStimulus::start()
{
    if (verbose & 1)
        std::cout << "Starting asynchronous stimulus\n";

    if (period)
    {
        // Create a data point for the rollover condition.
        // If an initial value was supplied when the stimulus was created, then
        // that's what we'll use for the rollover. Otherwise, we'll create
        // a rollover based on 'initial_state' (which should be a default value).

        ValueStimulusData vSample;
        vSample.time = period;
        vSample.v = initial.v ? initial.v : new Float(initial_state);
        put_data(vSample);
    }

    sample_iterator = samples.begin();

    if (sample_iterator != samples.end())
    {
        if (digital)
            initial_state = (initial_state > 0.0) ? Vth : 0.0;

        current       = initial.v;
        next_sample   = *sample_iterator;
        future_cycle  = next_sample.time + start_cycle;

        get_cycles().set_break(future_cycle, this);

    }

    if (verbose & 1)
        std::cout << "asy should've been started\n";
}

ValueStimulusData *ValueStimulus::getNextSample()
{
    ++sample_iterator;

    if (sample_iterator == samples.end())
    {
        // We've gone through all of the data. Now let's try to start over

        sample_iterator = samples.begin();

        // If the period is zero then we don't want to
        // regenerate the data stream.

        if (period == 0)
            return nullptr;

        start_cycle += period;

        if (verbose & 1)
        {
            std::cout << "  asynchronous stimulus rolled over\n"
                      << "   next start_cycle " << start_cycle << "  period " << period << '\n';
        }
    }

    return &(*sample_iterator);
}

//------------------------------------------------------------------------
AttributeStimulus::AttributeStimulus(const char *n)
    : ValueStimulus(n), attr(0)
{
}

/*
AttributeStimulus::~AttributeStimulus()
{
  ValueStimulus::~ValueStimulus();

}
*/

void AttributeStimulus::show()
{
    if (attr)
        std::cout << "\nDriving Attribute:" << attr->name() << '\n';
    ValueStimulus::show();
}

void AttributeStimulus::callback()
{
    uint64_t current_cycle = future_cycle;

    current = next_sample.v;

    if (verbose & 1)
        std::cout << "asynchro cycle " << current_cycle << "  state " << current->toString() << '\n';

    // If there's a node attached to this stimulus, then update it.
    if (attr)
        attr->set(current);

    ValueStimulusData *n = getNextSample();

    if (n)
    {
        next_sample = *n;

        if (verbose & 1)
        {
            std::cout << "  current_sample (" << next_sample.time << ","
                      << next_sample.v->toString() << ")\n";
            std::cout << " start cycle " << start_cycle << '\n';
        }

        // get the cycle when the data will change next

        future_cycle = next_sample.time + start_cycle;


        if (future_cycle <= current_cycle)
        {

            // There's an error in the data. Set a break on the next simulation cycle
            // and see if it can be resolved.

            future_cycle = current_cycle + 1;
        }

        get_cycles().set_break(future_cycle, this);
    }
    else
        future_cycle = 0;

    if (verbose & 1)
        std::cout <<"  next transition = " << future_cycle << '\n';
}

void AttributeStimulus::setClientAttribute(Value *v)
{
    if (attr)
        std::cout << "overwriting target attribute in AttributeStimulus\n";

    attr = v;

    if ((bool)verbose && v)
        std::cout << " attached " << name() << " to attribute: " << v->name() << '\n';
}

//========================================================================
//
// helper functions follow here


void stimuli_attach(gpsimObject *pNode, gpsimObjectList_t *pPinList)
{
    if (!pNode || !pPinList)
        return;

    if (verbose)
        std::cout << __FUNCTION__ << " pNode " << pNode->name() << '\n';

    gpsimObjectList_t :: iterator si = pPinList->begin();

    Stimulus_Node *psn = dynamic_cast<Stimulus_Node *>(pNode);
    if (psn)
    {

        //cout << __FUNCTION__ << " pNode " << pNode->name() << " is a Stimulus_Node\n";

        while (si != pPinList->end())
        {
            stimulus *ps = dynamic_cast<stimulus *> (*si);
            if (ps)
                psn->attach_stimulus(ps);
            ++si;
        }

        psn->update();

        return;
    }

    AttributeStimulus *ast = dynamic_cast<AttributeStimulus *>(pNode);
    if (ast)
    {
        Value *v = dynamic_cast<Value *>(*si);
        if (v)
            ast->setClientAttribute(v);

        if (verbose)
        {
            std::cout << __FUNCTION__ << " pNode " << pNode->name() << " is an attribute stimulus\n";
            if (v)
                std::cout << __FUNCTION__ << " connecting " << v->name() << '\n';
        }
    }
}

void DATA_SERVER::attach_data(DATA_RECEIVER *pt_rcv)
{
    DATA_RECEIVER *spt = data_rcv;
    bool searching = true;
    Dprintf(("DATA_SERVER 0x%x spt=%p this=%p\n", source_code, spt, this));
    if (data_rcv)
    {
	while(searching)
	{
	    if (pt_rcv == spt)  // Receiver already attached
	    {
	        Dprintf(("DATA_SERVER dup %p %s\n", pt_rcv, pt_rcv->name().c_str()));
		return;
	    }

	    if (spt->next == 0)
            {
	        Dprintf(("DATA_SERVER attach %p %s\n", pt_rcv, pt_rcv->name().c_str()));
		spt->next = pt_rcv;
		searching = false;
	     }
	     spt = spt->next;
	}
    }
    else	// first entry to attach
    {
	data_rcv = pt_rcv;
	return;
    }
    spt->next = nullptr;

}
void DATA_SERVER::detach_data(DATA_RECEIVER *pt_rcv)
{
    DATA_RECEIVER *spt = data_rcv;

    if (!pt_rcv || !spt)	// Cannot remove null receiver or list empty
	return;

    Dprintf(("DATA_SERVER 0x%x spt=%p %s this=%p\n", source_code, spt, pt_rcv->name().c_str(), this));
    if (pt_rcv == spt)
    {
	// This is the first entry in the list
	//data_rcv = pt_rcv;
	data_rcv = spt->next;
	return;
    }
    else
    {
	do
	{
	    if (pt_rcv == spt->next) // got match
	    {
		spt->next = pt_rcv->next;
		return;
	    }
	    spt = spt->next;
	} while(spt);
    }

}
void DATA_SERVER::send_data(int v1, int v2)
{
    DATA_RECEIVER *spt = data_rcv;

    v2 |= source_code;	// Tag server source

    while(spt)
    {
	spt->rcv_data(v1, v2);
	spt = spt->next;
    }
}
