
include ../make.regression


#
# Use 16f631.lkr for all processors in this family
#
SCRIPT = 16f631.lkr

all : nwdt_16f631.cod wdt_16f677.cod p16f690.cod epwm.cod \
	wdt_16f685.cod eusart.cod eusart_690.cod

nwdt_16f631.cod: nwdt_16f631.o $(SCRIPT)
	gplink --map  -s $(SCRIPT) -o nwdt_16f631.cod nwdt_16f631.o

wdt_16f677.cod: wdt_16f677.o $(SCRIPT)
	gplink --map  -s $(SCRIPT) -o wdt_16f677.cod wdt_16f677.o

wdt_16f685.cod: wdt_16f685.o $(SCRIPT)
	gplink --map  -s $(SCRIPT) -o wdt_16f685.cod wdt_16f685.o

p16f690.cod: p16f690.o $(SCRIPT)
	gplink --map  -s $(SCRIPT) -o p16f690.cod p16f690.o

eusart.cod: eusart.o $(SCRIPT)
	gplink --map  -s $(SCRIPT) -o eusart.cod eusart.o

eusart_690.cod: eusart_690.o $(SCRIPT)
	gplink --map  -s $(SCRIPT) -o eusart_690.cod eusart_690.o

epwm.cod: epwm.o $(SCRIPT)
	gplink --map  -s $(SCRIPT) -o epwm.cod epwm.o

sim: sim_epwm sim_eusart sim_p16f690 sim_nwdt_16f631 sim_wdt_16f677 \
	sim_wdt_16f685 sim_eusart_690

sim_epwm: epwm.cod
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$<

sim_eusart: eusart.cod
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$<

sim_p16f690: p16f690.cod
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$<

sim_nwdt_16f631: nwdt_16f631.cod
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$<

sim_wdt_16f677: wdt_16f677.cod
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$<

sim_wdt_16f685: wdt_16f685.cod
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$<

