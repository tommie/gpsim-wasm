# gpsim gui

AM_CPPFLAGS = @X_CFLAGS@ @Y_CFLAGS@

lib_LTLIBRARIES = libgpsimgui.la

libgpsimgui_la_SOURCES =  bitlog.cc bytelog.cc gui_break.cc gui_main.cc \
	gui_menu.cc gui_processor.cc gui_regwin.cc gui_src.cc gui_src_asm.cc \
	gui_src_opcode.cc gui_statusbar.cc \
	gui_symbols.cc gui_watch.cc gui_breadboard.cc \
	gui_stack.cc gui_trace.cc gui_profile.cc \
	gui_stopwatch.cc gui_object.cc gui_scope.cc settings_exdbm.cc \
	preferences.cc \
	gtkextra/gtkextra-marshal.c \
	gtkextra/gtkitementry.c \
	gtkextra/gtksheet.c \
	gtkextra/gtkextra.c

noinst_HEADERS = bitlog.h bytelog.h gui.h \
	gui_interface.h \
	gui_breadboard.h gui_object.h gui_processor.h gui_profile.h \
	gui_register.h gui_regwin.h \
	gui_scope.h gui_src.h gui_stack.h gui_statusbar.h gui_stopwatch.h \
	gui_symbols.h gui_trace.h gui_watch.h \
	preferences.h \
	settings.h settings_exdbm.h \
	gtkextra/gtkextra-marshal.h \
	gtkextra/gtkitementry.h \
	gtkextra/gtksheet.h \
	gtkextra/gtkextrafeatures.h

libgpsimgui_la_LIBADD = @X_LDFLAGS@ @Y_LDFLAGS@ @GTK@ @GDK@ @GLIB@ @LIBREADLINE@ ../eXdbm/libgpsim_eXdbm.la ../src/libgpsim.la ../cli/libgpsimcli.la

pixmapdir = $(datadir)/gpsim

AM_CFLAGS =
AM_CXXFLAGS =

EXTRA_DIST = makefile.mingw gtkextra/COPYING
