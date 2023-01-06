
SCRIPT = 18f452.lkr

PROJECT = glcd_test
OBJECTS =  $(PROJECT).o sed1520.o lcd100X32.o
OUTPUT = $(PROJECT).hex
COD = $(PROJECT).cod
STC = $(PROJECT).stc
STARTUP_STC=startup.stc

all : $(OUTPUT)

$(OUTPUT) $(COD) : $(OBJECTS) $(SCRIPT)
	gplink --map -s $(SCRIPT) -o $(OUTPUT) $(OBJECTS)

%.o : %.asm
	gpasm -c $<

sim: $(COD)
	$(GPSIM) -i -c $(STARTUP_STC) -D STC=$(STC)

clean:
	rm -f *~ *.o *.lst *.map *.hex
