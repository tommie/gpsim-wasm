
   ;;  Switch test 
   ;;
   ;;  Test switch with capacitance and without capacitance


	list    p=16f877                ; list directive to define processor
	include <p16f877.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros


;------------------------------------------------------------------------
; gpsim command
.command macro x
  .direct "C", x
  endm


;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA
temp            RES     1
temp1		RES	1
temp2		RES	1

  GLOBAL done



;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlw  high  start               ; load upper byte of 'start' label
        movwf  PCLATH                    ; initialize PCLATH
        goto   start                     ; go to beginning of program

INT_VECTOR   CODE    0x004               ; interrupt vector location
	nop

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE


;----------------------------------------------------------------------
; gpsim configuration script
;
; The configuration environment consists of a switch SW1 connecting
; portb0 and portc0 to portc1. Resistors on either side of the switch
; ensure that the nodes are never floating and allow the switch voltage
; to be checked.
;
; SW2.A is connected to the same node as SW1.A and SW2.B is connected to
; portc2. This allows checking of 2 switch infinite regression bug.
;
; Portb0 is used to drive the switch while portc 0-2 monitor the results
;
;               PU1   SW1
;                |   A \  B
;  portb0  >--+--+----O \O-----+--> portc1
;             |                |
;  portc0  <--+                |
;             |    A \  B     PD1
;             |-----O \O----------> portc2
;                   SW2
;
; The pull up and pull down resistors are modelled as parallel RC networks:
;
;
;           5V
;            ^                 x
;            |                 |
;         +--+--+           +--+--+
;         |     |           |     |
;      R  \    === C     R  \    === C
;         /     |           /     |
;         |     |           |     |
;         +--+--+           +--+--+
;            |                 |
;            x                ///
;
;        Pull up            Pull Down
;
; The 'x' is connection point.
;

   ;# Module libraries:
   .sim "module library libgpsim_modules"
   ;# The processor's reference designator is U1
   .sim "U1.xpos = 48.0"
   .sim "U1.ypos = 36.0"


   .sim "module load switch SW1"
   .sim "SW1.state=false"
   .sim "SW1.xpos = 216.0"
   .sim "SW1.ypos = 156.0"
   .sim "SW1.Ropen = 1.0e8"

   .sim "module load pullup PU1"
   .sim "PU1.resistance=1.0e8"
   .sim "PU1.capacitance=2.00e-07"
   .sim "PU1.xpos = 204.0"
   .sim "PU1.ypos = 36.0"

   .sim "module load pulldown PD1"
   .sim "PD1.resistance=5000."
   .sim "PD1.capacitance=4.00e-06"
   .sim "PD1.xpos = 204.0"
   .sim "PD1.ypos = 96.0"

   .sim "module load switch SW2"
   .sim "SW2.state=open"
   .sim "SW2.xpos = 216.0"
   .sim "SW2.ypos = 228.0"
   .sim "SW2.Ropen = 1.0e6"

    ;# Connections:
   .sim "node nb0"
   .sim "attach nb0 SW1.A portb0 portc0 PU1.pin SW2.A"

   .sim "node nc0"
   .sim "attach nc0 SW1.B PD1.pin portc1"

   .sim "node nc1"
   .sim "attach nc1 SW2.B portc2"

   .sim "frequency 10.0e6"

    ;# Scope monitoring
   .sim "scope.ch0='portb0'"
   .sim "scope.ch1='portc0'"
   .sim "scope.ch2='portc1'"
   .sim "scope.ch3='portc2'"

;# End of configuration
;
;------------------------------------------------------------------------


start

	BSF	STATUS,RP0

	movlw	0x07		; RC2,RC1 and RC0 are inputs
	movwf	TRISC
	CLRF	TRISB^0x80	;Port B is an output

	BCF 	STATUS,RP0

   ; The switch is open and portb0 is driving one side of it and portc0.
   ; The other side goes to portc1 and is pulled up by the pullup resistor
   ; Toggling portb0 should have no effect on portc1

	BCF	PORTB,0

	; both sides of switch should be 0
   .assert "(portc & 3) == 0, 'SW open, both sides 0'" 
	nop

	bsf	PORTB,0
        call    sdelay
	nop

   .assert "(portc & 3) == 1, 'SW open, drive high'"	; drive side only high
	nop

   ; Close the switch because of capacitance portc1 will go high after a delay:	
   ; R=145, C=4.2e-6 TC=6.11e-4 or 1527 cycles 0-2 volts requires 0.51 Tc
   .command "SW1.state=true"
	nop
        nop

	; portc0 should be same as portc1
   .assert "(portc & 3) == 0, 'SW1 closed, cap holds low'"	
 	nop
 	nop

       movlw   1	; wait 1280 cycles
       call    delay
	nop

   .assert "(portc & 3) == 3, 'Sw1 closed, after < 1TC'"	
	nop

       movlw   8	; let voltage go all the way high
       call    delay

;  drive portb low, voltages should from ~5 to 1 volts in 1.6 TC
;  or 2443 cycles, but because of RC step need 3 delay loops
	BCF	PORTB,0		; change drive voltage



       movlw   3
       call    delay

   .assert "(portc & 3) == 0, 'drive low 1.6TC'"   ; both sides now low
	nop

       movlw   6	; let voltages go to 0
       call    delay


