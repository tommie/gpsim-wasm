include ../make.regression


all : p18f452.cod p12f1822.cod

p%.cod : p%.o
	gplink --map -s $*.lkr -o $@  $<


sim: sim_p18f452 sim_p12f1822
