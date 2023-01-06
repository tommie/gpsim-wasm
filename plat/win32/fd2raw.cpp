/*
   Copyright (C) 2004-2006 Borut Razem

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
#include <io.h>
#include <fcntl.h>

#include "fd2raw.h"


#if defined _MSC_VER && _MSC_VER >= 1300
/*
 * a hack to call _setmode() from MSVCRT.DLL:
 * glib runtime uses MSVCRT.DLL, while gpsim executable uses MSVCR71.DLL.
 * To set the mode of file handle, which is used in glib, the _setmode()
 * from MSVCRT.DLL should be called.
 */

static int win32_setmode(int fd, int mode)
{
  int ret = -1;
  HINSTANCE hinst;

  /* get a handle to the DLL module */
  hinst = LoadLibrary("MSVCRT.DLL");

  /* if the handle is valid, try to get the function address */
  if (NULL == hinst)
    {
      gchar *emsg = g_win32_error_message(GetLastError());
      g_error("Error loading library MSVCRT.DLL: %s", emsg);
      g_free(emsg);
    }
  else
    {
      int (*proc_add)(int, int);
      proc_add = (int (*)(int, int)) GetProcAddress(hinst, "_setmode");

      /* if the function address is valid, call the function */
      if (NULL == proc_add)
        {
          gchar *emsg = g_win32_error_message(GetLastError());
          g_error("Error retriving address of function _setmode: %s", emsg);
          g_free(emsg);
        }
      else
        ret = (proc_add) (fd, mode);

      /* free the DLL module */
      if (0 == FreeLibrary(hinst))
        {
          gchar *emsg = g_win32_error_message(GetLastError());
          g_error("Error freeing the DLL module: %s", emsg);
          g_free(emsg);
        }
    }

  return ret;
}
#endif


/*
 * set file descriptor to raw mode
 */

bool win32_fd_to_raw(int fd)
{
  HANDLE handle = (HANDLE)_get_osfhandle(fd);

  /* if a console */
  if (0 != (GetFileType(handle) & FILE_TYPE_CHAR))
    {
      DWORD orig_mode;

      /* set it to raw mode */
      GetConsoleMode(handle, &orig_mode);
      SetConsoleMode(handle, orig_mode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT));
#if defined _MSC_VER && _MSC_VER >= 1300
      win32_setmode(0, _O_BINARY);
#else
      setmode(0, _O_BINARY);
#endif

      return true;
    }

  return false;
}

