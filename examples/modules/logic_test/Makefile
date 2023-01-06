# Module example
#
#

# system programs
RM              = /bin/rm -f
ARC             = tar -cvf
ZIP             = gzip

# gpasm programs
ASM             = gpasm
DIASM           = gpdasm
CODVIEWER       = gpvc -d 

# simulator
GPSIM		= gpsim

# project data
PROJECT         = logic_mod
VERSION         = 0.0.1

# derived variables
ARCHIVE         = $(PROJECT)-$(VERSION).tar

all:	$(PROJECT).cod

sim:	$(PROJECT).cod
	$(GPSIM) -c $(PROJECT).stc

$(PROJECT).cod: $(PROJECT).asm
	$(ASM)  $(PROJECT).asm

$(ARCHIVE):
	$(ARC) $(ARCHIVE) *

archive: $(ARCHIVE)
	$(ZIP) $(ARCHIVE)

clean:
	$(RM) *.hex *.lst *.cod $(PROJECT)*.gz *~

