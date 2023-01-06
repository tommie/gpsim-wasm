/*
   Copyright (C) 1998-2003 Scott Dattalo
                 2004 Rob Pearce
                 2006,2015   Roy R Rankin

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

#include <assert.h>
#include <stdio.h>
#include <iostream>
#include <iomanip>

#include "gpsim_time.h"
#include "processor.h"
#include "registers.h"
#include "stimuli.h"
#include "ui.h"

#include "i2c-ee.h"

//#define DEBUG
#if defined(DEBUG)
#include "../config.h"
#define Dprintf(arg) {printf("%s:%d ",__FILE__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

#define Vprintf(arg) { if (verbose) {printf("%s:%d ",__FILE__,__LINE__); printf arg;} }

// I2C EEPROM Peripheral
//
//  This object emulates the I2C EEPROM peripheral on the 12CE51x
//
//  It's main purpose is to provide a means by which the port pins
//  may communicate.
//


//--------------------------------------------------------------
//
//

PromAddress::PromAddress(I2C_EE *eeprom, const char *_name, const char * desc)
    : Value(_name, desc), m_eeprom(eeprom)
{
}


void PromAddress::get_as(char *buffer, int buf_size)
{
    snprintf(buffer, buf_size, "%p", m_eeprom);
}


class I2C_SLAVE_SDA : public IO_open_collector
{
public:
    I2C_SLAVE_SDA(i2c_slave *_pEE, const char *_name)
        : IO_open_collector(_name), pEE(_pEE)
    {
        bDrivingState = true;
        bDrivenState = true;
        // Make the pin an output.
        update_direction(IO_bi_directional::DIR_OUTPUT, true);
    }

    void setDrivenState(bool new_dstate) override
    {
        bool diff = new_dstate ^ bDrivenState;
        Dprintf(("i2c_slave sda setDrivenState %d\n", new_dstate));

        if (pEE && diff)
        {
            bDrivenState = new_dstate;
            pEE->new_sda_edge(new_dstate);
        }
    }

    void setDrivingState(bool new_state) override
    {
        bDrivingState = new_state;
        bDrivenState = new_state;

        if (!new_state)
        {
            update_direction(IO_bi_directional::DIR_OUTPUT, true);

        }
        else
        {
            update_direction(IO_bi_directional::DIR_INPUT, true);
        }

        if (snode)
        {
            snode->update();
        }
    }

    i2c_slave *pEE;
};


class I2C_SLAVE_SCL : public IO_open_collector
{
public:
    I2C_SLAVE_SCL(i2c_slave *_pEE, const char *_name)
        : IO_open_collector(_name), pEE(_pEE)
    {
        bDrivingState = true;
        bDrivenState = true;
        // Make the pin an output.
        update_direction(IO_bi_directional::DIR_INPUT, true);
    }

    void setDrivenState(bool new_state) override
    {
        bool diff = new_state ^ bDrivenState;
        Dprintf(("i2c_slave scl setDrivenState %d\n", new_state));

        if (pEE && diff)
        {
            bDrivenState = new_state;
            pEE->new_scl_edge(bDrivenState);
        }
    }
    void setDrivingState(bool new_state) override
    {
        bDrivingState = new_state;
        bDrivenState = new_state;

        if (snode)
        {
            snode->update();
        }
    }

    i2c_slave *pEE;
};


i2c_slave::i2c_slave()
{
    scl = new I2C_SLAVE_SCL(this, "SCL");
    sda = new I2C_SLAVE_SDA(this, "SDA");
    bus_state = IDLE;
}


i2c_slave::~i2c_slave()
{
    delete sda;
    delete scl;
}


void i2c_slave::new_scl_edge(bool level)
{
    scl_high = level;
    get_cycles().set_break(get_cycles().get() + 1, this);

    if (scl_high)	//SCL goes high
    {
	Vprintf(("SCL goes high sda_high:%d state=%s\n", sda_high, state_name()));
    }
    else  	//SCL goes low
    {
        if (bus_state == RX_DATA && bit_count == 0)
        {
            sda->setDrivingState(true);   // Master drives bus
        }
    }
}


void i2c_slave::callback()
{
    if (scl_high)  	// read data from master
    {
        switch (bus_state)
        {
        case RX_I2C_ADD :	// Read address, send ACK to master if us
            if (shift_read_bit(sda->getDrivenState()))
            {
                Vprintf(("%s : got i2c address 0x%x r/w %u ", __FUNCTION__,
                         xfr_data >> 1, xfr_data & 1));

                if (match_address())
                {
                    bus_state = ACK_I2C_ADD;
                    r_w = xfr_data & 1;
                    Vprintf((" - OK\n"));

                }
                else
                {
                    // not for us
                    bus_state = IDLE;
                    Vprintf((" - not for us\n"));
                }
            }

            break;

        case RX_DATA :	// read data from master, send ACK when complete
            if (shift_read_bit(sda->getDrivenState()))
            {
                //start_write();
                Vprintf(("%s : data set to 0x%x\n", __FUNCTION__, xfr_data));
                put_data(xfr_data);
                bus_state = ACK_RX;
            }

            break;

        case ACK_RD :	// read ACK/NACK from master
            if (sda->getDrivenState() == false)   // ACK
            {
                // The master has asserted ACK, so we send another byte
                bus_state = TX_DATA;
                bit_count = 8;
                xfr_data = get_data();

            }
            else  				// NACK
            {
                bus_state = IDLE;   // Terminate writes to master
            }

            break;

        case ACK_WR :	// slave sent ACK/NACK
            if (r_w)
            {
                // master is reading, we transmit
                bus_state = TX_DATA;
                bit_count = 8;
                xfr_data = get_data();

            }
            else
            {
                // master is writing, we read
                bus_state = RX_DATA;
                bit_count = 0;
                xfr_data = 0;
            }

            break;

        case ACK_RX :	// ACK being read by master
            bus_state = RX_DATA;
            bit_count = 0;
            xfr_data = 0;
            break;

        default:
            break;
        }

    }
    else  	// SCL low, put data on bus for master (if required)
    {
        switch (bus_state)
        {
        case ACK_I2C_ADD :		// after address ACK start to send data
            sda->setDrivingState(false);     // send ACK
            bus_state = ACK_WR;

            // Check the R/W bit of the address byte

            if (xfr_data & 0x01)
            {
                slave_transmit(true);
            }
            else
            {
                slave_transmit(false);
            }

            break;

        case TX_DATA :	// send data to master
            if (bit_count == 0)
            {
                sda->setDrivingState(true);        // Release the bus
                bus_state = ACK_RD;

            }
            else
            {
                sda->setDrivingState(shift_write_bit());
            }

            break;

        case ACK_RX :	// Send ACK read data
            sda->setDrivingState(false);
            break;

        default:
            break;
        }
    }
}


void i2c_slave::new_sda_edge(bool sda_val)
{
    sda_high = sda_val;
    //     Vprintf(("i2c_slave::new_sda_edge sda_high:%d\n", sda_high));
    if (scl->getDrivenState())  	// SCL high
    {
        int curBusState = bus_state;

        if (sda_high)  	// SDA high
        {
            /* stop bit */
            Vprintf(("i2c_slave : Rising edge in SCL high => stop bit %s\n", state_name()));

            if (bus_state == WRPEND)
            {
                Vprintf(("i2c_slave : write is pending - commence...\n"));
                bus_state = IDLE;   // Should be busy

            }
            else
            {
                bus_state = IDLE;
            }

        }
        else	// SDA going low, SCL high
        {
            /* start bit */
            Vprintf(("i2c_slave : SDA Falling edge with SCL high => start bit state=%s\n", state_name()));

            bus_state = RX_I2C_ADD;
            bit_count = 0;
            xfr_data = 0;
        }

        if (bus_state != curBusState)
        {
            Vprintf(("i2c_slave::new_sda_edge() new bus state = %s\n", state_name()));
        }
    }
    else	// SCL low
    {
	if (sda_high)	// SDA going high
	    Vprintf(("new_sda_edge() SCL=low SDA=%d state=%s bit_count=%d\n", sda_high, state_name(), bit_count));
    }
}


