include ../make.regression



all : compar_873a.cod compar_628.cod compar_877a.cod compar_10f204.cod \
	compar_882.cod compar_26k22.cod compar_2321.cod compar_6520.cod


%.cod : %.o
	gplink --map  -q -o $@  $<

sim: sim_compar_628 sim_compar_873a  sim_compar_877a sim_compar_10f204 \
	sim_compar_882 sim_compar_26k22 sim_compar_2321 sim_compar_6520 \
	sim_compar_26k22

sim_compar_873a: compar_873a.cod
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=compar_873a.stc