;
;	test capacitance delay on drive transition
;
	BSF	PORTB,0		; drive high again
        movlw	9
	call	delay

   .assert "(portc & 3) == 3, 'prepare switch open'"	; make sure both sides are high
	nop

   ; Open the switch, the time constant for the floating side is 0.02 sec.
   ; At 10 MHz oscillator frequency the time constant is 50,000 instruction
   ; cycles. To go from 5 to 0.5 volts requires 2.3 time constants or
   ; 1.15e5 cycles. As delay loop is 1277 want to delay 90 loops (5A hex).
   ; Note, although 1 Volt is the h2l transition voltage at 1.6 time constants,
   ; we miss this.
   ;
   ; The drive side time constant is 150 * 2e-7 = 3e-5 sec or 75 cycles
   ; so drive side goes low as soon as drive does.
   ;
   .command "SW1.state=false"
	nop

	; driven side still driven, floating side
	; held by capacitance.

   .assert "(portc & 3) == 3, 'SW1 open driving high'"   
	nop

	BCF	PORTB,0

	; C = 0.2 uF R = 150 so time constant = 30 uS
	; allow 77 uS (10Mhz clock) for portc0 to settle
	movlw	0x40
	movwf   temp1
        decfsz  temp1,f
         goto   $-1
	nop

;  C = 4 uF R = 5000 so time constant = 20 mS 
;  for Fosc = 10MHz clock = 50,000 instuction cycles
;  require ~ 2 time constants to reach low state


   .assert "(portc & 3) == 2, ' SW1 open driving low'"
	nop
        movlw   0x29    ; delay 52,480 cycles

	call	delay

   .assert "(portc & 3) == 2, 'SW1 open float still high'"
	nop

	movlw   0x31    ; delay another 62,720 cycles
	call	delay
	nop

   .assert "(portc & 3) == 0, 'SW1 open float low after 115215 cycles'"
	nop

;	Turn off Capacitance to test DC behaviour
;
   .command "PD1.capacitance=0.0"
	nop
   .command "PU1.capacitance=0."
	nop

   .assert "(portc & 3) == 0"
	nop

	BSF	PORTB,0		; Drive one side of switch high
   .assert "(portc & 3) == 1, 'Drive side high C=0'"
	nop

   ; Close the switch:	
   .command "SW1.state=true"
	nop

   .assert "(portc & 3) == 3, 'Both sides high C=0'" 
	nop

	BCF	PORTB,0
   .assert "(portc & 3) == 0, 'Both sides low C=0'"
	nop

   ; Close the SW2 switch, portc2 should be same as portc0 and portc1

	BSF	PORTB,0		; Drive one side of switch high

   .command "SW2.state=closed"
	nop

   .assert "(portc & 7) == 7, '2 Switch drive high'"
	nop

	BCF	PORTB,0
   .assert "(portc & 7) == 0, '2 switch drive low'"
	nop

   ; Now test very large switch resistance
   .command "SW1.state=false"
        nop
   .command "SW2.state=open"
        nop
	BCF	PORTB,0

   .assert "(portc & 3) == 0, 'Both sides low C=0 Starting Large Ron'"
	nop

   .command "SW1.Rclosed=1e4"
	nop
   .command "PD1.capacitance=0.50e-06"
	nop
   .assert "(portc & 3) == 0, 'SW1.Rclosed = 10k'"
	nop

   .command "SW1.state=true"

   ; Now with the switch closed, the switch resistance and 
   ; the pull down resistance form a voltage divider:	
   ;
   ;                10k
   ;  portb0 -----/\/\/\---+----+------ portc0
   ;                       \    |
   ;                       /    |
   ;                   5k  \   === 4uF
   ;                       /    |
   ;                       |    |
   ;                      ///  ///
   ;
   ;  When portb0 is driven to 5.0V, portc0 will rise to 5/3=1.66V

	BSF	PORTB,0		; Drive one side of switch high

	movlw   30
	call    delay

	BCF	PORTB,0		; Drive the switch low again

	movlw   30
	call    delay

   ; Remove the pull down resistor by making its value very large
   ; Now when we drive portb0 high, the other side can rise to 5.0V

   .command "PD1.resistance=1.0e8"
	nop

	BSF	PORTB,0		; Drive one side of switch high

	movlw   9
	call    delay

	BCF	PORTB,0		; Drive the switch low again

	movlw   9
	call    delay

	nop

done:

  .assert  "'*** PASSED Switch Test on 16f877'"
	goto	$

FAILED:
  .assert  "'*** FAILED Switch Test on 16f877'"
	goto	$


delay		; W=9 = delay about 11,500 cycles or  1.2 ms at 10 Mhz
        clrwdt
       movwf	temp2
       clrf    temp1
delay_loop
        decfsz  temp1,f
         goto   $+2
        decfsz  temp2,f
         goto   delay_loop
        return

sdelay		; delay about 100 cycles or  40 us at 10 Mhz
        clrwdt
	movlw	0x20
	movwf   temp1
        decfsz  temp1,f
         goto   $-1
        return


  end
