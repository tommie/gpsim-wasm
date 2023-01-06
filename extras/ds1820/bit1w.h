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
#ifndef BIT1W_H
#define BIT1W_H
#ifndef IN_MODULE
#define IN_MODULE
#endif

#include <src/gpsim_time.h>
#include <src/ioports.h>
#include <src/stimuli.h>
#include <src/interface.h>
#include <src/trigger.h>
#include <iostream>

using namespace std;

class LowLevel1W : public Module, public TriggerObject {
protected:
    enum NextAction {WRITE1, WRITE0, READ, RESET, IDLE};
private:
    guint64 cicluReper;
    bool lastValue;
    bool lastTimeout;
    class Pin1W : public IO_bi_directional {
        LowLevel1W &to;
    public:
        Pin1W(char const *name, LowLevel1W &to): IO_bi_directional(name), to(to) {}
        virtual void setDrivenState(bool new_dstate) {
            IO_bi_directional::setDrivenState(new_dstate);
            to.change(true);
        }
        virtual void setDrivenState(char new_state) {
            IO_bi_directional::setDrivenState(new_state);
            to.change(true);
        }
    };

    Pin1W *pin;
    virtual void gotReset() = 0;
    virtual NextAction gotBitStart() = 0;
    virtual void readBit(bool value) = 0;
    virtual int bit_remaining() = 0;
    virtual bool is_reading() = 0;
    void change(bool pinChange);

    void (LowLevel1W::*state)(bool input, bool isTimeout);
    bool ignoreCallback;

    void idle(bool input, bool isTimeout);
    void inResetPulse(bool input, bool isTimeout);
    void endResetPulse(bool input, bool isTimeout);
    void inPresencePulse(bool input, bool isTimeout);
    void endPresencePulse(bool input, bool isTimeout);
    void waitIdle(bool input, bool isTimeout);
    void inWritting0(bool input, bool isTimeout);
    void inWritting1(bool input, bool isTimeout);
    void inReading(bool input, bool isTimeout);
    void finalizeBit(bool input, bool isTimeout);

public:
    guint64 bit_break;
    LowLevel1W(const char *name, const char *desc);
    ~LowLevel1W();
    virtual void callback();
};

#endif
