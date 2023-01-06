
   ;;  Comparator test 
   ;;
   ;;  The purpose of this program is to verify the
   ;;  comparator module of the processor


	list    p=16f684               ; list directive to define processor
	include <p16f684.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

        __CONFIG  _EXTRC_OSC_NOCLKOUT

;------------------------------------------------------------------------
; gpsim command
.command macro x
  .direct "C", x
  endm

CMODE0                         EQU     H'0000'
CMODE1                         EQU     H'0001'
CMODE2                         EQU     H'0002'
CMODE3                         EQU     H'0003'
CMODE4                         EQU     H'0004'
CMODE5                         EQU     H'0005'
CMODE6                         EQU     H'0006'
CMODE7                         EQU     H'0007'


;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA_SHR
temp            RES     1

w_temp          RES     1
status_temp     RES     1

  GLOBAL done1


;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlw  high  start               ; load upper byte of 'start' label
        movwf  PCLATH                    ; initialize PCLATH
        goto   start                     ; go to beginning of program


;------------------------------------------------------------------------
;
;  Interrupt Vector
;
;------------------------------------------------------------------------
                                                                                
INT_VECTOR   CODE    0x004               ; interrupt vector location
                                                                                
        movwf   w_temp
        swapf   STATUS,W
        movwf   status_temp

       btfsc   PIR1,C1IF 
	 goto icm1
	goto exit_int

icm1:
	bcf PIR1,C1IF

	goto done1

exit_int:
                                                                                
        swapf   status_temp,w
        movwf   STATUS
        swapf   w_temp,f
        swapf   w_temp,w
        retfie
                                                                                

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start

  .sim "module library libgpsim_modules"
  .sim "p16f684.xpos = 60"
  .sim "p16f684.ypos = 84"

  .sim "module load pullup C1P"
  .sim "C1P.capacitance = 0"
  .sim "C1P.resistance = 10000"
  .sim "C1P.voltage = 1"
  .sim "C1P.xpos = 72"
  .sim "C1P.ypos = 24"

  .sim "module load pullup C1N"
  .sim "C1N.capacitance = 0"
  .sim "C1N.resistance = 10000"
  .sim "C1N.voltage = 2.5"
  .sim "C1N.xpos = 228"
  .sim "C1N.ypos = 36"

  .sim "module load pullup C2P"
  .sim "C2P.capacitance = 0"
  .sim "C2P.resistance = 10000"
  .sim "C2P.voltage = 2.8"
  .sim "C2P.xpos = 252"
  .sim "C2P.ypos = 96"

  .sim "module load pullup C2N"
  .sim "C2N.capacitance = 0"
  .sim "C2N.resistance = 10000"
  .sim "C2N.voltage = 2"
  .sim "C2N.xpos = 228"
  .sim "C2N.ypos = 144"

  .sim "node c1p"
  .sim "attach c1p C1P.pin porta0"
  .sim "node c1n"
  .sim "attach c1n C1N.pin porta1"
  .sim "node c2p"
  .sim "attach c2p C2P.pin portc0"
  .sim "node c2n"
  .sim "attach c2n C2N.pin portc1"

  .assert "cmcon0 == 0x00, 'FAILED 16f684 CMCON0 POR'"	; Rest value
	nop
	BANKSEL	PIR1
	clrf	PIR1
        movlw	CMODE7		; turn off comparator 
        movwf	CMCON0
	BANKSEL ANSEL
	clrf	ANSEL
	movlw	0x0f		; RA3-RA0 are inputs
	movwf	TRISA
	movlw	0xAB		; enable Vref 11 low range 2.29v
	movwf	VRCON
