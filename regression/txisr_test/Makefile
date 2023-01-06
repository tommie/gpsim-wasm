# USART module transmit interrupt regression test
#
#

include ../make.regression


SCRIPT = 16f871.lkr

PROJECT = txisr_test
OBJECTS =  $(PROJECT).o
OUTPUT = $(PROJECT).hex
COD = $(PROJECT).cod

all : $(OUTPUT)

$(OUTPUT) $(COD) : $(OBJECTS) $(SCRIPT)
	gplink --map -s $(SCRIPT) -o $(OUTPUT) $(OBJECTS)

sim: $(COD)
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$<
