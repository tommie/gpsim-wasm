include ../make.regression

all : p16c71.cod p18f452.cod p16f873a.cod p16f88.cod p16f871.cod p16f874a.cod p16f819.cod p18f4321.cod p18f1220.cod p10f222.cod p18f26k22.cod


p%.cod : p%.o
	gplink --map  -o $@  $<


sim: sim_p16c71 sim_p16f871 sim_p16f873a sim_p16f874a sim_p16f88 sim_p16f819 sim_p18f452 sim_p18f4321 sim_p18f1220 sim_p10f222 sim_p18f26k22