bool i2c_slave::shift_read_bit(bool x)
{
    xfr_data = (xfr_data << 1) | (x != 0);
    bit_count++;

    if (bit_count == 8)
    {
        return true;

    }
    else
    {
        return false;
    }
}


bool i2c_slave::shift_write_bit()
{
    bit_count--;
    bool bit = (xfr_data >> bit_count) & 1;
    Dprintf(("I2c_slave : send bit %u = %c\n", bit_count,
             bit ? '1' : '0'));
    return bit;
}


bool i2c_slave::match_address()
{
    if ((xfr_data & 0xfe) == i2c_slave_address)
    {
        r_w = xfr_data & 1;
        return true;
    }

    return false;
}


const char * i2c_slave::state_name()
{
    switch (bus_state)
    {
    case IDLE:
        return "IDLE";
        break;

    case START:
        return "START";
        break;

    case RX_I2C_ADD:
        return "RX_I2C_ADD";
        break;

    case ACK_I2C_ADD:
        return "ACK_I2C_ADD";
        break;

    case RX_DATA:
        return "RX_DATA";
        break;

    case ACK_WR:
        return "ACK_WR";
        break;

    case ACK_RX:
        return "ACK_RX";
        break;

    case WRPEND:
        return "WRPEND";
        break;

    case ACK_RD:
        return "ACK_RD";
        break;

    case TX_DATA:
        return "TX_DATA";
        break;
    }

    return "UNKNOWN";
}


