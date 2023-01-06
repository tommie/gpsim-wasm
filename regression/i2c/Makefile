# I2C module regression test
#
#

include ../make.regression


all :   p16f876a.cod p16f88.cod p16f819.cod p16f876a_2par.cod


p%.cod : p%.o
	gplink --map -o $@  $<

ds1307.cod : ds1307.o
	gplink --map -o $@  $<

p16f88.cod: p16f88.asm i2c_low.inc

p16f819.cod: p16f819.asm i2c_low.inc

sim: sim_p16f88 sim_p16f876a sim_p16f819

