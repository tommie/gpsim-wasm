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
PROJECT         = mod_test
VERSION         = 0.0.1

HEADER	        = /usr/local/share/gpasm/header

# derived variables
ARCHIVE         = $(PROJECT)-$(VERSION).tar

all:	$(PROJECT).cod

sim:	$(PROJECT).cod
	$(GPSIM) -c $(PROJECT).stc

$(PROJECT).cod: $(PROJECT).asm
	$(ASM) -I $(HEADER) $(PROJECT).asm

$(ARCHIVE):
	$(ARC) $(ARCHIVE) *

archive: $(ARCHIVE)
	$(ZIP) $(ARCHIVE)

clean:
	$(RM) *.hex *.lst *.cod $(PROJECT)*.gz *~

