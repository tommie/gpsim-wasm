# p16f6xx  regression test
#
#

include ../make.regression


all :   p16f616.cod p16f610.cod 


%.cod : %.o
	gplink -q --map -o $@  $<



sim: sim_p16f610 sim_p16f616

