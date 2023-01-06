# P12f1822, P16F1823  processor regression tests
#
#

include ../make.regression


all :   p12f1822.cod  p16f1788.cod p16f1788_comp.cod p16f1823.cod p16f1823_spi.cod p16f1823_i2c.cod p16f1823_i2c_v2.cod p16f1823_comp.cod p16f1788.cod p12f1822_usart.cod p12f1840.cod p16f1825.cod p16f1503.cod p16f1503_nco.cod zcd_f1705.cod p16f1705.cod p16f1705_comp.cod p16f1709.cod p16f1709_cog.cod p16f1709_i2c.cod p16f1705_clc.cod p16f1705_cog.cod

p16f1823_i2c_v2.o : p16f1823_i2c_v2.asm i2c_low.inc

%.cod : %.o
	gplink --map  -o $@  $<


sim: sim_p12f1822 sim_p16f1823 sim_p16f1823_i2c_v2 sim_p16f1823_i2c sim_p16f1823_comp sim_p16f1823_spi sim_p12f1822_usart sim_p16f1788 sim_p16f1788_comp sim_p16f1705 sim_p16f1709 sim_p16f1709_cog sim_p16f1709_i2c sim_zcd_f1705 sim_p16f1705_clc sim_p16f1705_cog

