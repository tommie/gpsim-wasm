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
#include <config.h>
#include "rom1w.h"

#include <algorithm>

class ROMCodeAttribute : public Integer {
   unsigned char familycode;

public:

  ROMCodeAttribute(unsigned char _familycode = 0x10)
    : Integer("ROMCode", 0x06050403020100LL, "Device ROM code"), familycode(_familycode)
  {
        // Add CRC
        set(get());
  }


  void set(int64_t i) override
  {
    int64_t id = (i & 0xffffffffffff00LL) | familycode;
    int64_t crc = Rom1W::calculateCRC8((const unsigned char *)&id, 7);
    id |= crc << 56;
    Integer::set(id);
  }

  std::string toString() override
  {
    char buf[64];
    snprintf(buf, sizeof(buf), "0x%" PRINTF_INT64_MODIFIER "x", get());
    return buf;
  }

};

void Rom1W::gotReset()
{
    if (verbose)
        std::cout << name() << " got rom reset\n";
    romState = &Rom1W::readRomCommand;
    bitRemaining = 8;
    isReading = true;
}

static bool getBit(int bitIndex, unsigned char * buffer)
{
    return 0 != (buffer[bitIndex / 8] & (1 << (7 - bitIndex % 8)));
}

Rom1W::NextAction Rom1W::gotBitStart()
{
    if (verbose)
        std::cout << name() << " gotBitStart\n";

    if (--bitRemaining < 0)
        return (this->*romState)();

    if (isReading)
        return READ;

    bool write1 = getBit(bitRemaining, octetBuffer);
    if (verbose)
        std::cout << name() << " writing bit = " << write1 << " remaining " << bitRemaining << '\n';
    return write1 ? WRITE1 : WRITE0;
}

void Rom1W::readBit(bool value)
{
    if (verbose)
      std::cout << name() << " " << __FUNCTION__ << " got readbit = " << value << '\n';
    if (value)
        octetBuffer[bitRemaining / 8] |= 1 << (7 - bitRemaining % 8);
    else
        octetBuffer[bitRemaining / 8] &= ~(1 << (7 - bitRemaining % 8));

    if (verbose && (bitRemaining % 8) == 0)
    {
        printf("%s read byte %0x index %d\n", name().c_str(), octetBuffer[bitRemaining / 8], (bitRemaining / 8));
    }
}

static void int64ToBuff(long long unsigned int num, unsigned char * buff)
{
    for (int i = 0; i < 8; ++i)
        buff[i] = num >> ((7 - i) * 8);
}

Rom1W::NextAction Rom1W::readRomCommand()
{
    if (verbose)
       std::cout << name() << " "<<__FUNCTION__ << " got " << std::hex << (int)octetBuffer[0] << '\n';
    int64_t intaddr;
    switch (octetBuffer[0]) {
    case 0x33:	// Skip ROM
        isSelected = false;
        romState = &Rom1W::readRom;
        intaddr = attr_ROMCode->get();
        int64ToBuff(intaddr, octetBuffer);
        bitRemaining = 64;
        isReading = false;
        return IDLE;

    case 0x55:	// Match ROM
        isSelected = false;
        romState = &Rom1W::matchRom;
        bitRemaining = 64;
        isReading = true;
        return READ;

    case 0xEC:	// Alarm Search
        // Fall through to Search ROM

    case 0xF0:	// Search ROM
        isSelected = (octetBuffer[0] == 0xF0 || isAlarm()) ? true : false;
        romState = &Rom1W::searchRom;
        intaddr = attr_ROMCode->get();
        int64ToBuff(intaddr, octetBuffer + 1);
        if (octetBuffer[8] & 1)
            octetBuffer[0] = 0x40;
        else
            octetBuffer[0] = 0x80;
        octetBuffer[9] = 63;
        bitRemaining = 2;
        isReading = false;
        return IDLE;

    case 0xCC:	// Skip ROM
        isSelected = false;
        if (verbose)
            std::cout << name() << " Skip rom function command\n";
        break;

    case 0xA5:
        if (isSelected)
            break;

    default:
        return RESET;
    }
    return readRom();
}

Rom1W::NextAction Rom1W::readRom() {
    if (verbose)
        std::cout << name() << " called " << __FUNCTION__ << '\n';
    resetEngine();
    romState = &Rom1W::deviceData;
    return IDLE;
}