//----------------------------------------------------------
//
// I2C EE PROM
//
// There are many conditions that need to be verified against a real part:
//    1) what happens if
//       > the simulator
//    2) what happens if a RD is initiated while data is being written?
//       > the simulator ignores the read
//    3) what happens if
//       > the simulator

I2C_EE::I2C_EE(Processor *pCpu, unsigned int _rom_size, unsigned int _write_page_size,
               unsigned int _addr_bytes, unsigned int _CSmask,
               unsigned int _BSmask, unsigned int _BSshift)
    : i2c_slave(),
      rom_size(_rom_size),                // size of eeprom in bytes
      rom_data_size(1),
      write_page_size(_write_page_size),  // Page size for writes
      m_CSmask(_CSmask),                  // mask for chip select in command
      m_BSmask(_BSmask),                  // mask for bank select in command
      m_BSshift(_BSshift),                // right shift bank select to bit 0
      m_addr_bytes(_addr_bytes),          // number of address bytes
      io_state(RX_EE_ADDR)
{
    // Create the rom
    //rom = (Register **) new char[sizeof (Register *) * rom_size];
    rom = new Register * [rom_size];
    // Initialize the rom
    char str[100];

    for (unsigned int i = 0; i < rom_size; i++)
    {
        snprintf(str, sizeof(str), "ee0x%02x", i);
        rom[i] = new Register(pCpu, str, "");
        rom[i]->address = i;
        rom[i]->value.put(0);
        rom[i]->alias_mask = 0;
    }

    if (pCpu)
    {
        m_UiAccessOfRom = new RegisterCollection(pCpu, "eeData", rom, rom_size);

    }
    else
    {
        m_UiAccessOfRom = nullptr;
    }
}


I2C_EE::~I2C_EE()
{
    for (unsigned int i = 0; i < rom_size; i++)
    {
        delete rom[i];
    }

    delete [] rom;
    delete m_UiAccessOfRom;
}


void I2C_EE::slave_transmit(bool yes)
{
    if (yes)  	// prepare to output eeprom data
    {
        io_state = TX_EE_DATA;
        xfr_addr += write_page_off;
        write_page_off = 0;

    }
    else  	// getting eeprom address
    {
        io_state = RX_EE_ADDR;
        xfr_addr = (m_command & m_BSmask) >> m_BSshift;
        m_addr_cnt = m_addr_bytes;
    }
}


// data written by master device
void I2C_EE::put_data(unsigned int data)
{
    switch (io_state)
    {
    case RX_EE_ADDR:
        // convert xfr_data to base and page offset to allow
        // sequencel writes to wrap around page
        xfr_addr = ((xfr_addr << 8) | data) % rom_size;

        if (--m_addr_cnt == 0)
        {
            write_page_off = xfr_addr % write_page_size;
            xfr_addr -= write_page_off;
            Vprintf(("I2C_EE : address set to 0x%x page offset 0x%x data:0x%x\n",
                     xfr_addr, write_page_off, data));
            io_state = RX_EE_DATA;
        }

        break;

    case RX_EE_DATA:
        if (! m_write_protect)
        {
            rom[xfr_addr + write_page_off]->value.put(data);
            write_page_off = (write_page_off + 1) % write_page_size;

        }
        else
        {
            std::cout << "I2c_EE start_write- write protect\n";
        }

        break;

    case TX_EE_DATA:
        std::cout << "I2C_EE put_data in output state\n";
        break;

    default:
        std::cout << "I2c_EE unexpected state\n";
        break;
    }
}


