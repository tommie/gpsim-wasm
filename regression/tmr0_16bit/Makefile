include ../make.regression


SCRIPT = 18f452.lkr

PROJECT = tmr0_16bit
OBJECTS =  $(PROJECT).o delay.o
OUTPUT = $(PROJECT).hex
COD = $(PROJECT).cod

all : $(OUTPUT)

$(OUTPUT) $(COD) : $(OBJECTS) $(SCRIPT)
	gplink --map -s $(SCRIPT) -o $(OUTPUT) $(OBJECTS)

sim: $(COD)
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$<
