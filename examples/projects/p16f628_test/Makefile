# Module example
#
#

# system programs
RM              = /bin/rm -f
ARC             = tar -cvf
GZIP            = gzip
ZIP		= zip

# gpasm programs
ASM             = gpasm
DISASM          = gpdasm
CODVIEWER       = gpvc -d 

# simulator
GPSIM		= gpsim

# Programmer
DEVICE		= 16F84
SERIAL_PORT	= /dev/ttyS1
PROG		= picp $(SERIAL_PORT) $(DEVICE) -wp

# project data
PROJECT         = f628
VERSION         = 1
SOURCES		= f628.asm f628.stc
HEADER	        = /usr/local/share/gpasm/header

DIST_FILES = $(SOURCES) \
	f628.hex f628.asm f628.stc	\
	ChangeLog	\
	Makefile	\
	README		


# derived variables
PROJ_VER	= $(PROJECT)-$(VERSION)
ARCHIVE         = $(PROJ_VER).tar
ZIP_PROJ_VER	= $(PROJECT)$(VERSION)
ZIPARCHIVE	= $(ZIP_PROJ_VER)
LATEST_HEX      = $(PROJECT)$(VERSION).hex

all:	$(PROJECT).cod

sim:	$(PROJECT).cod
	$(GPSIM) -c $(PROJECT).stc

$(PROJECT).cod: $(SOURCES)
	$(ASM) -I $(HEADER) $(PROJECT).asm

#$(PROJECT).hex: $(PROJECT).asm
#	$(ASM) -I $(HEADER) $(PROJECT).asm

$(ARCHIVE): $(DIST_FILES)
	mkdir $(PROJ_VER)
	cp $(DIST_FILES) $(PROJ_VER)/.
	$(ARC) $(ARCHIVE) $(PROJ_VER)
	rm -rf $(PROJ_VER)

#zip: $(DIST_FILES)
zip: $(PROJECT).cod
	mkdir $(ZIP_PROJ_VER)
	cp $(DIST_FILES) $(ZIP_PROJ_VER)/.
	cp $(PROJECT).hex $(ZIP_PROJ_VER)/$(LATEST_HEX)
	cp $(PROJECT).hex $(LATEST_HEX)
	$(ZIP) -r $(ZIP_PROJ_VER) $(ZIP_PROJ_VER)
	rm -rf $(ZIP_PROJ_VER)

archive: $(ARCHIVE)
	$(GZIP) $(ARCHIVE)


prog:	$(PROJECT).cod
	$(PROG) $(PROJECT).hex

clean:
	$(RM) *.lst *.cod $(PROJECT)*.gz *~

