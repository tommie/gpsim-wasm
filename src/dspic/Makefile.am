## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = @X_CFLAGS@

lib_LTLIBRARIES = libgpsim_dspic.la

libgpsim_dspic_la_SOURCES = \
	gpsim_modules.cc \
	dspic-processors.cc   dspic-processors.h   \
	dspic-instructions.cc dspic-instructions.h \
	dspic-registers.cc dspic-registers.h

libgpsim_dspic_la_LDFLAGS =  @X_LDFLAGS@

EXTRA_DIST = makefile.mingw
