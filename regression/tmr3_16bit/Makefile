include ../make.regression


SCRIPT = 18f452.lkr

PROJECT = tmr3_16bit
OBJECTS =  $(PROJECT).o 
OUTPUT = $(PROJECT).hex
COD = $(PROJECT).cod
STC = $(PROJECT).stc

all : tmr3_16bit.cod tmr3_18f4550.cod

$(OUTPUT) $(COD) : $(OBJECTS) $(SCRIPT)
	gplink --map -s $(SCRIPT) -o $(OUTPUT) $(OBJECTS)

tmr3_18f4550.cod : tmr3_18f4550.o
	gplink --map -s 18f4550_g.lkr -o tmr3_18f4550.cod tmr3_18f4550.o

sim: sim_16bit sim_18f4550


sim_16bit: tmr3_16bit.cod 
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$<

sim_18f4550: tmr3_18f4550.cod
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$<
