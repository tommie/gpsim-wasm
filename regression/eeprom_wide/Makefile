include ../make.regression


SCRIPT = 16f873.lkr

PROJECT = eeprom_wide
OBJECTS =  $(PROJECT).o
OUTPUT = $(PROJECT).hex
COD = $(PROJECT).cod
STC = $(PROJECT).stc

all : $(COD)

%.cod : %.o
	gplink --map  -q -o $@ $<


sim: $(COD)
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$(STC)
