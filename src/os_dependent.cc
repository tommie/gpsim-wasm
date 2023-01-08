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


/* os_dependent.cc  -                   */
/* version 0.1                          */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <list>
#include <string>

#include "exports.h"
#include "modules.h"

#ifdef _WIN32

#include <glib.h>
#include <glib/gwin32.h>
#include <direct.h>
#include <windows.h>

#else

#include <dlfcn.h>

#endif	// _WIN32

#ifdef _WIN32
#define MODULE_EXT ".dll"
#define FOLDERDELIMITER '\\'
#define FOLDERDELIMITERALTERNATIVE '/'
#define PATHDELIMITER ";"
#else
#ifdef __APPLE__
#define MODULE_EXT ".dylib"
#else
#define MODULE_EXT ".so"
#endif
#define MODULE_VER ".0"
#define FOLDERDELIMITER '/'
#define FOLDERDELIMITERALTERNATIVE '\\'
#define PATHDELIMITER ":"
#endif

#ifdef _WIN32
#define OS_E_FILENOTFOUND 0x0000007E
#define OS_E_MEMORYACCESS 0x000003E6
#else
// JRH - just made a guess
#define OS_E_FILENOTFOUND ENOENT
#define OS_E_MEMORYACCESS EADDRNOTAVAIL
#include <errno.h>
#endif

//------------------------------------------------------------------------
// Convert forward slashes and backslashes into the os-dependent
// slash
static void translatePath(std::string &sPath)
{
  std::replace(sPath.begin(), sPath.end(), FOLDERDELIMITERALTERNATIVE, FOLDERDELIMITER);
}


static CFileSearchPath asDllSearchPath;

static void addPathFromFilePath(const std::string &sFolder, std::string &sFile)
{
  auto LastDelimiter = sFolder.find_last_of(FOLDERDELIMITER);

  if (LastDelimiter == std::string::npos) {
    sFile = sFolder;

  } else {
    std::string sNewFolder = sFolder.substr(0, LastDelimiter + 1);
    sFile = sFolder.substr(LastDelimiter + 1);
    auto it = find(asDllSearchPath.begin(),
                       asDllSearchPath.end(), sNewFolder);

    if (it == asDllSearchPath.end()) {
      asDllSearchPath.insert(asDllSearchPath.begin(), sNewFolder);
    }
  }
}

#if defined(_DEBUG)
static bool bAltPaths = false;
#endif

void AddModulePathFromFilePath(char *arg)
{
  std::string sFile;
  std::string sFolder = arg;
  addPathFromFilePath(sFolder, sFile);
  char * pszGpsimModulePath = getenv("GPSIM_MODULE_PATH");

  if (pszGpsimModulePath) {
    char * pLast = pszGpsimModulePath;
    char * pChar = strchr(pszGpsimModulePath, PATHDELIMITER[0]);
    std::string folder;

    while (true) {
      if (pChar) {
        *pChar = '\0';
      }

      if (*pLast != '\0') {
        // only add non empty folders
        folder = pLast;
        translatePath(folder);

        if (folder.back() != FOLDERDELIMITER) {
          folder.push_back(FOLDERDELIMITER);
        }

        asDllSearchPath.push_back(folder);
      }

      if (pChar == nullptr) {
        break;
      }

      pChar++;
      pLast = pChar;
      pChar = strchr(pChar, PATHDELIMITER[0]);
    }
  }

#if defined(_DEBUG)

  if (!bAltPaths) {
    bAltPaths = true;
    std::string sPath;
    int iPos = sFolder.find_last_of(FOLDERDELIMITER);

    if (iPos != std::string::npos) {
      char szLine[1024];
      sPath = sFolder.substr(0, iPos + 1);
      sPath.append("altpaths.txt");
      FILE *pFile = fopen(sPath.c_str(), "r");

      if (pFile) {
        while (fgets(szLine, sizeof(szLine), pFile) != nullptr) {
          char *pChar = &szLine[strlen(szLine) - 1];

          while ((*pChar == '\n') && pChar != szLine) {
            *pChar-- = 0;
          }

          if (*pChar != FOLDERDELIMITER) {
            pChar++;
            *pChar = FOLDERDELIMITER;
            pChar++;
            *pChar = 0;
          }

          asDllSearchPath.push_back(std::string(szLine));
        }

        fclose(pFile);
      }
    }
  }

#endif
}


