/* -*- Mode: C++; c-file-style: "GNU"; comment-column: 40 -*- */
/*
   Copyright (C) 1998,1999 T. Scott Dattalo
   Copyright (C) 2022 Tommie Gannert

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

//
// cod.cc
//
//  The file contains the code for reading Byte Craft's .cod
// formatted symbol files.
//

#include "cod.h"

/*
 * .cod definitions
 *
 * A .cod file consists of an array of 512 byte blocks. There are two types
 * of blocks: a "directory" block and a "data" block. The directory block
 * describes the miscellaneous stuff like the compiler, the date, copy right
 * and it also describes the type of information that's available in the .cod
 * file. The "type of information" is specified by a range of blocks. For
 * example, if there are symbols in the .cod file then the directory block
 * tells the starting and ending blocks that contain the symbols.
 *
 * Types of data blocks:
 * short symbol table - a list of symbols in the "short format", which
 *     means that the symbol name is restricted to 12 characters. This
 *     is an old format and is not provided by gpasm.
 * long symbol table - a list of symbols in the "long format". Like the
 *     short symbol table except the symbol names can be up to 255 chars.
 * list table - a cross reference between the source line numbers, list
 *     line numbers, and the program memory.
 * Memory map table - describes the ranges of memory used in the processor.
 * Local variables table - [c files - not supported by gpasm] this describes
 *     the memory locations used by functions.
 * Source file names - a list of the files used to assemble the source file.
 * Debug messages - [not supported by gpasm] this provides a list of messages
 *     that can control the simulator or emulator.
 */

#define COD_BLOCK_BITS        9       /* COD_BLOCK_SIZE = 2^COD_BLOCK_BITS */
                                      /* number of bytes in one cod block */
#define COD_BLOCK_SIZE        (1<<COD_BLOCK_BITS)
#define COD_CODE_IMAGE_BLOCKS 128

/*
 * Here's a list of the offsets for the directory block. In each case the
 * offset is the number of bytes from the beginning of the block. Note that
 * it would be much more clever to alias a properly sized structure onto the
 * block. However, without using compiler dependent flags, it's not possible
 * to control how the data members of a structure are packed. Portability
 * has its costs.
 */

#define COD_DIR_CODE       0       /* code block indices are at the start */
#define COD_DIR_SOURCE     256     /* source file name */
#define COD_DIR_DATE       320     /* date .cod file was created */
#define COD_DIR_TIME       328     /* time .cod file was created */
#define COD_DIR_VERSION    330     /* Compiler version */
#define COD_DIR_COMPILER   350     /* Compiler name */
#define COD_DIR_NOTICE     363     /* Compiler copyright */
#define COD_DIR_SYMTAB     426     /* Start block of short symbol table */
#define COD_DIR_NAMTAB     430     /* Start block of file name table */
#define COD_DIR_LSTTAB     434     /* Start block of list file cross reference */
#define COD_DIR_ADDRSIZE   438     /* # of bytes for an address */
#define COD_DIR_HIGHADDR   439     /* High word of address for 64K Code block */
#define COD_DIR_NEXTDIR    441     /* Next directory block */
#define COD_DIR_MEMMAP     443     /* Start block of memory map */
#define COD_DIR_LOCALVAR   447     /* Start block of local variables */
#define COD_DIR_CODTYPE    451     /* Type of .cod file */
#define COD_DIR_PROCESSOR  453     /* Target processor */
#define COD_DIR_LSYMTAB    462     /* Start block of long symbol table */
#define COD_DIR_MESSTAB    466     /* Start block of debug message area */

/*
 * Here's a list of sizes of various objects in a .cod file.
 */
#define COD_DIR_COMPILER_SIZE      (COD_DIR_NOTICE - COD_DIR_COMPILER)
#define COD_DIR_PROCESSOR_SIZE     (COD_DIR_LSYMTAB - COD_DIR_PROCESSOR)
#define COD_FILE_SIZE               64      /* Length of filename strings */
#define COD_MAX_LINE_SYM            84      /* Number of source lines per cod block */
#define COD_LINE_SYM_SIZE            6      /* Line symbol structure size */
#define COD_DEBUG_MSG_MAX_SIZE     256
#define COD_LSYMBOL_NAME_MAX_SIZE  256

