/*
   Copyright (C) 2004 Borut Razem

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
Boston, MA 02111-1307, USA. */

#ifndef _SETTINGS_REG_H
#define _SETTINGS_REG_H

#include "settings.h"

class SettingsReg: public Settings
{
public:
  SettingsReg(const char *appl_name);
  virtual bool set(const char *module, const char *entry, const char *str);
  virtual bool set(const char *module, const char *entry, int value);
  virtual bool get(const char *module, const char *entry, char **str);
  virtual bool get(const char *module, const char *entry, int *value);
  virtual bool remove(const char *module, const char *entry);
};

#endif  //_SETTINGS_REG_H