;
;	test mode 2 - Four Inputs muxed to two comparators with Vref
;
        movlw	CMODE2		; Comparator mux reference
	BANKSEL CMCON0
	; C1IN- = 2.5 + = 2.29	out = 0
	; C2IN- = 2.0 + = 2.29  out = 1
        movwf	CMCON0
  .assert "(cmcon0 & 0xc0) == 0x80, 'FAILED 16f684 m2 C1OUT=0 C2OUT=1'"
	nop
  .assert "(pir1 & 0x18) == 0x10, 'FAILED 16f684 C1if=0 C2IF=1'"
	nop
	clrf	PIR1
	movlw	0x8B		; enable Vref 11 high range 2.92v
        BANKSEL VRCON
	movwf	VRCON
	; C1IN- = 2.5 + = 2.92	out = 1
	; C2IN- = 2.0 + = 2.92  out = 1
  .assert "(cmcon0 & 0xc0) == 0xc0, 'FAILED 16f684 m2 C1OUT=1 C2OUT=1'" 
	nop
	bcf	OPTION_REG,T0CS		; Timer not from RA4
	movlw   0x03
	movwf	TRISC

	BCF 	STATUS,RP0	; Select bank 0 
  .assert "(cmcon0 & 0xc0) == 0xc0, 'FAILED 16f684 m2 C1OUT=1 C2OUT=1'"
	nop
  .assert "(pir1 & 0x18) == 0x08, 'FAILED 16f684 C1IF=1 C2IF=0'"
	nop
	bcf	PIR1,C1IF
  .assert "(pir1 & 0x18) == 0x00, 'FAILED 16f684 C1IF=0 C2IF=0'"
	nop
  .command "C1N.voltage = 3.0"	; drive comp1 low (C1IN- > Vref)
	nop
  .assert "(cmcon0 & 0xc0) == 0x80, 'FAILED 16f684 m2 C1OUT=0 C2OUT=1'"
        nop
  .assert "(pir1 & 0x18) == 0x08, 'FAILED 16f684 C1IF=1'"
	nop
	bcf	PIR1,C1IF
	bsf	CMCON0,C1INV		; invert output
	bsf	CMCON0,C2INV
  .assert "(cmcon0 & 0xc0) == 0x40, 'FAILED 16f684 m2 invert C1OUT=1 C2OUT=0'"
	nop
  .assert "(pir1 & 0x18) == 0x18, 'FAILED 16f684 C1IF=1'"
	nop

	bcf	PIR1,C1IF
	bcf	PIR1,C2IF
  .command "C2N.voltage = 3.5"		; C2IN- > Vref
	nop
  .command "C1N.voltage = 2.5"		; C1IN- < Vref
	nop
  .command "C1P.voltage = 3.1"		; C1IN+ > Vref
	nop
  .assert "(cmcon0 & 0xc0) == 0x80, 'FAILED 16f684 m2 new inputs C1OUT=0 C2OUT=1'"
	nop
  .assert "(pir1 & 0x18) == 0x18, 'FAILED 16f684 C1IF=1 C2IF=1'"
	nop
	bsf	CMCON0,CIS	; switch pins
  .assert "(cmcon0 & 0xc0) == 0x40, 'FAILED 16f684 m2 new inputs C1OUT=1 C2OUT=0'"
	nop

        movlw	CMODE2		; Comparator mux reference
        movwf	CMCON0
  .assert "(cmcon0 & 0xc0) == 0x40, 'FAILED 16f684 m6 invert normal C1OUT=1 C2OUT=0'"
	nop
	clrf	PIR1
	BANKSEL PIE1
	bsf	PIE1,C1IE	; enable Comparator interrupts
	bsf	PIE1,C2IE	; enable Comparator interrupts
	bsf	INTCON,PEIE	; enable Peripheral interrupts
	bsf	INTCON,GIE	; and global interrupts
	BANKSEL CMCON0
	bsf	CMCON0,C1INV	; generate an interrupt
	nop
        nop
  .assert  "'*** FAILED Comparator no interrupt 16f684'"
	nop
	goto	$
done1:
;
;	mode1 - 3 inputs mux to two comparators
;
;	C2IN+(2.5V) to both comparator +
;       C1IN-(3.1V), C1IN+(2.1V) muxed to C1 - by CIS
;	C2in-(1.5V) connected to C2 -
  .command "C2P.voltage = 2.5"
	nop
  .command "C2N.voltage = 1.5"
	nop
  .command "C1P.voltage = 2.1"
	nop
  .command "C1N.voltage = 3.1"
	nop

        movlw	CMODE1
        movwf	CMCON0
  .assert "(cmcon0 & 0xc0) == 0x80, 'FAILED 16f684 m1 CIS=0 C1OUT=0 C2OUT=1'"
	nop
	bsf	CMCON0,CIS
  .assert "(cmcon0 & 0xc0) == 0xc0, 'FAILED 16f684 m1 CIS=1 C1OUT=1 C2OUT=1'"
	nop
