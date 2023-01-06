include ../make.regression



all :  p16f887.cod p16f882.cod comparator.cod p16f883.cod eusart.cod i2c.cod p16f887_epwm.cod a2d.cod p16f886.cod i2c_slave.cod

%.cod : %.o
	gplink --map  -q -o $@ $<


sim: sim_p16f887 sim_p16f882 sim_comparator sim_p16f883 sim_eusart sim_i2c sim_i2c_slave sim_p16f887_epwm sim_a2d sim_p16f886


