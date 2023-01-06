/*
   Copyright (C) 1998-2000 T. Scott Dattalo

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

#ifndef SRC_P16F87X_H_
#define SRC_P16F87X_H_

#include "p16x7x.h"

#include "eeprom.h"
#include "comparator.h"

class IOPORT;


class P16F871 : public P16C64 { // The 74 has too much RAM and too many CCPs
public:
  P16F871(const char *_name = nullptr, const char *desc = nullptr);
  ~P16F871();

  virtual void set_out_of_range_pm(unsigned int address, unsigned int value);

  virtual PROCESSOR_TYPE isa()
  {
    return _P16F871_;
  }
  virtual unsigned int program_memory_size() const
  {
    return 0x0800;
  }
  virtual unsigned int eeprom_memory_size() const
  {
    return 64;
  }
  virtual void create_symbols();
  void create_sfr_map();
  void create();
  virtual unsigned int register_memory_size() const
  {
    return 0x200;
  }

  static Processor *construct(const char *name);

  virtual void set_eeprom(EEPROM *)
  {
    // use set_eeprom_wide as P16F871 expect a wide EEPROM
    assert(0);
  }
  virtual void set_eeprom_wide(EEPROM_WIDE *ep)
  {
    eeprom = ep;
  }
  virtual EEPROM_WIDE *get_eeprom()
  {
    return ((EEPROM_WIDE *)eeprom);
  }

  // XXX
  // This pir1_2, pir2_2 stuff is not particularly pretty.  It would be
  // better to just tell C++ to redefine pir1 and pir2 and PIR1v2 and
  // PIR2v2, but C++ only supports covariance in member function return
  // values.
  PIR2v2 *pir2_2_reg;

  virtual PIR *get_pir2()
  {
    return pir2_2_reg;
  }

  ADCON0 adcon0;
  ADCON1 adcon1;
  sfr_register  adres;
  sfr_register  adresl;

  USART_MODULE usart;
};


class P16F873 : public P16C73 {
public:
  P16F873(const char *_name = nullptr, const char *desc = nullptr);
  ~P16F873();

  virtual void set_out_of_range_pm(unsigned int address, unsigned int value);

  virtual PROCESSOR_TYPE isa()
  {
    return _P16F873_;
  }
  virtual unsigned int program_memory_size() const
  {
    return 0x1000;
  }
  virtual void create_symbols();
  void create_sfr_map();
  void create();
  virtual unsigned int register_memory_size() const
  {
    return 0x200;
  }

  virtual void set_eeprom(EEPROM *)
  {
    // use set_eeprom_wide as P16F873 expect a wide EEPROM
    assert(0);
  }
  virtual unsigned int eeprom_memory_size() const
  {
    return 128;
  }
  virtual void set_eeprom_wide(EEPROM_WIDE *ep)
  {
    eeprom = ep;
  }
  virtual EEPROM_WIDE *get_eeprom()
  {
    return ((EEPROM_WIDE *)eeprom);
  }
  static Processor *construct(const char *name);

  sfr_register adresl;
};


class P16F873A : public P16F873 {
public:
  P16F873A(const char *_name = nullptr, const char *desc = nullptr);
  ~P16F873A();

  virtual PROCESSOR_TYPE isa()
  {
    return _P16F873A_;
  }

  void create_sfr_map();
  void create();
  static Processor *construct(const char *name);

  ComparatorModule comparator;
};


class P16F876 : public P16C73 {
public:
  P16F876(const char *_name = nullptr, const char *desc = nullptr);
  ~P16F876();

  virtual void set_out_of_range_pm(unsigned int address, unsigned int value);

  virtual PROCESSOR_TYPE isa()
  {
    return _P16F876_;
  }
  virtual unsigned int program_memory_size() const
  {
    return 0x2000;
  }
  virtual void create_symbols();
  void create_sfr_map();
  void create();
  virtual unsigned int register_memory_size() const
  {
    return 0x200;
  }

  static Processor *construct(const char *name);

  virtual void set_eeprom(EEPROM *)
  {
    // use set_eeprom_wide as P16F873 expect a wide EEPROM
    assert(0);
  }
  virtual unsigned int eeprom_memory_size() const
  {
    return 256;
  }
  virtual void set_eeprom_wide(EEPROM_WIDE *ep)
  {
    eeprom = ep;
  }
  virtual EEPROM_WIDE *get_eeprom()
  {
    return ((EEPROM_WIDE *)eeprom);
  }

  sfr_register adresl;
};


class P16F876A : public P16F873A {
public:
  P16F876A(const char *_name = nullptr, const char *desc = nullptr);
  ~P16F876A();

  virtual PROCESSOR_TYPE isa()
  {
    return _P16F876A_;
  }
  virtual unsigned int program_memory_size() const
  {
    return 0x2000;
  }
  virtual unsigned int eeprom_memory_size() const
  {
    return 256;
  }

  void create_sfr_map();
  void create();
  virtual unsigned int register_memory_size() const
  {
    return 0x200;
  }
  static Processor *construct(const char *name);

  ComparatorModule comparator;
};


class P16F874 : public P16C74 {
public:
  P16F874(const char *_name = nullptr, const char *desc = nullptr);
  ~P16F874();

  virtual void set_out_of_range_pm(unsigned int address, unsigned int value);

  virtual PROCESSOR_TYPE isa()
  {
    return _P16F874_;
  }
  virtual unsigned int program_memory_size() const
  {
    return 0x1000;
  }
  virtual void create_symbols();
  void create_sfr_map();
  void create();
  virtual unsigned int register_memory_size() const
  {
    return 0x200;
  }

  static Processor *construct(const char *name);

  virtual unsigned int eeprom_memory_size() const
  {
    return 128;
  }
  virtual void set_eeprom(EEPROM *)
  {
    // use set_eeprom_wide as P16F873 expect a wide EEPROM
    assert(0);
  }
  virtual void set_eeprom_wide(EEPROM_WIDE *ep)
  {
    eeprom = ep;
  }
  virtual EEPROM_WIDE *get_eeprom()
  {
    return (EEPROM_WIDE *)eeprom;
  }
  //virtual bool hasSSP() { return true;}

  ComparatorModule comparator;
  sfr_register adresl;
};


class P16F877 : public P16F874 {
public:
  P16F877(const char *_name = nullptr, const char *desc = nullptr);
  ~P16F877();

  virtual PROCESSOR_TYPE isa()
  {
    return _P16F877_;
  }
  virtual unsigned int program_memory_size() const
  {
    return 0x2000;
  }
  virtual unsigned int eeprom_memory_size() const
  {
    return 256;
  }
  virtual void create_symbols();
  void create_sfr_map();
  void create();
  static Processor *construct(const char *name);
};


class P16F874A : public P16F874 {
public:
  P16F874A(const char *_name = nullptr, const char *desc = nullptr);
  ~P16F874A();

  virtual void set_out_of_range_pm(unsigned int address, unsigned int value);

  virtual PROCESSOR_TYPE isa()
  {
    return _P16F874A_;
  }
  virtual unsigned int program_memory_size() const
  {
    return 0x1000;
  }
  virtual unsigned int eeprom_memory_size() const
  {
    return 256;
  }
  virtual void create_symbols();
  void create_sfr_map();
  void create();
  virtual unsigned int register_memory_size() const
  {
    return 0x200;
  }

  static Processor *construct(const char *name);

  ComparatorModule comparator;
};


class P16F877A : public P16F874A {
public:
  P16F877A(const char *_name = nullptr, const char *desc = nullptr);
  ~P16F877A();

  virtual PROCESSOR_TYPE isa()
  {
    return _P16F877A_;
  }
  virtual unsigned int program_memory_size() const
  {
    return 0x2000;
  }
  virtual unsigned int eeprom_memory_size() const
  {
    return 256;
  }
  virtual void create_symbols();
  void create_sfr_map();
  void create();
  static Processor *construct(const char *name);

  ComparatorModule comparator;
};


#endif
