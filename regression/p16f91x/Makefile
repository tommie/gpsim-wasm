include ../make.regression



all :  p16f917.cod  p16f916.cod p16f914.cod p16f913.cod compar_917.cod i2c_slave.cod  ccp_917.cod usart_917.cod pwm_914.cod 

%.cod : %.o
	gplink --map  -q -o $@ $<


sim: sim_p16f917 sim_p16f916 sim_p16f914 sim_p16f913 sim_compar_917 sim_i2c_slave sim_ccp_917 sim_usart_917 sim_pwm_914


