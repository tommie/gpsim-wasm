include ../make.regression


all : interrupts_16bit.cod priority.cod int_18f1320.cod


interrupts_16bit.cod : interrupts_16bit.o
	gplink --map -s 18f2321.lkr -o $@  $^

priority.cod : priority.o
	gplink --map -s 18f2321.lkr -o $@  $^


int_18f1320.cod : int_18f1320.o
	gplink --map -s 18f1320.lkr -o $@  $^

test_basic: interrupts_16bit.cod
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=interrupts_16bit.stc

test_priority: priority.cod
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=priority.stc

sim: test_basic test_priority
