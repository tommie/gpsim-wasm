
include ../make.regression


all : ttl377.cod ttl165.cod ttl595.cod

ttl%.cod : ttl%.o
	gplink --map -o $@  $<

sim : sim_ttl377 sim_ttl165 sim_ttl595

