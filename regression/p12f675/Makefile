# P12f processor regression tests
#
#

include ../make.regression


all :   p12f629.cod p12f675.cod p12f683.cod


p%.cod : p%.o
	gplink --map -s $*.lkr -o $@  $<


sim: sim_p12f629 sim_p12f675 sim_p12f683

