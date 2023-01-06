#makefile for socket test.

GCC      = g++
CFLAGS   = `pkg-config --cflags glib-2.0 gtk+-2.0`
LIBS     =  -lstdc++  `pkg-config --libs glib-2.0 gtk+-2.0` -lgpsim -lpthread

ASM      = gpasm
PICSRC   = gensquares

all: client client2 client3 $(PICSRC).cod

client_interface.o: client_interface.cc
	$(GCC) -c client_interface.cc $(CFLAGS)

client.o: client.cc
	$(GCC) -c client.cc $(CFLAGS)

client: client.o client_interface.o
	$(GCC) -o client client.o client_interface.o $(LIBS)


client2.o: client2.cc
	$(GCC) -c client2.cc $(CFLAGS)

client2: client2.o client_interface.o
	$(GCC) -o client2 client2.o client_interface.o $(LIBS)


client3.o: client3.cc
	$(GCC) -c client3.cc $(CFLAGS)

client3: client3.o client_interface.o
	$(GCC) -o client3 client3.o client_interface.o $(LIBS)


$(PICSRC).cod: $(PICSRC).asm
	gpasm $(PICSRC).asm

clean:
	rm -f $(PICSRC).lst $(PICSRC).hex $(PICSRC).cod
	rm -f client client2 client3 *.o
	rm -f *~


