include ../make.regression


SCRIPT = 16f873.lkr
OBJECTS = assertions.o
OUTPUT = assertions.hex

all : $(OUTPUT)

$(OUTPUT) : $(OBJECTS) $(SCRIPT)
	gplink --map -s $(SCRIPT) -o $(OUTPUT) $(OBJECTS)

assertions.o : assertions.asm