//---------------------------
//OS agnostic library loader

static void * sLoad(const std::string & library_name)
{
  void *handle;
#ifdef _WIN32
  handle = (void *)LoadLibrary((LPCSTR)library_name.c_str());
#else
  // According to the man page for dlopen, the RTLD_GLOBAL flag can
  // be or'd with the second pararmeter of the function call. However,
  // under Linux at least, this apparently cause *all* symbols to
  // disappear.
  handle = dlopen(library_name.c_str(), RTLD_NOW);  // | RTLD_GLOBAL);
#endif
  return handle;
}


void FixupLibraryName(std::string &sPath)
{
  translatePath(sPath);
}


void GetFileName(const std::string &sPath, std::string &sName)
{
  std::string::size_type pos = sPath.find_last_of(FOLDERDELIMITER);

  if (pos != std::string::npos) {
    sName = sPath.substr(pos + 1);

  } else if (&sName != &sPath) {
    sName = sPath;
  }
}


void GetFileNameBase(const std::string &sPath, std::string &sName)
{
  GetFileName(sPath, sName);
  std::string::size_type pos = sName.find_last_of('.');

  if (pos != std::string::npos) {
    sName = sName.substr(0, sName.size() - pos + 1);

  } else {
    sName = sPath;
  }
}


static const char * get_error_message()
{
#ifdef _WIN32
  return g_win32_error_message(GetLastError());
#else
  return dlerror();
#endif
}


void free_error_message(const char * pszError)
{
  (void)pszError;
#ifdef _WIN32
  g_free((char *) pszError);
#endif
}


static unsigned long get_error(const char *err_str)
{
#ifdef _WIN32
  return GetLastError();
#else
  /*
  ** In Linux and likely all Unix like OSs, dlopen leaves errno as 0
  ** even after failure, If so, look in error string returned by dlerror
  ** to try to determine if file was not found. RRR
  */
  unsigned long ret = errno;	// in Linux errno is 0

  if (! ret && err_str && strstr(err_str, "No such file")) {
    ret = OS_E_FILENOTFOUND;
  }

  return ret;
#endif
}


void * load_library(const char *library_name, const char **pszError)
{
  std::string sFile;
  std::string sPath(library_name);
  FixupLibraryName(sPath);
  addPathFromFilePath(sPath, sFile);

  // loop twice: first time take the file name as is,
  // the second time append the os-dependent library extension.
  for (int i = 0; i < 2; i++) {
    void *handle;
    // First, see if we can load the library from where ever the
    // system thinks libraries are located.
    if ((handle = sLoad(sPath)) != 0) {
      return handle;
    }

    *pszError = get_error_message();
    unsigned long uError = get_error(*pszError);

    if (uError == OS_E_FILENOTFOUND) {
      // Failed to find the library in the system paths, so try to load
      // from one of our paths.
      free_error_message(*pszError);

      for (const auto &SearchPath : asDllSearchPath) {
        sPath = SearchPath + sFile;
        handle = sLoad(sPath);

        if (handle) {
          return handle;
        }

        *pszError = get_error_message();
      }
    }

    // Append the module extension and try again.
    std::string::size_type nPos = sFile.find(MODULE_EXT, 0);

    if (nPos == std::string::npos) {
      sFile.append(MODULE_EXT);
    }

#ifndef _WIN32

    else if (sFile.find(MODULE_VER, nPos) == std::string::npos) {
      i--;
      sFile.append(MODULE_VER);
    }

#endif
    sPath = sFile;
  }

  // Should there be a free?
  if (*pszError)
    printf("Failed loading %s: %s\n", sPath.c_str(), *pszError);

  return nullptr;
}


void free_library(void *handle)
{
#ifdef _WIN32
  FreeLibrary((HMODULE)handle);
#else
  dlclose(handle);
#endif
}


void * get_library_export(const char *name, void *library_handle, const char **pszError)
{
  void * pExport;
#ifdef _WIN32
  pExport = (void*)GetProcAddress((HMODULE)library_handle, name);
#else
  dlerror();	// Clear possible previous errors
  pExport = dlsym(library_handle, name);
#endif

  if (!pExport && pszError) {
    *pszError = get_error_message();
  }

  return pExport;
}
