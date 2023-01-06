# PSP (Parallel Slave Port) module regression test
#
#

include ../make.regression


all : p16f871.cod p18f452.cod p18f6520.cod


p%.cod : p%.o
	gplink --map -s $*.lkr -o $@  $<

sim: sim_p16f871 sim_p18f452 sim_p18f6520

