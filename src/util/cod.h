/*
   Copyright (C) 1999 James Bowman, Scott Dattalo
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

#ifndef SRC_UTIL_COD_H_
#define SRC_UTIL_COD_H_

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include "program.h"


namespace util {

struct CODLineSymbol {
  uint64_t addr;
  unsigned int file_index;
  int line;
};

struct CODDebugMessage {
  uint64_t addr;
  char cmd;
  std::string msg;
};

struct CODSymbol {
  SourceSymbolType type;
  int value;
  std::string name;
};

// A Byte Craft COD file, containing machine code, as created by
// gputils.
//
// Use open() to do some basic validation. The other non-const
// functions will run open() as needed.
class CODFileReader {
public:
  static int read_program(Program *out, std::istream *contents);

  CODFileReader() = default;
  CODFileReader(CODFileReader&&) = default;

  // Creates a reader with a borrowed stream pointer.
  explicit CODFileReader(std::istream *contents);

  // Performs some initial validation.
  int open();

  // Returns the targeted processor type name. Only valid after open()
  // was successful.
  std::string processor_type() const;

  // Reads actual machine code as a set of ranges.
  int read_code(std::vector<CodeRange> *buf);

  // Reads source file names, as references by
  // CODLineSymbol::file_index.
  int read_src_file_names(std::vector<std::string> *buf);

  // Reads the line number mapping.
  int read_line_numbers(std::vector<CODLineSymbol> *buf);

  // Reads debug messages (a.k.a. directives.)
  int read_debug_messages(std::vector<CODDebugMessage> *buf);

  // Reads data and program symbol names.
  int read_symbols(std::vector<CODSymbol> *buf);

  CODFileReader(const CODFileReader&) = delete;
  CODFileReader& operator =(const CODFileReader&) = delete;

private:
  int read_main_directory(std::vector<u8string> *buf);
  int check_for_gputils();
  int read_block(u8string *buf, int block_number);

private:
  std::istream *m_stream;
  std::vector<u8string> m_main_dir;
};


}  // namespace util

#endif
