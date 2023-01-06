include ../make.regression


SCRIPT = 18f452.lkr

PROJECT = port_stim
OBJECTS =  $(PROJECT).o
OUTPUT = $(PROJECT).hex
COD = $(PROJECT).cod

all : port_stim.cod

$(OUTPUT) $(COD) : $(OBJECTS) $(SCRIPT)
	gplink --map -s $(SCRIPT) -o $(OUTPUT) $(OBJECTS)

sim: sim_port_stim

