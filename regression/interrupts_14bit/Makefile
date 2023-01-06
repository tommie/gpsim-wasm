include ../make.regression


SCRIPT = 16f84.lkr

PROJECT = interrupts_14bit
OBJECTS =  $(PROJECT).o
OUTPUT = $(PROJECT).hex
COD = $(PROJECT).cod int_sleep.cod
STC = $(PROJECT).stc

all : $(COD)

$(PROJECT).cod  : $(OBJECTS) $(SCRIPT)
	gplink --map -s $(SCRIPT) -o $(PROJECT).cod $(OBJECTS)

int_sleep.cod : int_sleep.o $(SCRIPT)
	gplink --map -s $(SCRIPT) -o int_sleep.cod  int_sleep.o

sim: $(COD)
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$(STC)
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=int_sleep.cod