;
;	mode 4 - Two independent Comparators
;
;		Both Comparator true
;       C1IN-(2.1V), C1 -
;	C1IN+(3.1V)  C1 + thus C1 out=1
;	C2in-(1.5V) connected to C2 -
;	C2IN+(2.5V) C2 + thus C2 out=1
  .command "C2P.voltage = 2.5"
	nop
  .command "C2N.voltage = 1.5"
	nop
  .command "C1P.voltage = 3.1"
	nop
  .command "C1N.voltage = 2.1"
	nop

        movlw	CMODE4
        movwf	CMCON0
  .assert "(cmcon0 & 0xc0) == 0xc0, 'FAILED 16f684 m4 CIS=0 C1OUT=1 C2OUT=1'"
	nop
	bsf	CMCON0,CIS
  .assert "(cmcon0 & 0xc0) == 0xc0, 'FAILED 16f684 m4 CIS=1 C1OUT=1 C2OUT=1'"
	nop
;	C2+ < C2- C2 out=0
  .command "C2P.voltage = 1.4"
	nop
	nop
  .assert "(cmcon0 & 0xc0) == 0x40, 'FAILED 16f684 m4 C1OUT=1 C2OUT=0'"
	nop
;	C1+ < C1- C1 out=0
;	C2+ > C2- C2 out=1
  .command "C1P.voltage = 2.0"
	nop
  .command "C2P.voltage = 2.5"
	nop
  .assert "(cmcon0 & 0xc0) == 0x80, 'FAILED 16f684 m4 C1OUT=0 C2OUT=1'"
	nop
;		both low
;	C2+ < C2- C2 out=0
  .command "C2P.voltage = 1.4"
	nop
  .assert "(cmcon0 & 0xc0) == 0x00, 'FAILED 16f684 m4  C1OUT=0 C2OUT=0'"
	nop

;
;	mode 3 - Two common reference Comparators
;
;	C2IN+(2.5V) to both comparator +
;       C1IN-(3.1V),connected to C1 - out=0
;	C2in-(1.5V) connected to C2 - out=1
  .command "C2P.voltage = 2.5"
	nop
  .command "C2N.voltage = 1.5"
	nop
  .command "C1P.voltage = 2.1"
	nop
  .command "C1N.voltage = 3.1"
	nop

        movlw	CMODE3
        movwf	CMCON0
  .assert "(cmcon0 & 0xc0) == 0x80, 'FAILED 16f684 m3 CIS=0 C1OUT=0 C2OUT=1'"
	nop
	bsf	CMCON0,CIS
  .assert "(cmcon0 & 0xc0) == 0x80, 'FAILED 16f684 m3 CIS=1 C1OUT=0 C2OUT=1'"
	nop
;	C2+ > C1- C1 out=1
;	C2+ < C2- C2 out=0
  .command "C1N.voltage = 1.7"
	nop
  .command "C2N.voltage = 2.7"
	nop
  .assert "(cmcon0 & 0xc0) == 0x40, 'FAILED 16f684 m3 C1OUT=1 C2OUT=0'"
	nop
;	C2+ > C1- C1 out=1
;	C2+ > C2- C2 out=1
  .command "C2N.voltage = 2.3"
	nop
  .assert "(cmcon0 & 0xc0) == 0xc0, 'FAILED 16f684 m3 C1OUT=1 C2OUT=1'"
	nop

;
;	mode 5 - One independent Comparator 
;
        movlw	CMODE5
        movwf	CMCON0
  .assert "(cmcon0 & 0xc0) == 0x80, 'FAILED 16f684 m5 C1OUT=0 C2OUT=1'"
	nop

;
;	mode 6 two common reference Comparators with Outputs
;
	BANKSEL TRISA
	bcf	TRISA,2
	BANKSEL CMCON0
        movlw	CMODE6
        movwf	CMCON0

  .assert "(cmcon0 & 0xc0) == 0xc0, 'FAILED 16f684 m6 C1OUT=1 C2OUT=1'"
	nop
  .assert "(porta & 0x04) == 0x04,  'FAILED 16f684 m6 porta2=1'"
	nop
  .assert "(portc & 0x10) == 0x10,  'FAILED 16f684 m6 portc4=1'"
	nop

;	C2+ < C1- out=0
;	C2+ < C2- out=0
  .command "C2P.voltage = 1.5"
	nop
  .assert "(cmcon0 & 0xc0) == 0x00, 'FAILED 16f684 m6 C1OUT=0 C2OUT=0'"
	nop
  .assert "(porta & 0x04) == 0x00,  'FAILED 16f684 m6 porta2=0'"
	nop
  .assert "(portc & 0x10) == 0x00,  'FAILED 16f684 m6 portc4=0'"
	nop

  .assert  "'*** PASSED Comparator on 16f684'"
	goto	$

FAILED:
  .assert  "'*** FAILED Comparator no interrupt 16f684'"
	goto	$



  end
