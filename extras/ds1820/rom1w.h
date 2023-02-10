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
#ifndef ROM1W_H
#define ROM1W_H
#ifndef IN_MODULE
#define IN_MODULE
#endif

#include "bit1w.h"
#include "../../src/value.h"

class ROMCodeAttribute;

class Rom1W : public LowLevel1W {
private:
    virtual void gotReset();
    virtual NextAction gotBitStart();
    virtual void readBit(bool value);
    virtual int bit_remaining() { return bitRemaining;}
    virtual bool is_reading() { return isReading;}

    bool isSelected;
    bool isReady;
    ROMCodeAttribute *attr_ROMCode;

    virtual void doneBits() = 0;
    virtual void resetEngine() = 0;
    virtual bool isAlarm() { return false;}

protected:
    int bitRemaining;
    bool isReading;
    uint64_t	poll_break;
    unsigned char octetBuffer[64];

private:

    NextAction (Rom1W::*romState)();

    NextAction readRomCommand();
    NextAction readRom();
    NextAction matchRom();
    NextAction searchRom();
    NextAction deviceData();
    NextAction ignoreData();
    NextAction statusPoll();

    virtual void callback();

public:
    Rom1W(const char *name, const char *desc, bool ds18b20 = false);
    ~Rom1W();
    static unsigned char calculateCRC8(const unsigned char *buffer, int bufferLen);
    void set_status_poll(uint64_t delay);
};

#endif