#define COD_LS_SFILE        0      /* offset of sfile in LineSymbol struct */
#define COD_LS_SMOD         1      /*  "        smod  " */
#define COD_LS_SLINE        2      /*  "        sline " */
#define COD_LS_SLOC         4      /*  "        sloc  " */

#define COD_LS_SMOD_FLAG_L  (1 << 2)
#define COD_LS_SMOD_FLAG_C1 (1 << 7)

#define COD_DEBUG_ADDR 0
#define COD_DEBUG_CMD  4
#define COD_DEBUG_MSG  5

/*
 * Symbol types
 */
#define COD_ST_C_SHORT       2
#define COD_ST_ADDRESS      46
#define COD_ST_CONSTANT     47

#ifndef NDEBUG
#include <iostream>
# define DRETURN(err, msg) do {                                 \
    const auto err_ = (err);                                    \
    std::cerr << "Err " << err_ << ": " << msg << std::endl;    \
    return err_;                                                \
  } while (0)
#else
# define DRETURN(err, msg) return (err)
#endif


namespace util {

namespace {

unsigned short get_short_int(const uint8_t *buff)
{
  return buff[0] | (buff[1] << 8);
}

unsigned int get_be_int(const uint8_t *buff)
{
  return buff[3] | (buff[2] << 8) | (buff[1] << 16) | (buff[0] << 24);
}

std::string get_pstring(const u8string &block, std::size_t pos, std::size_t size)
{
  int n = block[pos];

  if (1 + n > size || pos + 1 + n > block.size()) return "";

  return std::string(reinterpret_cast<const char*>(&block[pos + 1]), n);
}

SourceSymbolType as_symbol_type(int type) {
  switch (type) {
  case COD_ST_C_SHORT: return SourceSymbolType::DATA;
  case COD_ST_ADDRESS: return SourceSymbolType::PROGRAM;
  case COD_ST_CONSTANT: return SourceSymbolType::CONSTANT;
  default: return SourceSymbolType::UNKNOWN;
  }
}

}


int CODFileReader::read_program(Program *out, std::istream *contents)
{
  CODFileReader reader(contents);

  if (int err = reader.open(); err)
    return err;

  std::vector<CodeRange> code;
  if (int err = reader.read_code(&code); err)
    return err;

  std::vector<std::string> cod_fnames;
  if (int err = reader.read_src_file_names(&cod_fnames); err)
    return err;

  std::vector<CODLineSymbol> cod_linesyms;
  if (int err = reader.read_line_numbers(&cod_linesyms); err)
    return err;

  std::vector<CODDebugMessage> cod_msgs;
  if (int err = reader.read_debug_messages(&cod_msgs); err)
    return err;

  std::vector<CODSymbol> cod_syms;
  if (int err = reader.read_symbols(&cod_syms); err)
    return err;

  std::vector<SourceSymbol> syms;
  for (const auto &sym : cod_syms) {
    syms.push_back({
        .type = sym.type,
        .name = sym.name,
        .value = sym.value,
      });
  }

  std::vector<SourceLineRef> line_refs;
  for (const auto &ref : cod_linesyms) {
    if (ref.file_index >= cod_fnames.size())
      return EINVAL;

    line_refs.push_back({
        .addr = ref.addr,
        // Note this populates a string_view: the cod_fnames must be
        // in scope until Program().
        .file = cod_fnames[ref.file_index],
        .line = ref.line,
      });
  }

  std::vector<SourceDirective> directives;
  for (const auto &msg : cod_msgs) {
    directives.push_back({
        .addr = msg.addr,
        .type = std::string(1, msg.cmd),
        .text = msg.msg,
      });
  }

  *out = Program(std::move(code),
                 std::move(syms),
                 std::move(line_refs),
                 std::move(directives),
                 reader.processor_type());

  return 0;
}

CODFileReader::CODFileReader(std::istream *contents)
  : m_stream(contents) {}

int CODFileReader::open()
{
  if (!m_stream)
    return EINVAL;

  if (!m_main_dir.empty())
    return 0;

  if (int err = read_main_directory(&m_main_dir); err)
    return err;

  if (int err = check_for_gputils(); err) {
    return err;
  }

  return 0;
}

std::string CODFileReader::processor_type() const
{
  if (m_main_dir.empty()) std::abort();

  return get_pstring(m_main_dir[0], COD_DIR_PROCESSOR, COD_DIR_PROCESSOR_SIZE);
}

int CODFileReader::read_code(std::vector<CodeRange> *buf)
{
  if (int err = open(); err)
    return err;

  for (const auto &dir : m_main_dir) {
    int mmstarti = get_short_int(&dir[COD_DIR_MEMMAP]);
    int mmendi = get_short_int(&dir[COD_DIR_MEMMAP + 2]);

    if (mmstarti == 0) continue;
    if (mmendi < mmstarti) DRETURN(EINVAL, "invalid COD MEMMAP");

    uint64_t highaddr = static_cast<uint64_t>(get_short_int(&dir[COD_DIR_HIGHADDR])) << 15;

    u8string code;
    int prev_block_number = -1;

    for (; mmstarti <= mmendi; ++mmstarti) {
      u8string rblock;

      if (int err = read_block(&rblock, mmstarti); err)
        return err;

      for (int i = 0; i < COD_BLOCK_SIZE; i += 4) {
        int mstart = get_short_int(&rblock[i]);
        int mend = get_short_int(&rblock[i + 2]);

        if (mstart == mend) continue;
        if (mend < mstart || mend >= COD_BLOCK_SIZE * COD_CODE_IMAGE_BLOCKS)
          DRETURN(EINVAL, "invalid COD range: " << mstart << ", " << mend);

        int block_number = get_short_int(&dir[COD_DIR_CODE + mstart / COD_BLOCK_SIZE]);

        if (block_number != prev_block_number) {
          prev_block_number = block_number;
          if (int err = read_block(&code, block_number); err)
            return err;
        }

        buf->push_back({
            .addr = mstart + highaddr,
            .code = code.substr(mstart - mstart / COD_BLOCK_SIZE * COD_BLOCK_SIZE, mend - mstart + 1),
          });
      }
    }
  }

  return 0;
}

//-----------------------------------------------------------
int CODFileReader::read_src_file_names(std::vector<std::string> *buf)
{
  const int FILES_PER_BLOCK = COD_BLOCK_SIZE / COD_FILE_SIZE;

  if (int err = open(); err)
    return err;

  for (const auto &dir : m_main_dir) {
    int starti = get_short_int(&dir[COD_DIR_NAMTAB]);
    int endi = get_short_int(&dir[COD_DIR_NAMTAB + 2]);

    if (starti == 0) continue;
    if (endi < starti) DRETURN(EINVAL, "invalid COD NAMTAB");

    for (; starti <= endi; ++starti) {
      u8string nblock;

      if (int err = read_block(&nblock, starti); err)
        return err;

      for (int i = 0; i < FILES_PER_BLOCK; ++i) {
        std::string filenm = get_pstring(nblock, i * COD_FILE_SIZE, COD_FILE_SIZE);

        buf->push_back(filenm);
      }
    }
  }

  // Clean the tail up.
  while (!buf->empty() && buf->back().empty()) {
    buf->pop_back();
  }

  return 0;
}

//-----------------------------------------------------------
int CODFileReader::read_line_numbers(std::vector<CODLineSymbol> *buf)
{
  if (int err = open(); err)
    return err;

  for (const auto &dir : m_main_dir) {
    int starti = get_short_int(&dir[COD_DIR_LSTTAB]);
    int endi = get_short_int(&dir[COD_DIR_LSTTAB + 2]);

    if (starti == 0) continue;
    if (endi < starti) DRETURN(EINVAL, "invalid COD LSTTAB");

    uint64_t highaddr = static_cast<uint64_t>(get_short_int(&dir[COD_DIR_HIGHADDR])) << 15;

    // Loop through all of the .cod file blocks that contain line number info

    for (; starti <= endi; ++starti) {
      u8string lsblock;

      if (int err = read_block(&lsblock, starti); err)
        return err;

      for (int offset = 0; offset < COD_BLOCK_SIZE - (COD_LINE_SYM_SIZE -1); offset += COD_LINE_SYM_SIZE) {
        if (lsblock[offset + COD_LS_SMOD] & COD_LS_SMOD_FLAG_L)
          continue;

        int smod = lsblock[offset + COD_LS_SMOD] & 0xFF;

        if (smod == COD_LS_SMOD_FLAG_C1) {
          buf->push_back({
              .addr = highaddr + get_short_int(&lsblock[offset + COD_LS_SLOC]),
              .file_index = lsblock[offset + COD_LS_SFILE],
              .line = get_short_int(&lsblock[offset + COD_LS_SLINE]),
            });
        }
      }
    }
  }

  return 0;
}

//-----------------------------------------------------------
int CODFileReader::read_debug_messages(std::vector<CODDebugMessage> *buf)
{
  if (int err = open(); err)
    return err;

  for (const auto &dir : m_main_dir) {
    int starti = get_short_int(&dir[COD_DIR_MESSTAB]);
    int endi = get_short_int(&dir[COD_DIR_MESSTAB + 2]);

    if (starti == 0) continue;
    if (endi < starti) DRETURN(EINVAL, "invalid COD MESSTAB");

    for (; starti <= endi; ++starti) {
      u8string mblock;

      if (int err = read_block(&mblock, starti); err)
        return err;

      for (int i = 0; i < COD_BLOCK_SIZE - (6 - 1);) {
        uint64_t addr = get_be_int(&mblock[i]);
        i += 4;

        char cmd = mblock[i];
        ++i;

        if (cmd == '\0') break;

        std::string msg = get_pstring(mblock, i, COD_DEBUG_MSG_MAX_SIZE);
        i += 1 + mblock[i];

        // The lower case commands are user commands.  The upper case are
        // compiler or assembler generated.  This code makes no distinction
        // between them.
        //
        // a, A - assertion
        // c, C - gpsim command
        // e, E - directive
        // f, F - printf
        // l, L - log
        buf->push_back({
            .addr = addr,
            .cmd = cmd,
            .msg = msg,
          });
      }
    }
  }

  return 0;
}

//-----------------------------------------------------------
int CODFileReader::read_symbols(std::vector<CODSymbol> *buf)
{
  if (int err = open(); err)
    return err;

  for (const auto &dir : m_main_dir) {
    unsigned short starti = get_short_int(&dir[COD_DIR_LSYMTAB]);
    unsigned short endi = get_short_int(&dir[COD_DIR_LSYMTAB + 2]);

    if (starti == 0) continue;
    if (endi < starti) DRETURN(EINVAL, "invalid COD LSYMTAB");

    for (; starti <= endi; ++starti) {
      u8string sblock;

      if (int err = read_block(&sblock, starti); err)
        return err;

      for (int i = 0; i < COD_BLOCK_SIZE;) {
        std::string name = get_pstring(sblock, i, COD_LSYMBOL_NAME_MAX_SIZE);
        i += 1 + sblock[i];

        if (name.empty()) break;

        int type = get_short_int(&sblock[i]);
        i += 2;

        if (type > 128) type = COD_ST_CONSTANT;

        int value = get_be_int(&sblock[i]);
        i += 4;

        buf->push_back({
            .type = as_symbol_type(type),
            .value = value,
            .name = name,
          });
      }
    }
  }

  return 0;
}

int CODFileReader::read_main_directory(std::vector<u8string> *buf)
{
  int block_num = 0;

  do {
    auto &dir = buf->emplace_back();

    if (int err = read_block(&dir, block_num); err)
      return err;

    block_num = get_short_int(&dir[COD_DIR_NEXTDIR]);
  } while (block_num);

  return 0;
}

int CODFileReader::read_block(u8string *buf, int block_num)
{
  m_stream->seekg(block_num * COD_BLOCK_SIZE);
  if (!*m_stream) DRETURN(errno, "COD seekg failed");

  buf->resize(COD_BLOCK_SIZE);
  m_stream->read(reinterpret_cast<char*>(buf->data()), buf->size());
  if (!*m_stream) DRETURN(errno, "COD read failed");

  if (m_stream->gcount() != buf->size()) {
    DRETURN(EINVAL, "short COD read");
  }

  return 0;
}

int CODFileReader::check_for_gputils()
{
  // Some code may be specific to gputils, so refuse the file if this
  // was created by something else.

  if (m_main_dir[0][COD_DIR_CODTYPE] != 1) {
    DRETURN(EINVAL, "bad CODTYPE");
  }

  auto compiler = get_pstring(m_main_dir[0], COD_DIR_COMPILER, COD_DIR_COMPILER_SIZE);

  if (compiler != "gpasm" && compiler != "gplink") {
    DRETURN(EINVAL, "COD file not from gputils");
  }

  return 0;
}

}  // namespace util
