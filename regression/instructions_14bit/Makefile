include ../make.regression


all : instructions_14bit.cod branching.cod

%.cod: %.o
	gplink --map -s 16f628.lkr -o $@ $^