Rom1W::NextAction Rom1W::matchRom()
{
    if (verbose)
        std::cout << name() << " called " << __FUNCTION__ << '\n';
    unsigned char myaddr[8];
    int64_t intaddr;
    intaddr = attr_ROMCode->get();
    int64ToBuff(intaddr, myaddr);
    if (memcmp(myaddr, octetBuffer, 8))
    {
        if (verbose)
        {
            std::cout << name() << " " << hex << intaddr << " no match\n got ";
            for (int i = 0; i < 8; i++)
                printf("%02x", octetBuffer[i]);
            std::cout << '\n';
        }
        return ignoreData();
    }
    if (verbose)
        std::cout << name() << " " << std::hex << intaddr << " match\n";
    isSelected = true;
    return readRom();
}

Rom1W::NextAction Rom1W::searchRom()
{
    if (verbose)
        std::cout << name() << " called " << __FUNCTION__ << " isReading " << isReading << '\n';
    if (isReading) {
        bool myBit = getBit(octetBuffer[9], octetBuffer + 1);
        if (myBit != (0 != (0x80 & octetBuffer[0])))
        {
            isSelected = false;
        }
        if (!octetBuffer[9]) 	// read all bits
        {
            if (isSelected)
            {
                if (verbose)
                    printf("%s searchRom selected\n", name().c_str());
                return RESET;
            }
            else
            {
                if (verbose)
                    printf("%s searchRom not selected\n", name().c_str());
                return RESET;
            }
        }
        if (getBit(--octetBuffer[9], octetBuffer + 1))
            octetBuffer[0] = 0x40;
        else
            octetBuffer[0] = 0x80;
        if (!isSelected)
            octetBuffer[0] = 0xC0;	// do not pull down the bus
        bitRemaining = 2;
        isReading = false;
        return IDLE;
    }
    isReading = true;
    bitRemaining = 1;
    return IDLE;
}

Rom1W::NextAction Rom1W::deviceData()
{
    if (verbose)
        std::cout << name() << " called " << __FUNCTION__ << '\n';
    doneBits();
    return RESET;
}

//
// Just read data until a reset pulse happens
Rom1W::NextAction Rom1W::ignoreData() {
    if (verbose)
        cout << name() << " called " << __FUNCTION__ << '\n';
    romState = &Rom1W::ignoreData;
    bitRemaining = 64;
    isReading = true;
    return READ;
}

//
// Allow reading of device status
//
Rom1W::NextAction Rom1W::statusPoll()
{
    if (verbose)
        std::cout << name() << " called " << __FUNCTION__ << '\n';
    bitRemaining = 8;
    octetBuffer[0] = isReady?0xff:0x00;
    isReading = false;
    return IDLE;
}

// This is called to setup polling of device status.
// delay is the cycle counter after which the poll will return 1's
void Rom1W::set_status_poll(uint64_t delay)
{
    isReady = false;
    bitRemaining = 8;
    isReading = false;
    octetBuffer[0] = 0x00;
    romState = &Rom1W::statusPoll;
    if (delay > get_cycles().get())
    {
        if (poll_break)
            get_cycles().clear_break(poll_break);
        get_cycles().set_break(delay, this);
        if (verbose)
            printf("%s to poll busy for %.3f mS\n",
            name().c_str(), (delay - get_cycles().get()) * 4.0 / (20.0 * 1000.0));
        poll_break = delay;
    }
}

// Rom1W::callback catches breaks from both Bit1W and Rom1W
// and uses last break setting (bit_break-Bit1W and poll_break-Rom1W)
// to determine what action to take
void Rom1W::callback()
{
    uint64_t now = get_cycles().get();
    if (now == poll_break)
    {
        isReady = true;
        octetBuffer[0] = 0xff;
        poll_break = 0;
    }
    if (now == bit_break)
        LowLevel1W::callback();
}

Rom1W::Rom1W(const char *_name, const char *desc, bool ds18b20):
    LowLevel1W(_name, desc), isSelected(false), isReady(false), bitRemaining(0),
    isReading(false), poll_break(0), romState(&Rom1W::deviceData)
{
    std::fill_n(octetBuffer, 64, 0);
    attr_ROMCode = new ROMCodeAttribute(ds18b20 ? 0x28 : 0x10);
    addSymbol(attr_ROMCode);
}

Rom1W::~Rom1W()
{
    removeSymbol(attr_ROMCode);
    delete attr_ROMCode;
}

static const uint8_t crc8Table[256] = {
    0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65,
    157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130, 220,
    35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98,
    190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255,
    70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7,
    219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
    101, 59, 217, 135, 4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
    248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185,
    140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147, 205,
    17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80,
    175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238,
    50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115,
    202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139,
    87, 9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22,
    233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168,
    116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53};

uint8_t Rom1W::calculateCRC8(const unsigned char *buffer, int bufferLen)
{
    uint8_t crc = 0;
    for (int i = 0; i < bufferLen; ++i)
        crc = crc8Table[crc ^ buffer[i]];
    return crc;
}
