#!/bin/sh
#
# A meta-script that invokes each of the individual
# regression tests.

if [ $# -gt 1 ]
then
  echo "Usage: `basename $0` [ <gpsim_path> ] "
  exit 0
fi

case "$1" in
    *clean)
        DIRS='breakpoints instructions_12bit instructions_14bit 
		instructions_16bit node_test p16f84 p18f452_ports 
		p16f628 digital_stim analog_stim p12ce518 eeprom_wide epwm
		interrupts_14bit macro_test logic_test resistor usart_test 
	        txisr_test tmr0_16bit tmr1_16bit tmr3_16bit switch_test p18f 
		comparator a2d psp ttl ccp ccp_628 wavegen spi i2c port_stim 
		p12c509 wdt p12f675 p16f676 p16f690 p16f6xx p16f684 p1xf18xx
		p18f26k22 p16f91x p10f32x intosc'

        echo ${DIRS}
        for i in ${DIRS} ; do
            echo $i
            cd $i
            make clean
            cd ..
        done

        exit 0
        ;;

    *)
        if [ $# -gt 0 ]
            then
            GPSIM_PATH=$1
            export GPSIM_PATH
        fi
        ;;
esac

GPSIM_MODULE_PATH=$(pwd)/../modules/.libs
export GPSIM_MODULE_PATH

RT=./rt.sh

# Basic breakpoint test
${RT} breakpoints sim

# Instruction set simulation of the mid-range devices
${RT} instructions_14bit sim_instructions_14bit
${RT} instructions_14bit sim_branching

#instruction set simulation for the 16bit cores:
${RT} instructions_16bit sim

#instruction set simulation for the 12bit cores:
${RT} instructions_12bit sim

${RT} node_test sim

#${RT} p12c509 it_12bit
#${RT} p12c509 tmr0

${RT} p16f84 sim

${RT} p18f452_ports sim

#${RT} p16f628 p16f628

#${RT} p16f873 sim

${RT} digital_stim sim

${RT} analog_stim sim

${RT} register_stim sim

${RT} p12ce518 sim

${RT} eeprom_wide sim

${RT} interrupts_14bit sim

${RT} interrupts_16bit test_basic
${RT} interrupts_16bit test_priority

${RT} macro_test sim

${RT} logic_test sim

${RT} resistor sim

${RT} usart_test sim_pir1v1

${RT} usart_test sim_pir1v2

${RT} usart_test sim_eusart

${RT} usart_test sim_eusart_2455

${RT} usart_test sim_16f88

${RT} txisr_test sim

${RT} tmr0_16bit sim

${RT} tmr1_16bit sim

${RT} tmr3_16bit sim

${RT} switch_test sim

${RT} p18f sim_instructions

${RT} p18f sim_extended_instructions

${RT} p18f sim_reset

${RT} comparator sim

${RT} a2d sim_p16c71

${RT} a2d sim_p16f871

${RT} a2d sim_p16f873a

${RT} a2d sim_p16f874a

${RT} a2d sim_p16f88

${RT} a2d sim_p16f819

${RT} a2d sim_p18f452

${RT} a2d sim_p18f4321

${RT} a2d sim_p18f1220

${RT} a2d sim_p10f222

${RT} a2d sim_p18f26k22

${RT} psp sim_p18f452

${RT} psp sim_p18f6520

${RT} psp sim_p16f871

${RT} ttl sim_ttl377

${RT} ttl sim_ttl165

${RT} ttl sim_ttl595

${RT} ccp sim_ccp_12f683

${RT} ccp sim_ccp_877a

${RT} ccp sim_ccp_819

${RT} ccp sim_pwm_12f683

${RT} ccp sim_pwm_877a

${RT} ccp sim_pwm_6520

${RT} ccp sim_pwm_26k22

${RT} epwm sim

${RT} ccp_628 sim

${RT} wavegen sim

${RT} spi sim_p16f88

${RT} spi sim_p18f242

${RT} spi sim_p16c62

${RT} i2c sim_p16f88

${RT} i2c sim_p16f819

${RT} i2c sim_p16f876a

${RT} port_stim sim_port_stim

${RT} p12f675	sim_p12f629
${RT} p12f675	sim_p12f675
${RT} p12f675	sim_p12f683

${RT} p12c509 sim_p12c509_reset

${RT} p16f84 sim_reset

${RT} wdt sim_wdt_10f200
${RT} wdt sim_wdt_16f88
${RT} wdt sim_nwdt_16f88

${RT} wdt sim_nwdt_16f1823
${RT} wdt sim_pwdt_16f1823

${RT} wdt sim_wdt_16f628
${RT} wdt sim_nwdt_16f628
${RT} wdt sim_nwdt_16f648a

${RT} wdt sim_wdt_16c64
${RT} wdt sim_nwdt_16c64

${RT} wdt sim_wdt_18f452
${RT} wdt sim_nwdt_18f452
${RT} wdt sim_wdt_18f4620
${RT} wdt sim_nwdt_18f4620
${RT} wdt sim_wdt_18f26k22
${RT} wdt sim_nwdt_18f26k22
${RT} wdt sim_wdt_18f14k22

${RT} intosc sim_p18f1220
${RT} intosc sim_p18f2321
${RT} intosc sim_p18f14k22
${RT} intosc sim_p18f26k22

${RT} p16f676 sim_p16f676
${RT} p16f676 sim_reset
${RT} p16f690 sim_epwm 
${RT} p16f690 sim_eusart 
${RT} p16f690 sim_eusart_690 
${RT} p16f690 sim_nwdt_16f631
${RT} p16f690 sim_p16f690
${RT} p16f690 sim_wdt_16f677
${RT} p16f690 sim_wdt_16f685

${RT}       p16f684 sim_a2d_684
${RT}       p16f684 sim_compar_684
${RT}       p16f684 sim_epwm
${RT}       p16f684 sim_nwdt_16f684
${RT}       p16f684 sim_reset
${RT}       p16f684 sim_wdt_16f684

${RT}	    p16f6xx sim_p16f610
${RT}	    p16f6xx sim_p16f616

${RT}       p1xf18xx sim_p12f1822
${RT}       p1xf18xx sim_p12f1822_usart
${RT}       p1xf18xx sim_p16f1503
${RT}       p1xf18xx sim_p16f1503_nco
${RT}       p1xf18xx sim_p16f1823
${RT}       p1xf18xx sim_p16f1823_comp
${RT}       p1xf18xx sim_p16f1823_i2c
${RT}       p1xf18xx sim_p16f1823_i2c_v2
${RT}       p1xf18xx sim_p16f1823_spi
${RT}       p1xf18xx sim_p16f1825
${RT}       p1xf18xx sim_p16f1705
${RT}       p1xf18xx sim_p16f1705_clc
${RT}       p1xf18xx sim_p16f1705_cog
${RT}       p1xf18xx sim_zcd_f1705
${RT}       p1xf18xx sim_p16f1705_comp
${RT}       p1xf18xx sim_p16f1709
${RT}       p1xf18xx sim_p16f1709_cog
${RT}       p1xf18xx sim_p16f1709_i2c
${RT}       p1xf18xx sim_p16f1788_comp

${RT}	p18f26k22 sim
${RT}	p16f88x sim
${RT}	p16f91x sim
${RT}   p10f32x sim
grep ERR */*log
