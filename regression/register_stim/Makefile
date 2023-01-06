include ../make.regression


SCRIPT = 16f84.lkr

PROJECT = register_stim
OBJECTS =  $(PROJECT).o
OUTPUT = $(PROJECT).hex
COD = $(PROJECT).cod
STC = $(PROJECT).stc

all : $(OUTPUT)

$(OUTPUT) $(COD) : $(OBJECTS) $(SCRIPT)
	gplink --map -s $(SCRIPT) -o $(OUTPUT) $(OBJECTS)

$(PROJECT).o : $(PROJECT).asm

sim: $(COD)
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$(STC)
