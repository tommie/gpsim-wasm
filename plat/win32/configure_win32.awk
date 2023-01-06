#!/usr/bin/awk

# configure_win32.awk - Genarate config.h using config_win32.h.in as template
#                       and insert the version number definitions from configure.ac
#
# Written By - Borut Razem borut.razem@siol.net
#
# This file is part of gpsim.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2, or (at your option) any
# later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

BEGIN {
  # get the values from configure.ac

  while (getline <"configure.ac" > 0) {
    if ($0 ~ "^AC_INIT\\(.*\\)") {
      package = gensub("^AC_INIT\\(\\[([^]]*)\\].*","\\1","1", $0);
      version = gensub("^AC_INIT\\(\\[[^]]*\\],\\[([^]]*)\\].*","\\1", "1", $0);
      bugreport = gensub("^AC_INIT\\(\\[[^]]*\\],\\[[^]]*\\],\\[([^]]*)\\].*", "\\1", "1", $0);
    }
  }
  print("/* config.h                                                     */");
  print("/* Generated automatically by configure_win32.awk, DO NOT EDIT! */");
  print("/* To make changes to config.h edit config_win32.h.in instead.  */");
  print("");
}

/^#undef PACKAGE_BUGREPORT/ {
  print("#define PACKAGE_BUGREPORT \"" bugreport "\"");
  next;
}

/^#undef PACKAGE_NAME/ {
  print("#define PACKAGE_NAME \"" package "\"");
  next;
}

/^#undef PACKAGE_STRING/ {
  print("#define PACKAGE_STRING \"" package " " version "\"");
  next;
}

/^#undef PACKAGE_TARNAME/ {
  print("#define PACKAGE_TARNAME \"" package "\"");
  next;
}

/^#undef PACKAGE_VERSION/ {
  print("#define PACKAGE_VERSION \"" version "\"");
  next
}

/^#undef PACKAGE/ {
  print("#define PACKAGE \"" package "\"");
  next;
}

/^#undef VERSION/ {
  print("#define VERSION \"" version "\"");
  next;
}

{
  print;
}
