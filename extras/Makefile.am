# Makefile for the extras module library
# 
#AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = @X_CFLAGS@ -Ids1820 -Ilcd -Ids1307 -Idht11 \
             -Isolar -Igraphic_lcd/src


lib_LTLIBRARIES = libgpsim_extras.la

libgpsim_extras_la_SOURCES = \
	module_manager.cc ds1820/bit1w.cc  ds1820/ds1820.cc \
        ds1820/rom1w.cc ds1307/ds1307.cc  solar/solar.cc \
        lcd/hd44780.cc  lcd/lcdgui.cc lcd/lcd.cc  dht11/dht11.cc \
	lcd/raw_lcd.cc \
        graphic_lcd/src/glcd_100X32_sed1520.cc graphic_lcd/src/glcd.cc \
        graphic_lcd/src/osram.cc graphic_lcd/src/sed1520.cc \
         graphic_lcd/src/ssd0323.cc


libgpsim_extras_la_LDFLAGS =  @X_LDFLAGS@



EXTRA_DIST = makefile.mingw \
	rs232-gen/rs232-gen.c rs232-gen/Makefile rs232-gen/README \
	rs232-gen/example/Makefile rs232-gen/example/README \
	rs232-gen/example/example.stc rs232-gen/example/example.asm


SUBDIRS = dht11 graphic_lcd ds1820 ds1307 lcd solar

CFLAGS = @CFLAGS@
