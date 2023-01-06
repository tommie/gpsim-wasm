/* .cod file support
   Copyright (C) 1999 James Bowman, Scott Dattalo

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


#ifndef SRC_COD_H_
#define SRC_COD_H_

#include "program_files.h"

#include <stdio.h>
#include <string>

class Processor;

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

#define COD_BLOCK_BITS     9       /* COD_BLOCK_SIZE = 2^COD_BLOCK_BITS */
                                   /* number of bytes in one cod block */
#define COD_BLOCK_SIZE     (1<<COD_BLOCK_BITS)

/*
 * Here's a list of the offsets for the directory block. In each case the
 * offset is the number of bytes from the beginning of the block. Note that
 * it would be much more clever to alias a properly sized structure onto the
 * block. However, without using compiler dependent flags, it's not possible
 * to control how the data members of a structure are packed. Portability
 * has its costs.
 */

#define COD_DIR_CODE       0       /* code block indices are at the start */
#define COD_DIR_SOURCE     257     /* source file name */
#define COD_DIR_DATE       321     /* date .cod file was created */
#define COD_DIR_TIME       328     /* time .cod file was created */
#define COD_DIR_VERSION    331     /* Compiler version */
#define COD_DIR_COMPILER   351     /* Compiler name */
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
#define COD_DIR_PROCESSOR  454     /* Target processor */
#define COD_DIR_LSYMTAB    462     /* Start block of long symbol table */
#define COD_DIR_MESSTAB    466     /* Start block of debug message area */

/*
 * Here's a list of sizes of various objects in a .cod file.
 */
#define COD_FILE_SIZE      64      /* Length of filename strings */
#define COD_MAX_LINE_SYM   84      /* Number of source lines per cod block */
#define COD_LINE_SYM_SIZE   6      /* Line symbol structure size */

enum cod_block_types {
  cb_nobody,
  cb_dir,
  cb_file,
  cb_list,
  cb_ssymbols,
  cb_lsymbols,
  cb_code
};

#define COD_LS_SFILE        0      /* offset of sfile in LineSymbol struct */
#define COD_LS_SMOD         1      /*  "        smod  " */
#define COD_LS_SLINE        2      /*  "        sline " */
#define COD_LS_SLOC         4      /*  "        sloc  " */
#define COD_LS_SIZE         6      /* size of LineSymbol struct */

#define COD_CODE_IMAGE_BLOCKS 128  /* Max # of blocks for the opcodes */

typedef struct block_struct {
  char *block;
  int  block_number;
} Block;

typedef struct dir_block_info {
  Block dir;
  Block cod_image_blocks[COD_CODE_IMAGE_BLOCKS];
  struct dir_block_info *next_dir_block_info;
} DirBlockInfo;

/*
 * Symbol types
 */
#define COD_ST_C_SHORT       2
#define COD_ST_ADDRESS      46
#define COD_ST_CONSTANT     47

class PicCodProgramFileType : public ProgramFileType {
private:
  // PicHexProgramFileType member functions
#if 0
  void    display_symbol_file_error(int err);

  FILE *  open_a_file(char **filename);
#endif
  void    set_lstname(const char *filename);

  int     read_directory();
  int     check_for_gputils(const char *block);
  void    read_hex_from_cod(Processor *cpu);
  void    read_line_numbers_from_cod(Processor *cpu);
  void    read_message_area(Processor *cpu);
  void    delete_directory();
  int     read_src_files_from_cod(Processor *cpu);

  int     get_string(char *dest, const char *src, size_t len);
  int    read_block(char * block, int block_number);

  void    read_symbols(Processor *cpu);
  void    read_hll_line_numbers_from_asm(Processor *cpu);


  FILE *codefile;
  char *temp_block;
  std::string lstfilename;

  DirBlockInfo main_dir;

  // Define a flag that tells whether or not we should care about the
  // case of text strings in the .cod file.
  int ignore_case_in_cod;

  int gputils_recent;

public:
  PicCodProgramFileType();
  // ProgramFileType overrides
  int LoadProgramFile(Processor **pProcessor, const char *pFilename,
                      FILE *pFile, const char *pProcessorName) override;
};

#endif
