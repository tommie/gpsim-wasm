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
#include <stdio.h>
#include <vector>

#include "command.h"
#include "cmd_dump.h"
#include "misc.h"

#include "../src/14bit-registers.h"
#include "../src/gpsim_object.h"
#include "../src/pic-processor.h"
#include "../src/interface.h"
#include "../src/eeprom.h"
#include "../src/i2c-ee.h"
#include "../src/processor.h"
#include "../src/registers.h"
#include "../src/symbol.h"

cmd_dump dump;

static cmd_options cmd_dump_options[] = {
  {"e", cmd_dump::DUMP_EEPROM,    OPT_TT_BITFLAG},
  {"r", cmd_dump::DUMP_RAM,       OPT_TT_BITFLAG},
  {"s", cmd_dump::DUMP_SFRS,      OPT_TT_BITFLAG},
  {nullptr, 0, 0}
};


cmd_dump::cmd_dump()
  : command("dump", "du")
{
  brief_doc = "Display either the RAM or EEPROM";
  long_doc = "dump [r | e [module_name [filename]] | s]\n"
             "\tdump r or dump with no options will display all of the file\n"
             "\t       registers and special function registers.\n"
             "\tdump e will display the contents of the EEPROM (if the pic\n"
             "\t       being simulated contains any)\n"
             "\tdump e module_name \n"
             "\t       Display the contents of an EEPROM module where module_name\n"
             "\t       can either be the name of a module or processor,\n"
             "\tdump e module_name filename \n"
             "\t       dumps the contents of an EEPROM module\n"
             "\t       to a file as Intel hex format. The 'load e' command \n"
             "\t       can load the file generated by this command.\n"
             "\tdump s will display only the special function registers.\n";
  op = cmd_dump_options;
}


#define REGISTERS_PER_ROW  16
#define SFR_COLUMNS         3
#define MAX_SFR_NAME       10

void cmd_dump::dump_sfrs()
{
  Processor * cpu = GetActiveCPU();
  unsigned int reg_size = cpu->register_size();
  unsigned int uToDisplayCount = 0;
  unsigned int uColumns = SFR_COLUMNS;
  std::vector<Register*> RegListToDisplay;
  unsigned int auTopRowIndex[SFR_COLUMNS];
  // Examine all registers this pic has to offer
  std::list <ProgramMemoryAccess *> :: iterator itPMA;
  itPMA = cpu->pma_context.begin();

  for (; itPMA != cpu->pma_context.end(); ++itPMA) {
    std::list<Register *>::iterator itReg;

    for (itReg = (*itPMA)->SpecialRegisters.begin();
         itReg != (*itPMA)->SpecialRegisters.end();
         ++itReg) {
      uToDisplayCount++;
      RegListToDisplay.push_back(*itReg);
    }
  }

  if (RegListToDisplay.size() == 0) {
    for (unsigned int i = 0; i < cpu->register_memory_size(); i++) {
      Register * pReg = cpu->registers[i];

      if (pReg->isa() == Register::SFR_REGISTER &&
          // Found an sfr. Display its contents only if not aliased
          // at some other address too.
          i == pReg->address) {
        uToDisplayCount++;
        RegListToDisplay.push_back(pReg);
      }
    }
  }

  //
  //  All this is so we can have the reg number sequence go
  //  from top to bottom of each column instead of across columns.
  unsigned int uMod = uToDisplayCount % uColumns;
  unsigned int uRowsPerColumn = uToDisplayCount / uColumns;
  auTopRowIndex[0] = 0;

  for (unsigned int i = 1; i < sizeof(auTopRowIndex) / sizeof(unsigned int);
       i++) {
    auTopRowIndex[i] = auTopRowIndex[i - 1] + uRowsPerColumn + (uMod > i ? 1 : 0);
  }

  uRowsPerColumn += (uMod == 0 ? 0 : 1);
  putchar('\n');
  unsigned int uRegCount = 0;

  for (unsigned int uRow = 0; uRow < uRowsPerColumn; uRow++) {
    for (unsigned int uColCurrent = 0; uColCurrent < uColumns; uColCurrent++) {
      unsigned int uIndex = auTopRowIndex[uColCurrent] + uRow;

      if (uRegCount > uToDisplayCount) {
        break;
      }

      //      printf("%03d ", uIndex); // used for testings
      uRegCount++;
      Register *pReg = RegListToDisplay[uIndex];
      printf("%03x %-7s = %0*x   ", pReg->address,
             pReg->name().c_str(), reg_size * 2,
             pReg->get_value());
    }

    putchar('\n');
  }
}


