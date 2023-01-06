/*
   Copyright (C) 2003-2004 Borut Razem

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

/*
Minimal implementation of glib:
only slist functionality and g_win32_error_message
is needed by gpsim with --disable-gui
*/

#include "glib.h"
#include <windows.h>

gpointer
g_malloc0 (gulong n_bytes)
{
  if (n_bytes)
    {
      return calloc (1, n_bytes);
    }

  return NULL;
}


void
g_free (gpointer mem)
{
  if (mem)
    free (mem);
}


#define _g_slist_alloc g_slist_alloc 
GSList*
g_slist_alloc (void)
{
  GSList *list;

  list = g_new0 (GSList, 1);

  return list;
}


void
g_slist_free_1 (GSList *list)
{
  g_free (list);
}


GSList*
g_slist_append (GSList   *list,
                gpointer  data)
{
  GSList *new_list;
  GSList *last;

  new_list = _g_slist_alloc ();
  new_list->data = data;

  if (list)
    {
      last = g_slist_last (list);
      /* g_assert (last != NULL); */
      last->next = new_list;

      return list;
    }
  else
      return new_list;
}


GSList*
g_slist_remove (GSList        *list,
                gconstpointer  data)
{
  GSList *tmp, *prev = NULL;

  tmp = list;
  while (tmp)
    {
      if (tmp->data == data)
        {
          if (prev)
            prev->next = tmp->next;
          else
            list = tmp->next;

          g_slist_free_1 (tmp);
          break;
        }
      prev = tmp;
      tmp = prev->next;
    }

  return list;
}


GSList*
g_slist_last (GSList *list)
{
  if (list)
    {
      while (list->next)
        list = list->next;
    }

  return list;
}


gchar *
g_win32_error_message (gint error)
{
  gchar *msg;
  gchar *retval;
  int nbytes;

  FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER
                 |FORMAT_MESSAGE_IGNORE_INSERTS
                 |FORMAT_MESSAGE_FROM_SYSTEM,
                 NULL, error, 0,
                 (LPTSTR) &msg, 0, NULL);
  nbytes = strlen (msg);

  if (nbytes > 2 && msg[nbytes-1] == '\n' && msg[nbytes-2] == '\r')
    msg[nbytes-2] = '\0';

  retval = strdup (msg);

  if (msg != NULL)
    LocalFree (msg);

  return retval;
}
