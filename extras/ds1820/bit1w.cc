/*  Copyright (C) 2012 Eduard Timotei Budulea
    Copyright (C) 2013 Roy R. Rankin

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "config.h"
#include "bit1w.h"

bool debug = false;

LowLevel1W::LowLevel1W(const char *name, const char *desc):
    Module(name, desc), cicluReper(0), lastValue(true), 
    lastTimeout(false),
    state(&LowLevel1W::idle), ignoreCallback(false), bit_break(0)
{
    pin = new Pin1W("pin", *this);
    addSymbol(pin);
    create_pkg(1);
    assign_pin(1, pin);
    pin->putState(false);
    pin->update_direction(IOPIN::DIR_INPUT, true);
    change(true);
}
LowLevel1W::~LowLevel1W()
{
    removeSymbol(pin);
}

void LowLevel1W::callback() {
    change(false);
}

void LowLevel1W::change(bool pinChange) {
    if (ignoreCallback) return;
    guint64  ciclu = get_cycles().get();
    bool bitValue = false;
    switch(pin->getBitChar()) {
    case 'Z':
    case '1':
    case 'x':
    case 'W':
        bitValue = true;
    }
    bool isTimeout = ciclu >= cicluReper;


    if ((lastValue != bitValue || lastTimeout != isTimeout) && debug) {
        cout << name() <<" +++change state: line = " << bitValue << ", timeout = " << isTimeout << "; time = " << hex << ciclu << ", reper = " << cicluReper << endl;
    }
    lastValue = bitValue;
    lastTimeout = isTimeout;
    ignoreCallback = true;
    (this->*state)(bitValue, isTimeout);
    ignoreCallback = false;
    if (cicluReper > ciclu) {
        if (!pinChange && bit_break >= ciclu) {
            get_cycles().clear_break(bit_break);
        }
	if(cicluReper != bit_break)
            get_cycles().set_break(cicluReper, this);
	if(debug)
	    printf("%s now %" PRINTF_GINT64_MODIFIER "x next break  %" PRINTF_GINT64_MODIFIER "x last break %" PRINTF_GINT64_MODIFIER "x delta(usec) %.1f\n", name().c_str(), ciclu, cicluReper, bit_break, (cicluReper-ciclu)*4./20.);
	bit_break = cicluReper;
    }
}

void LowLevel1W::idle(bool input, bool isTimeout) {
    if(debug && !isTimeout)
      cout <<name() << " idle input="<<input <<" timout="<<isTimeout<<endl;
    if (input) return;
    switch(gotBitStart()) {
    case WRITE1:
	if(verbose) cout << name() << " ===write1" << endl;
        state = &LowLevel1W::inWritting1;
        cicluReper = get_cycles().get(0.000045);
        return;

    case WRITE0:
        if (verbose) cout << name() << " ===write0" << endl;
        state = &LowLevel1W::inWritting0;
        cicluReper = get_cycles().get(0.000040);
        pin->update_direction(IOPIN::DIR_OUTPUT, true);
        return;

    case READ:
        if(verbose) cout << name() << " ===read" << endl;
        state = &LowLevel1W::inReading;
        cicluReper = get_cycles().get(0.000030);
        return;

    case RESET:
        if(verbose) cout << name() <<" ===expect reset" << endl;
        state = &LowLevel1W::inResetPulse;
        cicluReper = get_cycles().get(0.000440);
        return;

    case IDLE:
        state = &LowLevel1W::idle;
	return;
    }
}

void LowLevel1W::inResetPulse(bool input, bool isTimeout) {
    if(debug)
      cout <<name() << " inResetPulse input="<<input <<" timout="<<isTimeout<<endl;
    if (input) {
        state = &LowLevel1W::idle;
        return;
    }
    if (!isTimeout) return;
    state = &LowLevel1W::endResetPulse;
}

void LowLevel1W::endResetPulse(bool input, bool isTimeout) {
    if(debug)
      cout << name() << " "<<__FUNCTION__ << "  input="<<input <<" timout="<<isTimeout<<endl;
    if (!input) return;
    gotReset();
    state = &LowLevel1W::inPresencePulse;
    cicluReper = get_cycles().get(0.00005);
}

void LowLevel1W::inPresencePulse(bool input, bool isTimeout) {
    if(debug)
    cout << name() << " "<< __FUNCTION__ << "  input="<<input <<" timout="<<isTimeout<<endl;
    if (!isTimeout) return;
    state = &LowLevel1W::endPresencePulse;
    pin->update_direction(IOPIN::DIR_OUTPUT, true);
    cicluReper = get_cycles().get(0.0002);
}

void LowLevel1W::endPresencePulse(bool input, bool isTimeout) {
    if(debug)
      cout << name() << " " << __FUNCTION__ << "  input="<<input <<" timout="<<isTimeout<<endl;
    if (!isTimeout) return;
    pin->update_direction(IOPIN::DIR_INPUT, true);
    state = &LowLevel1W::waitIdle;
    cicluReper = get_cycles().get(0.00002);
}

void LowLevel1W::waitIdle(bool input, bool isTimeout) {
    if(debug)
      cout << name() << "waitIdle input=" <<input <<" timeout="<<isTimeout<<endl;
    if (!input) return;
    state = &LowLevel1W::idle;
}

void LowLevel1W::inWritting0(bool input, bool isTimeout) {
    if(debug)
      cout << name() << " " << __FUNCTION__ << "  input="<<input <<" timout="<<isTimeout<<endl;
    if (!isTimeout) return;
    state = &LowLevel1W::finalizeBit;
    pin->update_direction(IOPIN::DIR_INPUT, true);
    //cicluReper = get_cycles().get(0.00006);
    cicluReper = get_cycles().get(0.0000050);
}

void LowLevel1W::inWritting1(bool input, bool isTimeout) {
    if(debug)
      cout <<name() << " " << __FUNCTION__ << "  input="<<input <<" timout="<<isTimeout<<endl;
    if (!isTimeout) return;
    if(!input) // OOPS either a reset or bus collision
    {
        state = &LowLevel1W::inResetPulse;
        cicluReper = get_cycles().get(0.000440);
	return;
    }
    state = &LowLevel1W::idle;
    if (bit_remaining() == 0)  gotBitStart();
    //state = &LowLevel1W::finalizeBit;
}

void LowLevel1W::inReading(bool input, bool isTimeout) {
    if(debug)
      cout << name() << " " << __FUNCTION__ << "  input="<<input <<" timout="<<isTimeout<<endl;
    if (input) {
        readBit(true);
        state = &LowLevel1W::idle;
        if (bit_remaining() == 0)  gotBitStart();
        return;
    }
    if (!isTimeout) return;
    readBit(false);
    state = &LowLevel1W::finalizeBit;
    // assume read < 120 us (30 + 90)
    cicluReper = get_cycles().get(0.000090);
}

// Wait for a 1 then go to idle
void LowLevel1W::finalizeBit(bool input, bool isTimeout) {
    if(debug)
      cout << name() << " " <<__FUNCTION__ << "  input="<<input <<" timout="<<isTimeout<<endl;
    if (input) {
        state = &LowLevel1W::idle;
	if (bit_remaining() == 0)  gotBitStart();
        return;
    }
    if (!isTimeout) return;
    state = &LowLevel1W::inResetPulse;
    cicluReper = get_cycles().get(0.000320);
}

