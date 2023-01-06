EXTRA_DIST = ds1820.asm ds18b20.asm 16f1823_g.lkr README

MOSTLYCLEANFILES = *.o *.hex *.cod *.lst *.map *~
CLEANFILES = *.o *.hex *.cod *.lst *.map *~
DISTCLEANFILES = *.o *.hex *.cod *.lst *.map *~
MAINTAINERCLEANFILES = *.o *.hex *.cod *.lst *.map *~

.SUFFIXES:  .o .asm .cod

all: ds1820.cod ds18b20.cod 


ds1820.cod: ds1820.o

ds18b20.cod: ds18b20.o

%.cod: %.o
	gplink --map -o $@ $<

%.o: %.asm
	gpasm -c $<

clean:
	rm -f ${CLEANFILES}