unsigned int I2C_EE::get_data()
{
    unsigned int data = rom[xfr_addr]->get();
    xfr_addr = (xfr_addr + 1) % rom_size;
    return data;
}


// Bit 0 is write protect, 1-3 is A0 - A2
void I2C_EE::set_chipselect(unsigned int _cs)
{
    m_write_protect = (_cs & 1) == 1;
    m_chipselect = (_cs & m_CSmask);
}


void I2C_EE::debug()
{
    if (!scl || !sda || !rom)
    {
        return;
    }

    std::cout << "I2C EEPROM: current state=" << state_name() << '\n';
    std::cout << " t=0x" << std::hex << get_cycles().get() << '\n';
    std::cout << "  scl drivenState="  << scl->getDrivenState()
              << " drivingState=" << scl->getDrivingState()
              << " direction=" << ((scl->get_direction() == IOPIN::DIR_INPUT) ? "IN" : "OUT")
              << '\n';
    std::cout << "  sda drivenState="  << sda->getDrivenState()
              << " drivingState=" << sda->getDrivingState()
              << " direction=" << ((sda->get_direction() == IOPIN::DIR_INPUT) ? "IN" : "OUT")
              << '\n';
    std::cout << "  bit_count:" << bit_count
              << " ee_busy:" << ee_busy
              << " xfr_addr:0x" << std::hex << xfr_addr
              << " xfr_data:0x" << std::hex << xfr_data
              << '\n';
}


Register * I2C_EE::get_register(unsigned int address)
{
    if (address < rom_size)
    {
        return rom[address];
    }

    return nullptr;
}


void I2C_EE::change_rom(unsigned int offset, unsigned int val)
{
    assert(offset < rom_size);
    rom[offset]->value.put(val);
}


// write data to eeprom unles write protect is active
void I2C_EE::start_write()
{
    unsigned int addr = xfr_addr + write_page_off;

    if (! m_write_protect)
    {
        rom[addr]->put(xfr_data);

    }
    else
    {
        std::cout << "I2c_EE start_write- write protect\n";
    }
}


// allow 5 msec after last write
void I2C_EE::write_busy()
{
    uint64_t fc;

    if (! ee_busy && ! m_write_protect)
    {
        fc = (uint64_t)(get_cycles().instruction_cps() * 0.005);
        get_cycles().set_break(get_cycles().get() + fc, this);
        ee_busy = true;
    }
}


void I2C_EE::write_is_complete()
{
}


/* RRR
void I2C_EE::callback()
{

  ee_busy = false;
  Vprintf(("I2C_EE::callback() - write cycle is complete\n"));
}
*/


void I2C_EE::callback_print()
{
    std::cout << "Internal I2C-EEPROM\n";
}


bool I2C_EE::match_address()
{
    if ((xfr_data & 0xf0) == 0xa0 && ((xfr_data & m_CSmask) == m_chipselect))
    {
        m_command = xfr_data;
        return true;
    }

    return false;
}


void I2C_EE::reset(RESET_TYPE by)
{
    switch (by)
    {
    case POR_RESET:
        bus_state = IDLE;
        ee_busy = false;
        break;

    default:
        break;
    }
}


void I2C_EE::attach(Stimulus_Node *_scl, Stimulus_Node *_sda)
{
    _scl->attach_stimulus(scl);
    _sda->attach_stimulus(sda);
}


void I2C_EE::dump()
{
    unsigned int i, j, reg_num, v;
    std::cout << "     " << std::hex;

    // Column labels
    for (i = 0; i < 16; i++)
    {
        std::cout << std::setw(2) << std::setfill('0') << i << ' ';
    }

    std::cout << '\n';

    for (i = 0; i < rom_size / 16; i++)
    {
        std::cout << std::setw(2) << std::setfill('0') << i << ":  ";

        for (j = 0; j < 16; j++)
        {
            reg_num = i * 16 + j;

            if (reg_num < rom_size)
            {
                v = rom[reg_num]->get_value();
                std::cout << std::setw(2) << std::setfill('0') << v << ' ';

            }
            else
            {
                std::cout << "-- ";
            }
        }

        std::cout << "   ";

        for (j = 0; j < 16; j++)
        {
            reg_num = i * 16 + j;

            if (reg_num < rom_size)
            {
                v = rom[reg_num]->get_value();

                if ((v >= ' ') && (v <= 'z'))
                {
                    std::cout.put(v);

                }
                else
                {
                    std::cout.put('.');
                }
            }
        }

        std::cout << '\n';
    }
}
