/*
   Copyright (C) 2006 T. Scott Dattalo

This file is part of the libgpsim_modules library of gpsim

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

#ifndef MODULES_TTL_H_
#define MODULES_TTL_H_

#include "../src/modules.h"
#include "../src/trigger.h"

class IOPIN;
class IO_bi_directional;

namespace TTL {
//------------------------------------------------------------------------
//
//  TTL devices
//


class TTLbase : public Module {
public:
  TTLbase(const char *_name, const char *desc);
  ~TTLbase();

  virtual void setClock(bool) {}
  virtual void setStrobe(bool) {}
  virtual void setEnable(bool) {}
  virtual void setReset(bool) {}
  //virtual void create_iopin_map();
  virtual void update_state() = 0;
  //GtkWidget *create_pixmap(char **pixmap_data);

protected:
  double  m_dVdd;
  bool    m_bClock;
  bool    m_bEnable;
};


class Clock;
class Strobe;
class Enable;
class Reset;

class TTL377 : public TTLbase {
public:
  explicit TTL377(const char *_name);
  ~TTL377();

  static Module *construct(const char *new_name = nullptr);

  virtual void create_iopin_map();
  void setClock(bool) override;
  void setEnable(bool) override;
  void update_state() override;

protected:
  Clock  *m_clock;
  Enable *m_enable;
  IOPIN **m_D;
  IO_bi_directional **m_Q;
};


class TTL595 : public TTLbase, public TriggerObject {
public:
  explicit TTL595(const char *_name);
  ~TTL595();

  static Module *construct(const char *new_name = nullptr);

  virtual void create_iopin_map();
  void setClock(bool) override;
  void setStrobe(bool) override;
  void setEnable(bool) override;
  void setReset(bool) override;
  void update_state() override;
  void callback() override;
  void callback_print() override;

protected:
  bool    m_bStrobe;
  Clock  *m_clock;
  Strobe *m_strobe;
  Reset  *m_reset;
  Enable *m_enable;
  IOPIN  *m_Ds;
  IOPIN  *m_Qs;
  IO_bi_directional **m_Q;
  unsigned short sreg;
};


class TTL165 : public TTLbase {
public:
  explicit TTL165(const char *_name);
  ~TTL165();

  static Module *construct(const char *new_name = nullptr);

  virtual void create_iopin_map();
  void setClock(bool) override;
  void setStrobe(bool) override;
  void setEnable(bool) override;
  void update_state() override;

protected:
  bool    m_bStrobe;
  Clock  *m_clock;
  Strobe *m_strobe;
  Enable *m_enable;
  IOPIN  *m_Ds;
  IOPIN  *m_Q;
  IOPIN  *m_Qbar;
  IOPIN  **m_D;
  unsigned short sreg;
};


} // end of namespace TTL

#endif // MODULES_TTL_H_
