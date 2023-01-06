include ../make.regression


STARTUP_STC=../startup.stc


all : reset.cod

reset.cod: reset.o
	gplink  --map -s 12c509.lkr -o $@  $<

sim: sim_p12c509_reset

sim_p12c509_reset: reset.cod
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$<
