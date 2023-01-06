EXTRA_DIST = 

MOSTLYCLEANFILES = *.o *.hex *.cod *.lst *.map *~
CLEANFILES = *.o *.hex *.cod *.lst *.map *~
DISTCLEANFILES = *.o *.hex *.cod *.lst *.map *~
MAINTAINERCLEANFILES = *.o *.hex *.cod *.lst *.map *~

.SUFFIXES:  .o .asm .cod

all: ds1307.cod 


ds1307.cod: ds1307.o


%.cod: %.o
	gplink --map -o $@ $<

%.o: %.asm
	gpasm -c $<

clean:
	rm -f ${CLEANFILES}
