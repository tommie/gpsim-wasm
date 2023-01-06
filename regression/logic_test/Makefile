include ../make.regression


SCRIPT = 16f873.lkr

PROJECT = logic_test
OBJECTS =  $(PROJECT).o
OUTPUT = $(PROJECT).hex
COD = $(PROJECT).cod

all : $(OUTPUT)

$(OUTPUT) $(COD) : $(OBJECTS) $(SCRIPT)
	gplink --map -s $(SCRIPT) -o $(OUTPUT) $(OBJECTS)

sim: $(COD)
	$(GPSIM) -i -I $(STARTUP_STC) -D STC="$<"