int cmd_dump::dump(int bit_flag, gpsimObject* module, const char * filename)
{
  Register **fr = nullptr;
  unsigned int mem_size = 0;
  int reg_size = 1;
  int address_off = 0x0000;
  char s1[256];
  pic_processor *pic;
  std::string symName;
  I2C_EE *eeprom;
  PromAddress *sym;
  FILE *fd = nullptr;
  int iReturn;

  if (bit_flag != DUMP_EEPROM) {
    printf("cmd_dump: invalid option\n");
    return 0;
  }

  module->name(s1, sizeof(s1));

  symName = s1;

  symName += ".eeprom";

  fprintf(stdout, "cmd_dump module=%s file=%s\n", s1, filename);

  if (filename && (fd = fopen(filename, "w")) == nullptr) {
    perror(filename);
    return 0;
  }

  if ((pic = dynamic_cast<pic_processor *>(module)) && pic->eeprom) {
    fr = pic->eeprom->get_rom();
    mem_size = pic->eeprom->get_rom_size();
    reg_size = pic->eeprom->register_size();
    address_off = 0x0000;

  } else if ((sym = dynamic_cast<PromAddress *>(globalSymbolTable().find(symName)))) {
    sym->get(eeprom);
    fr = eeprom->get_rom();
    mem_size = eeprom->get_rom_size();
    reg_size = eeprom->register_size();
    address_off = 0x0000;

  } else {
    std::cout << "*** Error cmd_dump module " << s1 << " not EEPROM\n";
    iReturn = 0;
  }

  if (fd) {
    if (reg_size == 1) {
      IntelHexProgramFileType::writeihex8(fr, mem_size, fd, address_off);
      iReturn = 1;

    } else {
      printf("cmd_dump: module EEPROM register size of %d not currently supported\n", reg_size);
      iReturn = 0;
    }

    fclose(fd);

  } else {
    gpsim_set_bulk_mode(1);
    dump_regs(fr, mem_size, reg_size, bit_flag);
    gpsim_set_bulk_mode(0);
    iReturn = 1;
  }

  return iReturn;
}


void cmd_dump::dump(int mem_type)
{
  unsigned int mem_size = 0;
  unsigned int reg_size = 1;
  Register **fr = nullptr;
  Processor *pCpu = GetActiveCPU(true);

  if (!pCpu) {
    return;
  }

  switch (mem_type) {
  case DUMP_EEPROM: {
    pic_processor *pic = dynamic_cast<pic_processor *>(pCpu);

    if (pic && pic->eeprom) {
      fr = pic->eeprom->get_rom();
      mem_size = pic->eeprom->get_rom_size();

    } else {
      return;
    }
  }
  break;

  case DUMP_RAM:
    mem_size = GetActiveCPU()->register_memory_size();
    reg_size = GetActiveCPU()->register_size();
    fr = GetActiveCPU()->registers;
    break;

  case DUMP_SFRS:
    dump_sfrs();
    putchar('\n');
    return;
  }

  if (mem_size == 0) {
    return;
  }

  gpsim_set_bulk_mode(1);
  dump_regs(fr, mem_size, reg_size, mem_type);

  // Now Dump the sfr's

  if (mem_type == DUMP_RAM) {
    dump_sfrs();
    pic_processor *pic = dynamic_cast<pic_processor *>(pCpu);

    if (pic) {
      printf("\n%s = %02x\n", pic->Wreg->name().c_str(), pic->Wreg->get_value());
    }

    printf("pc = 0x%x\n", GetActiveCPU()->pc->value);
  }

  gpsim_set_bulk_mode(0);
}


void cmd_dump::dump_regs(Register **fr, unsigned int mem_size, int reg_size, int /* mem_type */)
{
  unsigned int i, j, reg_num;
  unsigned int uRegPerRow = reg_size == 1 ? REGISTERS_PER_ROW : 8;
  unsigned int v;
  bool previous_row_is_invalid = false;

  if (reg_size == 1) {
    printf("      ");

    // Column labels
    for (i = 0; i < uRegPerRow; i++) {
      printf(" %0*x", reg_size * 2, i);
    }

    putchar('\n');
  }

  reg_num = 0;

  for (i = 0; i < mem_size; i += uRegPerRow) {
    /* First, see if there are any valid registers on this row */
    bool all_invalid = true;

    for (j = 0; j < uRegPerRow; j++) {
      if (fr[i + j]->isa() != Register::INVALID_REGISTER) {
        all_invalid = false;
        break;
      }
    }

    if (!all_invalid) {
      previous_row_is_invalid = false;
      printf("%04x:  ", i);

      for (j = 0; j < uRegPerRow; j++) {
        reg_num = i  + j;

        if ((reg_num < mem_size) &&
            fr[reg_num] &&
            fr[reg_num]->isa() != Register::INVALID_REGISTER) {
          v = fr[reg_num]->get_value();
          printf("%0*x ", reg_size * 2, v);

        } else {
          for (int i = 0; i < reg_size; i++) {
            printf("--");
          }

          putchar(' ');
        }
      }

      if (reg_size == 1) {
        // don't bother with ASCII for > 8 bit registers
        printf("   ");

        for (j = 0; j < uRegPerRow; j++) {
          reg_num = i + j;
          v = fr[reg_num]->get_value();

          if ((v >= ' ') && (v <= 'z')) {
            putchar(v);

          } else {
            putchar('.');
          }
        }
      }

      putchar('\n');

    } else {
      if (!previous_row_is_invalid) {
        putchar('\n');
      }

      previous_row_is_invalid = true;
      reg_num += REGISTERS_PER_ROW;
    }
  }
}
