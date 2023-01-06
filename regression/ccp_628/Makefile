include ../make.regression



all : ccp_628.cod



ccp_628.cod : ccp_628.o
	gplink --map -s 16f628.lkr -o $@  $^

sim: sim_ccp_628

