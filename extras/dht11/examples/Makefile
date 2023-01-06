
MOSTLYCLEANFILES = *.o *.hex *.cod *.lst *.map *~
CLEANFILES = *.o *.hex *.cod *.lst *.map *~
DISTCLEANFILES = *.o *.hex *.cod *.lst *.map *~
MAINTAINERCLEANFILES = *.o *.hex *.cod *.lst *.map *~

.SUFFIXES: .asm .cod

all: dht11_example.cod

dht11_example.cod: dht11_example.o


%.cod: %.o
	gplink --map $< -o $@ 

%.o : %.asm 
	gpasm -c $<


clean:
	rm -f ${CLEANFILES}
