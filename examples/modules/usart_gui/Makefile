PROJECT = usart_gui
OBJECTS =  $(PROJECT).o
OUTPUT = $(PROJECT).cod
STC = $(PROJECT).stc
SRC = $(PROJECT).asm
GPSIM = gpsim
 
all : $(OUTPUT)
 
$(OUTPUT) : $(SRC)
	gpasm -w1 $(SRC)

clean:
	rm -f *~ *.o *.lst *.map *.hex *.cod

