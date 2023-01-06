/*
   Copyright (C) 1999 T. Scott Dattalo

This file is part of gpsim.

gpsim is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

gpsim is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with gpsim; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#include <iostream>
#include <list>
#include <string>

#include "command.h"
#include "misc.h"
#include "cmd_stimulus.h"
#include "../src/errors.h"
#include "../src/gpsim_object.h"
#include "../src/stimuli.h"
#include "../src/symbol.h"
#include "../src/ui.h"
#include "../src/value.h"

static ValueStimulus *last_stimulus = nullptr;
cmd_stimulus c_stimulus;

#define ASYNCHRONOUS_STIMULUS    1
#define SYNCHRONOUS_STIMULUS     2

#define  STIM_NOTHING          0
#define  STIM_PERIOD          (1 << 0)
#define  STIM_PHASE           (1 << 1)
#define  STIM_HIGH_TIME       (1 << 2)
#define  STIM_INITIAL_STATE   (1 << 3)
#define  STIM_START_CYCLE     (1 << 4)
#define  STIM_DATA            (1 << 5)

#define  STIM_ASY             (1 << 7)
#define  STIM_SQW             (1 << 8)
#define  STIM_NAME            (1 << 9)
#define  STIM_TRI             (1 << 10)
#define  STIM_ATTRIBUTE       (1 << 11)
#define  STIM_ANALOG          (1 << 12)
#define  STIM_DIGITAL         (1 << 13)
#define  STIM_DUMP            (1 << 14)

const unsigned int
SQW_OPTIONS = STIM_SQW | STIM_PERIOD | STIM_PHASE | STIM_HIGH_TIME | STIM_START_CYCLE;
const unsigned int
ASY_OPTIONS = STIM_ASY | STIM_PERIOD | STIM_PHASE | STIM_HIGH_TIME | STIM_START_CYCLE | STIM_DATA;
const unsigned int
TRI_OPTIONS = STIM_TRI | STIM_PERIOD | STIM_PHASE | STIM_HIGH_TIME | STIM_START_CYCLE;
const unsigned int
ATTR_OPTIONS = STIM_ATTRIBUTE | STIM_PERIOD | STIM_PHASE | STIM_HIGH_TIME | STIM_START_CYCLE | STIM_DATA;

static cmd_options cmd_stimulus_options[] =
{
  {"asy",                   STIM_ASY,           OPT_TT_SUBTYPE},
  {"asynchronous_stimulus", STIM_ASY,           OPT_TT_SUBTYPE},
  {"attr",                  STIM_ATTRIBUTE,     OPT_TT_SUBTYPE},
  {"attribute_stimulus",    STIM_ATTRIBUTE,     OPT_TT_SUBTYPE},
  {"period",                STIM_PERIOD,        OPT_TT_NUMERIC},
  {"phase",                 STIM_PHASE,         OPT_TT_NUMERIC},
  {"high_time",             STIM_HIGH_TIME,     OPT_TT_NUMERIC},
  {"initial_state",         STIM_INITIAL_STATE, OPT_TT_NUMERIC},
  {"start_cycle",           STIM_START_CYCLE,   OPT_TT_NUMERIC},
  {"start",                 STIM_START_CYCLE,   OPT_TT_NUMERIC},
  {"name",                  STIM_NAME,          OPT_TT_STRING},
  {"digital",               STIM_DIGITAL,       OPT_TT_BITFLAG},
  {"analog",                STIM_ANALOG,        OPT_TT_BITFLAG},
  {"d",                     STIM_DUMP,          OPT_TT_BITFLAG},
  {"dump",                  STIM_DUMP,          OPT_TT_BITFLAG},
  {"sqw",                   STIM_SQW,           OPT_TT_SUBTYPE},
  {"square_wave",           STIM_SQW,           OPT_TT_SUBTYPE},
  {"tri",                   STIM_TRI,           OPT_TT_SUBTYPE},
  {"triangle_wave",         STIM_TRI,           OPT_TT_SUBTYPE},

 { 0,0,0}
};


cmd_stimulus::cmd_stimulus()
    : command("stimulus","stim"),
      valid_options(0), options_entered(0), have_data(0)
{
    brief_doc = "Create a stimulus";

    long_doc = "\nstimulus [[type] options]\n"
		     "\tstimulus will create a signal that can be tied to a node or an\n"
		     "\attribute. Note that in most cases it is easier to create a\n"
		     "\tstimulus file then to type this by hand.\n"
		     "\n"
		     "\t  Supported stimuli:\n"
		     "\n"
		     //"\tsquare_wave | sqw  [period p] [high_time h] [phase ph] [initial_state i]\n"
		     //"\t  port port_name bit_pos end\n"
		     //"\t\t  creates a square wave with a period of \"p\" cpu cycles.\n"
		     //"\t\t  If the high time is specified then that's the number of cycles\n"
		     //"\t\t  the square wave will be high.\n"
		     //"\t\t  The phase is with respect to the cpu's cycle counter.\n"
		     //"\t\t  The \"port_name\" and \"bit_pos\" describe where the stimulus\n"
		     //"\t\t  will be attached.\n"
		     "\tasynchronous_stimulus | asy  [period p] [phase ph]  [initial_state i]\n"
		     "\t  { c0,e0 [,c1, e1, c2, e2, ... ,cn,en] } [name stim_name] end\n"
		     "\t\t  creates an asynchronous square wave with a period of \"p\" cpu\n"
		     "\t\t  cycles.  The phase is with respect to the cpu's cycle counter.\n"
		     "\t\t  The data is specified as a pair of expressions. The first expression\n"
		     "\t\t  is for the cycle time and the second is the data. "
		     "\n"
		     "\texamples:\n"
		     "\n"
		     //"\t  stimulus sqw period 200 high_time 20 phase 60 port portb 0 end\n"
		     //"\t  create a square wave stimulus that repeats every 200 cpu cycles,\n"
		     //"\t  is high for 20 cpu cycles (and low for 200-20=180 cycles). The\n"
		     //"\t  first rising edge will occur at cycle\n"
		     //"\t  60, the second at 260, . . . Bit 0 of portb will receive the stimulus.\n"

		     "\t  # define a stimulus to generate two pulses every 1000 cycles\n"
		     "\t  \n"
		     "\t  stimulus asynchronous_stimulus \n"
		     "\t  \n"
		     "\t  # The initial state AND the state the stimulus is when\n"
		     "\t  # it rolls over\n"
		     "\t  \n"
		     "\t  initial_state 0\n"
		     "\t  start_cycle 0\n"
		     "\t  \n"
		     "\t  # the asynchronous stimulus will roll over in 'period'\n"
		     "\t  # cycles. Delete this line if you don't want a roll over.\n"
		     "\t  \n"
		     "\t  period 1000\n"
		     "\t  \n"
		     "\t  { 100, 1,\n"
		     "\t  200, 0,\n"
		     "\t  300, 1,\n"
		     "\t  400, 0\n"
		     "\t  }\n"
		     "\t  \n"
		     "\t  # Give the stimulus a name:\n"
		     "\t  \n"
		     "\t  name two_pulse_repeat\n"
		     "\t  \n"
		     "\t  end\n"
		     "\n";

    op = cmd_stimulus_options;
}

static std::string table_name;

void dumpStimulus(const SymbolEntry_t &sym)
{
    stimulus *ps = dynamic_cast<stimulus *>(sym.second);

    if (ps) {
        std::cout << table_name << '.' << ps->name();
        ps->show();
        std::cout << '\n';
    }
}

void dumpStimuli(const SymbolTableEntry_t &st)
{
    std::cout << " Symbol Table: " << st.first << '\n';
    table_name = st.first;
    (st.second)->ForEachSymbolTable(dumpStimulus);
}

void cmd_stimulus::stimulus()
{
    std::cout << "\nSymbol table\n";
    globalSymbolTable().ForEachModule(dumpStimuli);
}

//------------------------------------------------------------------
// stimulus(int bit_flag)
//
// For the bit_flags of SQW, ASY, TRI:
//   A new stimulus is dynamically created and a pointer to it is
// is assigned to 'last_stimulus'. The last_stimulus also acts like
// a flag. If it is non-null then a stimulus is in the process of
// being created. When the stimulus 'end' option is specified at the
// cli, then 'last_stimulus' is set to NULL. Note the memory for
// used by the last stimulus is created here, but destroyed by the
// stimulus code in ../src/stimuli.cc .
//
void cmd_stimulus::stimulus(int bit_flag)
{
    switch (bit_flag) {
        case STIM_SQW:
            if (verbose)
                std::cout << "creating sqw stimulus\n";

            if (!last_stimulus) {
                //create_stimulus(NEW_SQW,stim_name);
                valid_options = SQW_OPTIONS;
                options_entered = STIM_SQW;
                //last_stimulus = new square_wave;
            } else
                std::cout << "warning: ignoring sqw stim creation";

            break;

        case STIM_ASY:
            if (verbose)
                std::cout << "creating asy stimulus\n";

            if (!last_stimulus) {
                //create_stimulus(NEW_ASY,stim_name);
                last_stimulus = new ValueStimulus;
                valid_options = ASY_OPTIONS;
                options_entered = STIM_ASY;
            } else
                std::cout << "warning: ignoring asy stim creation";
            break;

        case STIM_ATTRIBUTE:
            if (verbose)
                std::cout << "creating asy stimulus\n";

            if (!last_stimulus) {
                last_stimulus = new AttributeStimulus;
                valid_options = ATTR_OPTIONS;
                options_entered = STIM_ATTRIBUTE;
            } else
                std::cout << "warning: ignoring asy stim creation";
            break;

        case STIM_TRI:
            if (verbose)
                std::cout << "creating tri stimulus\n";

            if (!last_stimulus) {
                //create_stimulus(NEW_TRI,stim_name);
                //last_stimulus = new triangle_wave;
                valid_options = TRI_OPTIONS;
                options_entered = STIM_TRI;
            } else
                std::cout << "warning: ignoring tri stim creation";
            break;

        case STIM_DUMP:
            stimulus();      // Display the list of stimuli.
            return;

        case STIM_DIGITAL:
            if (last_stimulus)
                last_stimulus->set_digital();
            return;

        case STIM_ANALOG:
            if (last_stimulus)
                last_stimulus->set_analog();
            return;

        default:
            std::cout << " Invalid stimulus option\n";
            return;
    }

}


void cmd_stimulus::stimulus(cmd_options_expr *coe)
{
  /*
  double dvalue = 0.0;
  if(coe->expr)
    dvalue = evaluate(coe->expr);

  int value = (int) dvalue;
  */

    if (!coe || !coe->expr)
        return;

    Value *value = toValue(coe->expr);

    switch (coe->co->value) {
        case STIM_PHASE:
            if (verbose)
                std::cout << "stimulus command got the phase " << value << '\n';

            if (last_stimulus)
                last_stimulus->put_phase(value);

            break;

        case STIM_PERIOD:
            if (verbose)
                std::cout << "stimulus command got the period " << value << '\n';

            if (last_stimulus)
                last_stimulus->put_period(value);

            break;

        case STIM_HIGH_TIME:
            if (verbose)
                std::cout << "stimulus command got the high_time " << value << '\n';

            if (last_stimulus)
                last_stimulus->put_duty(value);

            break;

        case STIM_INITIAL_STATE:
            if (verbose)
                std::cout << "stimulus command got the initial_state " << value << '\n';

            if (last_stimulus)
                last_stimulus->put_initial_state(value);

            break;

        case STIM_START_CYCLE:
            if (verbose)
                std::cout << "stimulus command got the start_cycle " << value << '\n';

            if (last_stimulus)
                last_stimulus->put_start_cycle(value);

            break;

        default:
            std::cout << " Invalid stimulus option\n";
            return;
    }

    options_entered |= coe->co->value;
    delete coe->expr;
    delete value;
}


