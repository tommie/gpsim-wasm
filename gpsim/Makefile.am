# gpsim  executable Makefile
#
#

AM_CPPFLAGS = @X_CFLAGS@ @Y_CFLAGS@

bin_PROGRAMS = gpsim

gpsim_SOURCES = main.cc \
	gpsim.h.in

#gpsim_LDFLAGS =  $(shell gtk-config --cflags)
gpsim_LDADD = ../src/libgpsim.la ../cli/libgpsimcli.la ../gui/libgpsimgui.la \
  ../eXdbm/libgpsim_eXdbm.la  ../modules/libgpsim_modules.la \
  ../extras/libgpsim_extras.la \
  @GTK@ @GDK@ @GLIB@ -lstdc++ -lpopt @LIBDL@ \
  @X_LDFLAGS@ @Y_LDFLAGS@ @LIBREADLINE@

# Make sure we have parse.h when compiling other sources
BUILT_SOURCES = gpsim.h

CLEANFILES = gpsim.h

EXTRA_DIST = makefile.mingw

gpsim.h: gpsim.h.in
	sed -e "s/@REVISION@/$$(${top_srcdir}\/get_cl_revision.sh -s ${top_srcdir}\/ChangeLog)/g" "$<" > "$@"
