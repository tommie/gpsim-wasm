include ../make.regression

all: hlvd_26k22.cod  ctmu_26k22.cod tbl_26k22.cod sync_26k22.cod osccon_26k22.cod test.cod srlatch_26k22.cod

%.cod : %.o
	gplink -q --map  -o $@  $<

sim: sim_hlvd_26k22 sim_ctmu_26k22 sim_tbl_26k22 sim_sync_26k22 sim_osccon_26k22

.PHONY: all clean