void cmd_stimulus::stimulus(cmd_options_str *cos)
{
    if (!last_stimulus) {
        std::cout << "warning: Ignoring stimulus (string) option because there's no stimulus defined.\n";
        return;
    }

    switch (cos->co->value) {
        case STIM_NAME:
            if (verbose)
                std::cout << "stimulus command got the name " << cos->str << '\n';

            last_stimulus->new_name(cos->str);

            break;
    }

    options_entered |= cos->co->value;
}

void cmd_stimulus::stimulus(ExprList_t *eList)
{
    ExprList_itor ei;

    ValueStimulusData sample;
    sample.time = 0;
    sample.v = 0;

    if (last_stimulus) {
        bool bHaveSample = false;
        for (ei = eList->begin(); ei != eList->end(); ++ei) {
            try {
                Value *v = (*ei)->evaluate();

                if (!bHaveSample) {
                    v->get(sample.time);
                    delete v;
                    bHaveSample = true;
                } else {
                    sample.v = v;
                    last_stimulus->put_data(sample);
                    bHaveSample = false;
                    have_data = 1;
                }
            }
            catch (const Error &err) {
                std::cout << "ERROR:" << err.what() << '\n';
            }
        }
    }

    eList->clear();
    delete eList;
}

//-----------------
// end()
// All of the stimulus' options have been entered. Now it's time
// to create the stimulus.

void cmd_stimulus::end()
{
    if (!last_stimulus) {
        std::cout << "warning: Ignoring stimulus (string) option because there's no stimulus defined.";
        return;
    }

    switch (options_entered & (STIM_SQW | STIM_TRI | STIM_ASY | STIM_ATTRIBUTE)) {
        case STIM_SQW:
            if (verbose)
                std::cout << "created sqw stimulus\n";
            break;

        case STIM_ASY:
            if (verbose)
                std::cout << "created asy stimulus\n";
            last_stimulus->start();
            break;

        case STIM_ATTRIBUTE:
            if (verbose)
                std::cout << "created attribute stimulus\n";
            last_stimulus->start();
            break;

        case STIM_TRI:
            if (verbose)
                std::cout << "creating tri stimulus\n";
            break;
    }

    last_stimulus = 0;
}
