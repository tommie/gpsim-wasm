/*
   Copyright (C) 1998,1999,2000,2001
   T. Scott Dattalo and Ralf Forsberg

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

#include "settings_exdbm.h"
#include <cstdlib>
#include <cstdio>
#include <string>

extern "C" {
#include "../eXdbm/eXdbm.h"
#include "../eXdbm/eXdbmErrors.h"
}

SettingsEXdbm::SettingsEXdbm(const char *appl_name)
{
  int ret;
  const char *homedir;
  std::string path;

  ret = eXdbmInit();
  if (ret == -1)
  {
    puts(eXdbmGetErrorString(eXdbmGetLastError()));
  }

  homedir = getenv("HOME");
  if (!homedir)
    homedir = ".";

  path = std::string(homedir) + "/." + appl_name;

  ret = eXdbmOpenDatabase((char *)path.c_str(), &dbid);
  if (ret == -1)
  {
    int error = eXdbmGetLastError();
    if (error == DBM_OPEN_FILE)
    {
      ret = eXdbmNewDatabase((char *)path.c_str(), &dbid);
      if (ret == -1)
        puts(eXdbmGetErrorString(eXdbmGetLastError()));
      else
      {
        ret = eXdbmUpdateDatabase(dbid);
        if(ret == -1)
          puts(eXdbmGetErrorString(eXdbmGetLastError()));
      }
    }
    else
      puts(eXdbmGetErrorString(eXdbmGetLastError()));
  }
}


bool SettingsEXdbm::set(const char *module, const char *entry, const char *str)
{
  int ret;
  DB_LIST list;

  list = eXdbmGetList(dbid, 0, (char *)module);
  if (!list)
  {
    ret = eXdbmCreateList(dbid, 0, (char *)module, 0);
    if (ret == -1)
    {
      puts(eXdbmGetErrorString(eXdbmGetLastError()));
      return false;
    }

    list = eXdbmGetList(dbid, 0, (char *)module);
    if (!list)
    {
      puts(eXdbmGetErrorString(eXdbmGetLastError()));
      return false;
    }
  }

  // We have the list
  ret = eXdbmChangeVarString(dbid, list, (char *)entry, (char *)str);
  if (ret == -1)
  {
    ret = eXdbmCreateVarString(dbid, list, (char *)entry, 0, (char *)str);
    if (ret == -1)
    {
      puts("\n\n\n\ndidn't work");
      puts(eXdbmGetErrorString(eXdbmGetLastError()));
      puts("\n\n\n\n");
      return false;
    }
  }
  ret = eXdbmUpdateDatabase(dbid);
  if (ret == -1)
  {
    puts(eXdbmGetErrorString(eXdbmGetLastError()));
    return false;
  }

  return true;
}


bool SettingsEXdbm::set(const char *module, const char *entry, int value)
{
  int ret;
  DB_LIST list;

  if (!module || !entry)
    return false;

  list = eXdbmGetList(dbid, 0, (char *)module);
  if (!list)
  {
    ret = eXdbmCreateList(dbid, 0, (char *)module, 0);
    if (ret == -1)
    {
      puts(eXdbmGetErrorString(eXdbmGetLastError()));
      return false;
    }

    list = eXdbmGetList(dbid, 0, (char *)module);
    if (!list)
    {
      puts(eXdbmGetErrorString(eXdbmGetLastError()));
      return false;
    }
  }

  // We have the list
  ret = eXdbmChangeVarInt(dbid, list, (char *)entry, value);
  if (ret == -1)
  {
    ret = eXdbmCreateVarInt(dbid, list, (char *)entry, 0, value);
    if (ret == -1)
    {
      puts("\n\n\n\ndidn't work");
      puts(eXdbmGetErrorString(eXdbmGetLastError()));
      puts("\n\n\n\n");
      return false;
    }
  }
  ret = eXdbmUpdateDatabase(dbid);
  if (ret == -1)
  {
    puts(eXdbmGetErrorString(eXdbmGetLastError()));
    return false;
  }

  return true;
}


bool SettingsEXdbm::get(const char *module, const char *entry, char **str)
{
  int ret;
  DB_LIST list;

  list = eXdbmGetList(dbid, 0, (char *)module);
  if (!list)
    return false;

  // We have the list
  ret = eXdbmGetVarString(dbid, list, (char *)entry, str);
  if (ret == -1)
    return false;

  return true;
}


bool SettingsEXdbm::get(const char *module, const char *entry, int *value)
{
  int ret;
  DB_LIST list;

  list = eXdbmGetList(dbid, 0, (char *)module);
  if (!list)
    return false;

  // We have the list
  ret = eXdbmGetVarInt(dbid, list, (char *)entry, value);
  if (ret == -1)
    return false;

  return true;
}

bool SettingsEXdbm::remove(const char *module, const char *entry)
{
  int ret;
  DB_LIST list;

  list = eXdbmGetList(dbid, 0, (char *)module);
  if (!list)
  {
    ret = eXdbmCreateList(dbid, 0, (char *)module, 0);
    if(ret == -1)
    {
      puts(eXdbmGetErrorString(eXdbmGetLastError()));
      return false;
    }

    list = eXdbmGetList(dbid, 0, (char *)module);
    if (!list)
    {
      puts(eXdbmGetErrorString(eXdbmGetLastError()));
      return false;
    }
  }

  // We have the list
  ret = eXdbmDeleteEntry(dbid, list, (char *)entry);
  if (ret == -1)
  {
    return false;
  }
  ret = eXdbmUpdateDatabase(dbid);
  if (ret == -1)
  {
    puts(eXdbmGetErrorString(eXdbmGetLastError()));
    return false;
  }

  return true;
}


