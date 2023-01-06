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

#ifndef GUI_SETTINGS_EXDBM_H_
#define GUI_SETTINGS_EXDBM_H_

extern "C" {
#include "../eXdbm/eXdbmTypes.h"
}
#include "settings.h"

class SettingsEXdbm: public Settings
{
public:
  explicit SettingsEXdbm(const char *appl_name);

  bool set(const char *module, const char *entry, const char *str) override;
  bool set(const char *module, const char *entry, int value) override;
  bool get(const char *module, const char *entry, char **str) override;
  bool get(const char *module, const char *entry, int *value) override;
  bool remove(const char *module, const char *entry) override;

private:
  DB_ID dbid;
};

#endif  // GUI_SETTINGS_EXDBM_H_
