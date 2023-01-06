include ../make.regression


all : pulsegen.cod

pulsegen.cod : pulsegen.o
	gplink --map -s 16f877a.lkr -o $@  $^

sim: pulsegen

pulsegen: pulsegen.cod
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$<

