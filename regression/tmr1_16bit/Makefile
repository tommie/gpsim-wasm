include ../make.regression


all : tmr1_16bit.cod  tmr1_18f2620.cod tmr1_18f26k22.cod tmr1_18f2525.cod

%.cod : %.o
	gplink --map  -o $@  $<


sim: sim_tmr1_16bit sim_tmr1_18f2620 sim_tmr1_18f26k22 sim_tmr1_18f2525
