# USART module regression test
#
#


PROJECT = partest
OBJECTS =  $(PROJECT).o
OUTPUT = $(PROJECT).hex
COD = $(PROJECT).cod
GPSIM = gpsim
STC = $(PROJECT).stc
STARTUP_STC=partest.stc

all : $(OUTPUT)

$(OUTPUT) : $(OBJECTS) $(SCRIPT)
	gplink --map  -o $(OUTPUT) $(OBJECTS)

$(COD) : $(OBJECTS) $(SCRIPT)
	gplink --map  -o $(OUTPUT) $(OBJECTS)

%.o : %.asm
	gpasm -c $<

clean:
	rm -f *~ *.o *.lst *.map *.hex *.cod

$(PROJECT).o : $(PROJECT).asm

sim: $(COD)
	$(GPSIM) -c $(STARTUP_STC)

