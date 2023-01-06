# P16f676 regression test
#
#

include ../make.regression


all :   p16f676.cod reset.cod 


%.cod : %.o
	gplink -q --map -o $@  $<



sim: sim_p16f676 sim_reset

