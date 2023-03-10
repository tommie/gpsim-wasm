/*
   Copyright (C) 1998-2003 Scott Dattalo
                 2003 Mike Durian
                 2006,2013 Roy Rankin

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

#include <iostream>
#include <iomanip>

#include "gpsim_interface.h"
#include "gpsim_time.h"
#include "trace.h"
#include "pic-processor.h"
#include "eeprom.h"
#include "pir.h"
#include "intcon.h"
#include "processor.h"


// EEPROM - Peripheral
//
//  This object emulates the 14-bit core's EEPROM/FLASH peripheral
//  (such as the 16c84).
//
//  It's main purpose is to provide a means by which the control
//  registers may communicate.
//

//------------------------------------------------------------------------
//
// EEPROM related registers

//#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("%s:%d-%s() ",__FILE__,__LINE__,__FUNCTION__); printf arg; }
#else
#define Dprintf(arg) {}
#endif


void EECON1::put(unsigned int new_value)
{
  emplace_value_trace<trace::WriteRegisterEntry>();
  put_value(new_value);
}


void EECON1::put_value(unsigned int new_value)
{
  new_value &= valid_bits;
  new_value |= always_on_bits;

  //cout << "EECON1::put new_value " << hex << new_value << "  valid_bits " << valid_bits << '\n';
  Dprintf(("new_value %x valid_bits %x\n", new_value, valid_bits));

  if (new_value & WREN) {
    if (eeprom->get_reg_eecon2()->is_unarmed()) {
      eeprom->get_reg_eecon2()->unready();
      value.put(value.get() | WREN);
    }

    // WREN is true and EECON2 is armed (which means that we've passed through here
    // once before with WREN true). Initiate an eeprom write only if WR is true and
    // RD is false AND EECON2 is ready

    else if ((new_value & WR) && !(new_value & RD) &&
             (eeprom->get_reg_eecon2()->is_ready_for_write())) {
      value.put(value.get() | WR);
      eeprom->start_write();

    } else if ((new_value & WR) && (new_value & RD)) {
      std::cout << "\n*** EECON1: write ignored " << std::hex << new_value << " both WR & RD set\n\n";
    }

    //    else cout << "EECON1: write ignored " << new_value << "  (WREN is probably already set)\n";

  } else {
    // WREN is low so inhibit further eeprom writes:
    if (! eeprom->get_reg_eecon2()->is_writing()) {
      eeprom->get_reg_eecon2()->unarm();
    }

    //cout << "EECON1: write is disabled\n";
  }

  value.put((value.get() & (RD | WR)) | new_value);


  if ((value.get() & RD) && !(value.get() & WR)) {
    Dprintf(("RD true WR false EEPGD|CFGS %x\n", value.get() & (EEPGD | CFGS)));

    if (value.get() & (EEPGD | CFGS)) {
      eeprom->get_reg_eecon2()->read();
      eeprom->start_program_memory_read();
      //cout << "eestate " << eeprom->eecon2->eestate << '\n';
      // read program memory

    } else {
      //eeprom->eedata->value = eeprom->rom[eeprom->eeadr->value]->get();
      eeprom->get_reg_eecon2()->read();
      eeprom->callback();
      value.put(value.get() & ~RD);
    }
  }
}


unsigned int EECON1::get()
{
  emplace_value_trace<trace::ReadRegisterEntry>();
  return value.get();
}


EECON1::EECON1(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc), eeprom(nullptr)
{
  valid_bits = EECON1_VALID_BITS;
  always_on_bits = 0;
}


void EECON2::put(unsigned int new_value)
{
  emplace_value_trace<trace::WriteRegisterEntry>();
  value.put(new_value);

  if ((eestate == EENOT_READY) && (0x55 == new_value)) {
    eestate = EEHAVE_0x55;

  } else if ((eestate == EEHAVE_0x55) && (0xaa == new_value)) {
    eestate = EEREADY_FOR_WRITE;

  } else if ((eestate == EEHAVE_0x55) || (eestate == EEREADY_FOR_WRITE)) {
    eestate = EENOT_READY;
  }
}


unsigned int EECON2::get()
{
  emplace_value_trace<trace::ReadRegisterEntry>();
  return 0;
}


EECON2::EECON2(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc), eeprom(nullptr)
{
  ee_reset();
}


unsigned int EEDATA::get()
{
  emplace_value_trace<trace::ReadRegisterEntry>();
  return value.get();
}


void EEDATA::put(unsigned int new_value)
{
  emplace_value_trace<trace::WriteRegisterEntry>();
  value.put(new_value);
}


EEDATA::EEDATA(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc), eeprom(nullptr)
{
}


unsigned int EEADR::get()
{
  emplace_value_trace<trace::ReadRegisterEntry>();
  return value.get();
}


void EEADR::put(unsigned int new_value)
{
  emplace_value_trace<trace::WriteRegisterEntry>();
  value.put(new_value);
}


EEADR::EEADR(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc), eeprom(nullptr)
{
}


//------------------------------------------------------------------------
EEPROM_PIR::EEPROM_PIR(Processor *pCpu, PIR *pPir)
  : EEPROM(pCpu), m_pir(pPir),
    eeadrh(pCpu, "eeadrh", "EE Address High byte")
{
}


EEPROM_PIR::~EEPROM_PIR()
{
  pic_processor *pCpu = dynamic_cast<pic_processor *>(cpu);

  if (pCpu) {
    pCpu->remove_sfr_register(&eeadrh);
  }
}


void EEPROM_PIR::start_write()
{
  get_cycles().set_break(get_cycles().get() + EPROM_WRITE_TIME, this);

  if (rom_size > 256) {
    wr_adr = eeadr.value.get() + (eeadrh.value.get() << 8);

  } else {
    wr_adr = eeadr.value.get();
  }

  wr_data = eedata.value.get();
  eecon2.start_write();
}


//------------------------------------------------------------------------
// Set the EEIF and clear the WR bits.

void EEPROM_PIR::write_is_complete()
{
  eecon1.value.put(eecon1.value.get()  & (~eecon1.WR));
  Dprintf(("eecon1 0x%x\n", eecon1.value.get()));
  if (m_pir)
      m_pir->set_eeif();
}


void EEPROM_PIR::initialize(unsigned int new_rom_size)
{
  eeadrh.set_eeprom(this);
  EEPROM::initialize(new_rom_size);
}


//----------------------------------------------------------
//
// EE PROM
//
// There are many conditions that need to be verified against a real part:
//    1) what happens if RD and WR are set at the same time?
//       > the simulator ignores both the read and the write.
//    2) what happens if a RD is initiated while data is being written?
//       > the simulator ignores the read
//    3) what happens if EEADR or EEDATA are changed while data is being written?
//       > the simulator will update these registers with the new values that
//         are written to them, however the write operation will be unaffected.
//    4) if WRERR is set, will this prevent a valid write sequence from being initiated?
//       > the simulator views WRERR as a status bit
//    5) if a RD is attempted after the eeprom has been prepared for a write
//       will this affect the RD or write?
//       > The simulator will proceed with the read and leave the write-enable state alone.
//    6) what happens if WREN goes low while a write is happening?
//       > The simulator will complete the write and WREN will be cleared.

EEPROM::EEPROM(Processor *pCpu)
  : name_str(nullptr),
    cpu(pCpu),
    intcon(nullptr),
    eecon1(pCpu, "eecon1", "EE Control 1"),
    eecon2(pCpu, "eecon2", "EE Control 2"),
    eedata(pCpu, "eedata", "EE Data"),
    eeadr(pCpu, "eeadr", "EE Address"),
    rom(nullptr),
    rom_data_size(1),
    rom_size(0),
    wr_adr(0), wr_data(0), rd_adr(0), abp(0)
{
}


EEPROM::~EEPROM()
{
  pic_processor *pCpu = dynamic_cast<pic_processor *>(cpu);

  if (pCpu) {
    pCpu->remove_sfr_register(&eedata);
    pCpu->remove_sfr_register(&eeadr);
    pCpu->remove_sfr_register(&eecon1);
    pCpu->remove_sfr_register(&eecon2);
  }

  for (unsigned int i = 0; i < rom_size; i++) {
    delete rom[i];
  }

  delete [] rom;
}


Register *EEPROM::get_register(unsigned int address)
{
  if (address < rom_size) {
    return rom[address];
  }

  return nullptr;
}


void EEPROM::start_write()
{
  get_cycles().set_break(get_cycles().get() + EPROM_WRITE_TIME, this);
  wr_adr = eeadr.value.get();
  wr_data = eedata.value.get();
  eecon2.start_write();
}


// Set the EEIF and clear the WR bits.

void EEPROM::write_is_complete()
{
  assert(intcon != nullptr);
  eecon1.value.put((eecon1.value.get()  & (~eecon1.WR)) | eecon1.EEIF);
  intcon->peripheral_interrupt();
}


void EEPROM::start_program_memory_read()
{
  std::cout << "ERROR: program memory flash should not be accessible\n";
}


void EEPROM::callback()
{
  switch (eecon2.get_eestate()) {
  case EECON2::EEREAD:
    //cout << "eeread\n";
    eecon2.unarm();

    if (get_address() < rom_size) {
      eedata.value.put(rom[get_address()]->get());

    } else {
      std::cout << "EEPROM read address is out of range " << std::hex << eeadr.value.get() << '\n';
    }

    eecon1.value.put(eecon1.value.get() & (~EECON1::RD));
    break;

  case EECON2::EEWRITE_IN_PROGRESS:

    //cout << "eewrite\n";
    if (wr_adr < rom_size) {
      rom[wr_adr]->value.put(wr_data);

    } else {
      std::cout << "EEPROM write address is out of range " << std::hex << wr_adr << '\n';
    }

    write_is_complete();

    if (eecon1.value.get() & eecon1.WREN) {
      eecon2.unready();

    } else {
      eecon2.unarm();
    }

    break;
    // FIXME: Why is this after the break ??? eecon1.value.put(eecon1.value.get() & (~EECON1::WR));

  default:
    std::cout << "EEPROM::callback() bad eeprom state " <<
         eecon2.get_eestate() << '\n';
  }
}


void EEPROM::reset(RESET_TYPE by)
{
  switch (by) {
  case POR_RESET:
    eecon1.value.put(0);          // eedata & eeadr are undefined at power up
    eecon2.unarm();
    break;

  default:
    break;
  }
}


void EEPROM::initialize(unsigned int new_rom_size)
{
  rom_size = new_rom_size;
  // Let the control registers have a pointer to the peripheral in
  // which they belong.
  eecon1.set_eeprom(this);
  eecon2.set_eeprom(this);
  eedata.set_eeprom(this);
  eeadr.set_eeprom(this);
  // Create the rom
  rom = (Register **) new char[sizeof(Register *) * rom_size];
  assert(rom != nullptr);
  // Initialize the rom
  char str[100];

  for (unsigned int i = 0; i < rom_size; i++) {
    snprintf(str, sizeof(str), "eereg 0x%02x", i);
    rom[i] = new Register(cpu, str);
    rom[i]->address = i;
    rom[i]->value.put(0);
    rom[i]->alias_mask = 0;
  }

  if (cpu) {
    //cpu->ema.set_cpu(cpu);
    cpu->ema.set_Registers(rom, rom_size);
  }
}


void EEPROM::set_intcon(INTCON *ic)
{
  intcon = ic;
}


void EEPROM::dump()
{
  unsigned int i, j, reg_num, v;
  std::cout << "     " << std::hex;

  // Column labels
  for (i = 0; i < 16; i++) {
    std::cout << std::setw(2) << std::setfill('0') <<  i << ' ';
  }

  std::cout << '\n';

  for (i = 0; i < rom_size / 16; i++) {
    std::cout << std::setw(2) << std::setfill('0') <<  i << ":  ";

    for (j = 0; j < 16; j++) {
      reg_num = i * 16 + j;

      if (reg_num < rom_size) {
        v = rom[reg_num]->get_value();
        std::cout << std::setw(2) << std::setfill('0') <<  v << ' ';

      } else {
        std::cout << "-- ";
      }
    }

    std::cout << "   ";

    for (j = 0; j < 16; j++) {
      reg_num = i * 16 + j;

      if (reg_num < rom_size) {
        v = rom[reg_num]->get_value();

        if ((v >= ' ') && (v <= 'z')) {
          std::cout.put(v);

        } else {
          std::cout.put('.');
        }
      }
    }

    std::cout << '\n';
  }
}


//------------------------------------------------------------------------
EEPROM_WIDE::EEPROM_WIDE(Processor *pCpu, PIR *pPir)
  : EEPROM_PIR(pCpu, pPir),
    eedatah(pCpu, "eedatah", "EE Data High byte")
{
}


EEPROM_WIDE::~EEPROM_WIDE()
{
  pic_processor *pCpu = dynamic_cast<pic_processor *>(cpu);
  pCpu->remove_sfr_register(&eedatah);
}


void EEPROM_WIDE::start_write()
{
  get_cycles().set_break(get_cycles().get() + EPROM_WRITE_TIME, this);
  wr_adr = eeadr.value.get() + (eeadrh.value.get() << 8);
  wr_data = eedata.value.get() + (eedatah.value.get() << 8);
  eecon2.start_write();
}


void EEPROM_WIDE::start_program_memory_read()
{
  rd_adr = eeadr.value.get() | (eeadrh.value.get() << 8);
  get_cycles().set_break(get_cycles().get() + 2, this);
  // The next 2 commands are executed as NOPs as program memory is read.
  cpu->inattentive(2);
}


void EEPROM_WIDE::callback()
{
  //cout << "eeprom call back\n";
  Dprintf(("state 0x%x\n", eecon2.get_eestate()));

  switch (eecon2.get_eestate()) {
  case EECON2::EEREAD:
    //cout << "eeread\n";
    eecon2.unarm();

    if (eecon1.value.get() & EECON1::EEPGD) {
      // read program memory
      int opcode = cpu->pma->get_opcode(rd_adr);
      eedata.value.put(opcode & 0xff);
      eedatah.value.put((opcode >> 8) & 0xff);

    } else {
      if (eeadr.value.get() < rom_size) {
        eedata.value.put(rom[eeadr.value.get()]->get());

      } else {
        std::cout << "WIDE_EEPROM read address is out of range " << std::hex << eeadr.value.get() << '\n';
      }
    }

    eecon1.value.put(eecon1.value.get() & (~EECON1::RD));
    break;

  case EECON2::EEWRITE_IN_PROGRESS:
    //cout << "eewrite\n";
    Dprintf(("EEWRITE_IN_PROGRESS eecon1 %x\n", eecon1.value.get()));

    if (eecon1.value.get() & EECON1::EEPGD) { // write program memory
      cpu->init_program_memory_at_index(wr_adr, wr_data);

    } else {                              // write eeprom memory
      Dprintf(("wr_adr 0x%x rom_size 0x%x data 0x%x\n", wr_adr, rom_size, wr_data));

      if (wr_adr < rom_size) {
        rom[wr_adr]->value.put(wr_data);

      } else {
        std::cout << "WIDE_EEPROM write address is out of range " << std::hex << wr_adr << '\n';
      }
    }

    write_is_complete();

    if (eecon1.value.get() & eecon1.WREN) {
      eecon2.unready();

    } else {
      eecon2.unarm();
    }

    break;

  default:
    std::cout << "EEPROM_WIDE::callback() bad eeprom state " << eecon2.get_eestate() << '\n';
  }
}


void EEPROM_WIDE::initialize(unsigned int new_rom_size)
{
  eedatah.set_eeprom(this);
  eeadrh.set_eeprom(this);
  EEPROM::initialize(new_rom_size);
}


void EEPROM_PIR::callback()
{
  switch (eecon2.get_eestate()) {
  case EECON2::EEREAD:
    //cout << "eeread\n";
    eecon2.unarm();

    if (eecon1.value.get() & EECON1::EEPGD) {
      std::cout << "Should not be possible to get here\n";

    } else {
      if (get_address() < rom_size) {
        eedata.value.put(rom[get_address()]->get());

      } else {
        std::cout << "LONG_EEPROM read address is out of range " << std::hex << eeadr.value.get()  + (eeadrh.value.get() << 8) << '\n';
      }
    }

    eecon1.value.put(eecon1.value.get() & (~EECON1::RD));
    break;

  case EECON2::EEWRITE_IN_PROGRESS:

    //cout << "eewrite\n";
    if (eecon1.value.get() & EECON1::EEPGD) { // write program memory
      std::cout << "EEPROM_PIR can't do program writes\n";

    } else {                              // read eeprom memory
      if (wr_adr < rom_size) {
        rom[wr_adr]->value.put(wr_data);

      } else {
        std::cout << "LONG_EEPROM write address is out of range " << std::hex << wr_adr << '\n';
      }
    }

    write_is_complete();

    if (eecon1.value.get() & eecon1.WREN) {
      eecon2.unready();

    } else {
      eecon2.unarm();
    }

    break;

  default:
    std::cout << "EEPROM_LONG::callback() bad eeprom state " << eecon2.get_eestate() << '\n';
  }
}


//------------------------------------------------------------------------
// EEPROM_EXTND is based on data sheets for 16F193X and
// 12(L)F1822/PIC16(L)F1823. It has the following features
//
// read/write with 16 bit data path
// performs I/O to eeprom, program and configuration memory
// supports data latches for writing to program and Configuration memory
// Performs block erase of program and Configuration memory
// Supports write protect of program memory
//
// RP : Amended to also suit the 12F1571 where there is no EEPROM but
//      the flash program memory controller looks nearly identical
//
EEPROM_EXTND::EEPROM_EXTND(Processor *pCpu, PIR *pPir)
  : EEPROM_WIDE(pCpu, pPir),
    erase_block_size(0), num_write_latches(0),
    write_latches(nullptr), config_word_base(0), prog_wp(0), has_eeadrh(false)
{
}


EEPROM_EXTND::~EEPROM_EXTND()
{
  delete[] write_latches;
}


void EEPROM_EXTND::initialize(
  unsigned int new_rom_size,
  int block_size,
  int num_latches,
  unsigned int cfg_word_base,
  bool _has_eeadrh)

{
  EEPROM_WIDE::initialize(new_rom_size);
   if ( new_rom_size == 0 )
   {
       eecon1.always_on_bits = EECON1::EEPGD;      // this is a flash-only device
       eecon1.valid_bits &= ~EECON1::EEPGD;
   }
  erase_block_size = block_size;
  num_write_latches = num_latches;
  delete[] write_latches;
  write_latches = new unsigned int [num_latches];

  for (int i = 0; i < num_latches; i++) {
    write_latches[i] = LATCH_MT;
  }

  config_word_base = cfg_word_base;
  has_eeadrh = _has_eeadrh;
}


void EEPROM_EXTND::start_program_memory_read()
{
  // Bit 7 of EEADRH reads back as 1
  rd_adr = eeadr.value.get() | ((eeadrh.value.get() & 0x7f) << 8);
  get_cycles().set_break(get_cycles().get() + 2, this);
  // The next 2 commands are executed as NOPs as program memory is read.
  // SB: The data sheet implies that the first instruction is executed, and
  // only the second instruction will be a forced NOP. But that is not how
  // it works on a real PIC (verified on a PIC16F1847).
  cpu->inattentive(2);
}


void EEPROM_EXTND::start_write()
{
  eecon1.value.put(eecon1.value.get()  | eecon1.WRERR);
  wr_adr = eeadr.value.get() + ((eeadrh.value.get() & 0x7f) << 8);
  wr_data = eedata.value.get() + (eedatah.value.get() << 8);
  eecon2.start_write();

  if (eecon1.value.get() & (EECON1::EEPGD | EECON1::CFGS)) {
    // stop execution for 2 ms
    get_cycles().set_break(get_cycles().get() + (uint64_t)(0.002 * get_cycles().instruction_cps()), this);
    cpu->pc->increment();
    static_cast<pic_processor*>(cpu)->pm_write();

  } else {
    get_cycles().set_break(get_cycles().get() + EPROM_WRITE_TIME, this);
  }
}


void EEPROM_EXTND::callback()
{
  int index;
  bool write_error = false;
  //cout << "eeprom call back\n";

  switch (eecon2.get_eestate()) {
  case EECON2::EEREAD:
    //cout << "eeread\n";
    eecon2.unarm();

    if (eecon1.value.get() & EECON1::CFGS) { // Read Config data
      unsigned int read_data;
      read_data = cpu->get_config_word(config_word_base | rd_adr);
      Dprintf(("read_data=0x%x config_word_base=0x%x rd_adr=0x%x\n", read_data, config_word_base, rd_adr));

      if (read_data == 0xffffffff) {
        read_data = 0;
      }

      eedata.value.put(read_data & 0xff);
      eedatah.value.put((read_data >> 8) & 0xff);

    } else if (eecon1.value.get() & EECON1::EEPGD) { // read program memory
      int opcode = cpu->pma->get_opcode(rd_adr);
      eedata.value.put(opcode & 0xff);
      eedatah.value.put((opcode >> 8) & 0xff);

    } else {	// read eeprom data
      if (eeadr.value.get() < rom_size) {
        eedata.value.put(rom[eeadr.value.get()]->get_value());

      } else {
        std::cout << "EXTND_EEPROM read address is out of range " << std::hex << eeadr.value.get() << '\n';
      }
    }

    eecon1.value.put(eecon1.value.get() & (~EECON1::RD));
    break;

  case EECON2::EEWRITE_IN_PROGRESS:

    //cout << "eewrite\n";
    switch (eecon1.value.get() & (EECON1::EEPGD | EECON1::CFGS | EECON1::LWLO | EECON1::FREE)) {
    case EECON1::EEPGD:	// write program memory
      index = wr_adr & (num_write_latches - 1);
      wr_adr &= ~(num_write_latches - 1);
      write_latches[index] = wr_data;

      if (wr_adr >= prog_wp) {
        for (int i = 0; i < num_write_latches; i++) {
          if (write_latches[i] != LATCH_MT) {
            cpu->init_program_memory(cpu->map_pm_index2address(wr_adr + i), write_latches[i]);
            write_latches[i] = LATCH_MT;
          }
        }

      } else {
        printf("Warning: attempt to Write  protected Program memory 0x%x\n",
               wr_adr);
        write_error = true;
        gi.simulation_has_stopped();
      }

      break;

    case EECON1::EEPGD|EECON1::LWLO:	// write to latches
    case EECON1::CFGS|EECON1::LWLO:	// write to latches
    case EECON1::EEPGD|EECON1::CFGS|EECON1::LWLO:	// write to latches
      index = wr_adr & (num_write_latches - 1);
      write_latches[index] = wr_data;
      break;

    case EECON1::CFGS:			// write config word memory
    case EECON1::CFGS|EECON1::EEPGD:
      index = wr_adr & (num_write_latches - 1);
      wr_adr &= ~(num_write_latches - 1);
      write_latches[index] = wr_data;
      for (int i = 0; i < num_write_latches; i++) {
        if (write_latches[i] != LATCH_MT) { // was latch modified?
          unsigned int cfg_add = config_word_base | (wr_adr + i);
          index = cpu->get_config_index(cfg_add);
          if (index < 0) {
            printf("EEWRITE No config word at 0x%x\n", cfg_add);
            write_error = true;

          } else if (!static_cast<pic_processor*>(cpu)->getConfigMemory()->getConfigWord(index)->isEEWritable()) {
            printf("EEWRITE config word at 0x%x write protected\n", cfg_add);
            write_error = true;

          } else {
            Dprintf(("write config data cfg_add %x wr_data %x\n", cfg_add, wr_data));

            if (!cpu->set_config_word(cfg_add, wr_data)) {
              printf("EEWRITE unknown failure to write %x to 0x%x\n", wr_data, cfg_add);
              write_error = true;
            }
          }

          write_latches[i] = LATCH_MT;
        }
      }

      break;

    case EECON1::CFGS|EECON1::FREE:	// free Configuration memory row
    case EECON1::EEPGD|EECON1::CFGS|EECON1::FREE:	// free Configuration memory row
      // This row erase simply skips non-existant or write protected
      // configuration words
      wr_adr &= ~(erase_block_size - 1);

      for (int i = 0; i < erase_block_size; i++) {
        unsigned int cfg_add = config_word_base | (wr_adr + i);
        index = cpu->get_config_index(cfg_add);

        if (index >= 0 &&
            static_cast<pic_processor*>(cpu)->getConfigMemory()->getConfigWord(index)->isEEWritable()) {
          cpu->set_config_word(cfg_add, 0);
        }
      }

      break;

    case EECON1::EEPGD|EECON1::FREE:	// free program memory row
      wr_adr &= ~(erase_block_size - 1);

      if (wr_adr >= prog_wp) {
        for (int i = 0; i < erase_block_size; i++)
          //cpu->erase_program_memory(cpu->map_pm_index2address(wr_adr+i));
        {
          cpu->init_program_memory(cpu->map_pm_index2address(wr_adr + i), 0);
        }

      } else {
        printf("Warning: attempt to row erase protected Program memory\n");
        write_error = true;
        gi.simulation_has_stopped();
      }

      break;

    case EECON1::LWLO:	// LWLO ignored to eeprom
    default:		// write to eeprom
      if (wr_adr < rom_size) {
        rom[wr_adr]->value.put(wr_data);

      } else {
        std::cout << "EXTND_EEPROM write address is out of range " << std::hex << wr_adr << '\n';
        write_error = true;
      }

      break;
    }

    if (!write_error) {
      eecon1.value.put(eecon1.value.get()  & ~ eecon1.WRERR);
    }

    write_is_complete();

    if (eecon1.value.get() & eecon1.WREN) {
      eecon2.unready();

    } else {
      eecon2.unarm();
    }

    break;

  default:
    std::cout << "EEPROM_EXTND::callback() bad eeprom state " << eecon2.get_eestate() << '\n';
  }
}
