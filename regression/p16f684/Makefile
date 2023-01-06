
include ../make.regression


#
# Use 16f684.lkr for all processors in this family
#
SCRIPT = 16f684.lkr

all : nwdt_16f684.cod compar_684.cod wdt_16f684.cod  epwm.cod a2d_684.cod reset.cod two_speed.cod rc_16f684.cod

#	wdt_16f685.cod eusart.cod 

nwdt_16f684.cod: nwdt_16f684.o $(SCRIPT)
	gplink --map  -s $(SCRIPT) -o nwdt_16f684.cod nwdt_16f684.o

wdt_16f684.cod: wdt_16f684.o $(SCRIPT)
	gplink --map  -s $(SCRIPT) -o wdt_16f684.cod wdt_16f684.o

a2d_684.cod: a2d_684.o $(SCRIPT)
	gplink --map  -s $(SCRIPT) -o a2d_684.cod a2d_684.o

compar_684.cod: compar_684.o $(SCRIPT)
	gplink --map  -s $(SCRIPT) -o compar_684.cod compar_684.o

epwm.cod: epwm.o $(SCRIPT)
	gplink --map  -s $(SCRIPT) -o epwm.cod epwm.o

reset.cod: reset.o $(SCRIPT)
	gplink --map  -s $(SCRIPT) -o reset.cod reset.o

two_speed.cod: two_speed.o $(SCRIPT)
	gplink --map  -s $(SCRIPT) -o two_speed.cod two_speed.o

rc_16f684.cod: rc_16f684.o $(SCRIPT)
	gplink --map  -s $(SCRIPT) -o rc_16f684.cod rc_16f684.o

sim: sim_epwm sim_nwdt_16f684  \
	sim_wdt_16f684 sim_reset sim_compar_684
 

sim_epwm: epwm.cod
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$<

sim_reset: reset.cod
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$<

sim_compar_684: compar_684.cod
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$<

sim_nwdt_16f684: nwdt_16f684.cod
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$<

sim_wdt_16f684: wdt_16f684.cod
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$<


