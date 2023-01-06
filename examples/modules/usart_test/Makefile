# USART module regression test
#
#

SCRIPT = 16f877.lkr

PROJECT = usart_test
OBJECTS =  $(PROJECT).o
OUTPUT = $(PROJECT).hex
COD = $(PROJECT).cod
GPSIM = gpsim
STC = $(PROJECT).stc
STARTUP_STC=usart_test.stc

all : $(OUTPUT)

$(OUTPUT) : $(OBJECTS) $(SCRIPT)
	gplink --map -s $(SCRIPT) -o $(OUTPUT) $(OBJECTS)

$(COD) : $(OBJECTS) $(SCRIPT)
	gplink --map -s $(SCRIPT) -o $(OUTPUT) $(OBJECTS)

%.o : %.asm
	gpasm -c $<

clean:
	rm -f *~ *.o *.lst *.map *.hex *.cod

$(PROJECT).o : $(PROJECT).asm

sim: $(COD)
	$(GPSIM) -c $(STARTUP_STC)

