## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = @X_CFLAGS@

lib_LTLIBRARIES = libgpsim.la
gpsimincludedir = $(includedir)/gpsim

libgpsim_la_SOURCES = \
	12bit-hexdecode.cc \
	12bit-processors.cc \
	14bit-instructions.cc \
	14bit-processors.cc \
	14bit-registers.cc \
	14bit-tmrs.cc \
	14bit-hexdecode.cc \
	16bit-instructions.cc \
	16bit-processors.cc \
	16bit-registers.cc \
	16bit-hexdecode.cc \
	16bit-tmrs.cc \
	attributes.cc \
	at.cc \
	a2dconverter.cc \
	a2d_v2.cc \
	ctmu.cc \
	bitlog.cc \
	bit.cc \
	bytelog.cc \
	clc.cc \
	clock_phase.cc \
	comparator.cc \
	cwg.cc \
	eeprom.cc \
	errors.cc \
	i2c-ee.cc \
	gpsim_object.cc \
	gpsim_time.cc \
	init.cc \
	intcon.cc \
	interface.cc \
	ioports.cc \
	lcd_module.cc \
	lxt_write.c \
	modules.cc \
	nco.cc \
	op_amp.cc \
	p1xf1xxx.cc \
	p12f6xx.cc \
	p12x.cc \
	p16f62x.cc \
	p16x8x.cc \
	p16f8x.cc \
	p16f88x.cc \
	p16f87x.cc \
	p16x7x.cc \
	p16x5x.cc \
	p16x6x.cc \
	p16f91x.cc \
	p17c75x.cc \
	p18x.cc \
	p18fk.cc \
	packages.cc \
	pic-processor.cc \
	pic-registers.cc \
	pic-instructions.cc \
	pic-ioports.cc \
	pid.cc \
	pie.cc \
	pir.cc \
	pm_rd.cc \
	processor.cc \
	protocol.cc \
	registers.cc \
	sim_context.cc \
	stimuli.cc \
	symbol.cc \
	tmr0.cc \
	trace.cc \
	trigger.cc \
	uart.cc \
	ssp.cc \
	psp.cc \
	pps.cc \
	spp.cc \
	dsm_module.cc \
	icd.cc \
	ui.cc \
	value.cc \
	wdt.cc \
	zcd.cc

gpsiminclude_HEADERS = \
	12bit-instructions.h \
	12bit-processors.h \
	14bit-instructions.h \
	14bit-processors.h \
	14bit-registers.h \
	14bit-tmrs.h \
	16bit-instructions.h \
	16bit-processors.h \
	16bit-registers.h \
	16bit-tmrs.h \
	a2dconverter.h \
	a2d_v2.h \
	ctmu.h \
	attributes.h \
	at.h \
	bit.h \
	bitlog.h \
	bytelog.h \
	clc.h \
	clock_phase.h \
	cmd_gpsim.h \
	comparator.h \
	cwg.h \
	eeprom.h \
	exports.h \
	i2c-ee.h \
	gpsim_classes.h \
	gpsim_def.h \
	gpsim_interface.h \
	gpsim_object.h \
	gpsim_time.h \
	intcon.h \
	interface.h \
	ioports.h \
	lcd_module.h \
	lxt_write.h \
	modules.h \
	nco.h \
	op_amp.h \
	p12f6xx.h \
	p12x.h \
	p1xf1xxx.h \
	p16x5x.h \
	p16f62x.h \
	p16x6x.h \
	p16x7x.h \
	p16x8x.h \
	p16f8x.h \
	p16f88x.h \
	p16f87x.h \
	p16f91x.h \
	p17c75x.h \
	p18x.h \
	p18fk.h \
	packages.h \
	pic-instructions.h \
	pic-packages.h \
	pic-processor.h \
	pic-registers.h \
	pic-ioports.h \
	pid.h \
	pie.h \
	pir.h \
	pm_rd.h \
	processor.h \
	protocol.h \
	registers.h \
	rcon.h \
	sim_context.h \
	stimuli.h \
	symbol.h \
	tmr0.h \
	trace.h \
	trigger.h \
	uart.h \
	icd.h \
	ssp.h \
	psp.h \
	pps.h \
	spp.h \
	dsm_module.h \
	errors.h \
	ui.h \
	value.h \
	wdt.h \
	zcd.h



#libgpsim_la_LDFLAGS = $(shell gtk-config --libs)
libgpsim_la_LIBADD =  @X_LDFLAGS@

SUBDIRS = . dspic

EXTRA_DIST = makefile.mingw
