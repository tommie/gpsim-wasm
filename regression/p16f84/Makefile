include ../make.regression


SCRIPT = 16f84.lkr

all : p16f84.cod reset.cod

%.cod : %.o
	gplink --map -s $(SCRIPT) -o $@ $<


sim: sim_p16f84 sim_reset

