include ../make.regression

all : p18f1220.cod p18f2321.cod p18f26k22.cod p18f14k22.cod #p18f4321.cod p16f882.cod p16f887.cod


p%.cod : p%.o
	gplink --map  -o $@  $<



sim: sim_p18f1220 sim_p18f2321 sim_p18f26k22 sim_p18f14k22 #sim_p18f4321 sim_p16f882 sim_p16f887
