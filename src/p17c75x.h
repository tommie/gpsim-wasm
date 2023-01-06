/*
   Copyright (C) 1998 T. Scott Dattalo

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

#ifndef SRC_P17C75X_H_
#define SRC_P17C75X_H_

#include "16bit-processors.h"
#include "16bit-registers.h"
#include "pic-processor.h"

class Processor;

class P17C7xx : public  _16bit_processor {
public:
  P17C7xx();

  static Processor *construct(const char *name);
  PROCESSOR_TYPE isa() override
  {
    return _P17C7xx_;
  }
  void create_symbols() override;
  virtual void create(int ram_top);

  void create_sfr_map() override;
  unsigned int program_memory_size() const override
  {
    return 0x400;
  }

  CPUSTA cpusta;
};


class P17C75x : public P17C7xx {
public:
  P17C75x();

  static Processor *construct(const char *name);
  void create(int ram_top) override;
  void create_sfr_map() override;

  PROCESSOR_TYPE isa() override
  {
    return _P17C75x_;
  }
  void create_symbols() override;

  unsigned int program_memory_size() const override
  {
    return 0x4000;
  }
};


class P17C752 : public P17C75x {
public:
  P17C752();

  PROCESSOR_TYPE isa() override
  {
    return _P17C752_;
  }
  static Processor *construct(const char *name);
  void create();
  //  void create_sfr_map();

  void create_sfr_map() override;
  void create_symbols() override;
  unsigned int program_memory_size() const override
  {
    return 0x2000;
  }
  unsigned int register_memory_size() const override
  {
    return 0x800;
  }
};


class P17C756 : public P17C75x {
public:
  P17C756();

  PROCESSOR_TYPE isa() override
  {
    return _P17C756_;
  }
  void create_sfr_map() override;
  void create_symbols() override;
  static Processor *construct(const char *name);
  void create();

  unsigned int program_memory_size() const override
  {
    return 0x4000;
  }
  unsigned int register_memory_size() const override
  {
    return 0x800;
  }
};


class P17C756A : public P17C75x {
public:
  P17C756A();

  PROCESSOR_TYPE isa() override
  {
    return _P17C756A_;
  }
  void create_sfr_map() override;
  void create_symbols() override;
  static Processor *construct(const char *name);
  void create();

  unsigned int program_memory_size() const override
  {
    return 0x4000;
  }
  unsigned int register_memory_size() const override
  {
    return 0x800;
  }
};


class P17C762 : public P17C75x {
public:
  P17C762();

  PROCESSOR_TYPE isa() override
  {
    return _P17C762_;
  }
  void create_sfr_map() override;
  void create_symbols() override;

  static Processor *construct(const char *name);
  void create();

  unsigned int program_memory_size() const override
  {
    return 0x4000;
  }
  unsigned int register_memory_size() const override
  {
    return 0x800;
  }
};


class P17C766 : public P17C75x {
public:
  P17C766();

  PROCESSOR_TYPE isa() override
  {
    return _P17C766_;
  }
  void create_sfr_map() override;
  void create_symbols() override;
  static Processor *construct(const char *name);
  void create();

  unsigned int program_memory_size() const override
  {
    return 0x4000;
  }
  unsigned int register_memory_size() const override
  {
    return 0x800;
  }
};

#endif
