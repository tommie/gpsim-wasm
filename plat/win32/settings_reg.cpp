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

#include <windows.h>

#include "settings_reg.h"


#define REG_PATH_PREFIX_STR "Software\\"


SettingsReg::SettingsReg(const char *appl_name)
{
  name = std::string(REG_PATH_PREFIX_STR) + appl_name + "\\";
}


bool SettingsReg::set(const char *module, const char *entry, const char *str)
{
  HKEY key;
  bool ret = true;
  DWORD disposition;
  std::string reg_path = name + module;

  if (RegCreateKeyEx(HKEY_CURRENT_USER, reg_path.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE,
    KEY_ALL_ACCESS, NULL, &key, &disposition) != ERROR_SUCCESS)
    return 0;

  if (RegSetValueEx(key, entry, 0, REG_SZ, (LPBYTE)str, strlen(str) + 1) != ERROR_SUCCESS)
    ret = 0;

  RegCloseKey(key);

  return ret;
}


bool SettingsReg::set(const char *module, const char *entry, int value)
{
  HKEY key;
  bool ret = true;
  DWORD disposition;
  std::string reg_path = name + module;

  if (RegCreateKeyEx(HKEY_CURRENT_USER, reg_path.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE,
    KEY_ALL_ACCESS, NULL, &key, &disposition) != ERROR_SUCCESS)
    return 0;

  if (RegSetValueEx(key, entry, 0, REG_BINARY, (LPBYTE)&value, sizeof value) != ERROR_SUCCESS)
    ret = 0;

  RegCloseKey(key);

  return ret;
}


bool SettingsReg::get(const char *module, const char *entry, char **str)
{
  static char buf[256];
  HKEY key;
  bool ret = true;
  DWORD size = sizeof buf;
  std::string reg_path = name + module;

  if (RegOpenKeyEx(HKEY_CURRENT_USER, reg_path.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS)
    return 0;

  if (RegQueryValueEx(key, entry, NULL, NULL, (LPBYTE)buf, &size) != ERROR_SUCCESS)
    ret = 0;
  else
    *str = buf;

  RegCloseKey(key);

  return ret;
}


bool SettingsReg::get(const char *module, const char *entry, int *value)
{
  HKEY key;
  bool ret = true;
  DWORD size = sizeof *value;
  std::string reg_path = name + module;

  if (RegOpenKeyEx(HKEY_CURRENT_USER, reg_path.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS)
    return 0;

  if (RegQueryValueEx(key, entry, NULL, NULL, (LPBYTE)value, &size) != ERROR_SUCCESS)
    ret = 0;

  RegCloseKey(key);

  return ret;
}

bool SettingsReg::remove(const char *module, const char *entry)
{
  HKEY key;
  bool ret = true;
  std::string reg_path = name + module;

  if (RegOpenKeyEx(HKEY_CURRENT_USER, reg_path.c_str(), 0,  KEY_SET_VALUE, &key) != ERROR_SUCCESS)
    return 0;

  if (RegDeleteValue(key, entry) != ERROR_SUCCESS)
    ret = 0;

  RegCloseKey(key);

  return ret;
}
