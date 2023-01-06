
ACLOCAL_AMFLAGS = -I m4

if HAVE_CLI
cli_subdir = cli
else
cli_subdir =
endif

if HAVE_PROGRAM
gpsim_subdir = gpsim
else
gpsim_subdir =
endif

SUBDIRS = eXdbm src $(cli_subdir) xpms gui modules extras $(gpsim_subdir) examples doc regression

dist-hook:
	cp gpsim.spec $(distdir)

EXTRA_DIST = PROCESSORS HISTORY README.EXAMPLES ANNOUNCE README.MODULES \
	README INSTALL.gpsim COPYING.LESSER \
	config_win32.h.in \
	makefile.mingw  \
	plat/win32/uxtime.h \
	plat/win32/libgpsim.def \
	plat/win32/glist.cpp \
	plat/win32/settings_reg.h \
	plat/win32/makefile.mingw \
	plat/win32/modules.def \
	plat/win32/icd.cc \
	plat/win32/uxsleep.cc \
	plat/win32/gpsim.ico \
	plat/win32/gpsim.def \
	plat/win32/gpsim.nsi \
	plat/win32/uxtime.cc \
	plat/win32/fd2raw.cpp \
	plat/win32/unistd.h \
	plat/win32/fd2raw.h \
	plat/win32/settings_reg.cpp \
	plat/win32/configure_win32.awk \
	plat/win32/make.mingw
