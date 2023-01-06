## Process this file with automake to produce Makefile.in

## T. Scott Dattalo - created this makefile so that eXdbm
## could be packaged with the gpsim distribution. The source
## code was unchanged except for the following minor change:
##
## --- misc.h      Mon Jan 17 20:25:24 2000
## +++ misc.h~     Sat Dec 27 13:46:53 1997
## @@ -35,7 +35,7 @@
##  #define MISC_H
## 
##  #include <stdio.h>
## -#include "eXdbmTypes.h"
## +#include <eXdbmTypes.h>
##
##  void RaiseError(int errorcode);
##  int DbmIsInit(void);


AM_CPPFLAGS = @X_CFLAGS@

lib_LTLIBRARIES = libgpsim_eXdbm.la
gpsim_eXdbmincludedir = $(includedir)/eXdbm

libgpsim_eXdbm_la_SOURCES = hash.c parse.c misc.c eXdbm.c \
	hash.h parse.h  misc.h

gpsim_eXdbminclude_HEADERS = eXdbm.h eXdbmErrors.h eXdbmTypes.h

libgpsim_eXdbm_la_LIBADD = @X_LDFLAGS@

EXTRA_DIST = \
	Makefile.orig README LICENSE COPYING.LIB eXdbm.txt \
	test1.c test2.c test3.c test1.cfg.orig test2.cfg \
	README.gpsim

