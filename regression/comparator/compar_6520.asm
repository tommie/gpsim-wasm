
   ;;  Comparator test 
   ;;
   ;;  The purpose of this program is to verify the
   ;;  comparator module of the processor


	list    p=18f6520               ; list directive to define processor
	include <p18f6520.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

        CONFIG  OSC=RC

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
GPR_DATA                UDATA_SHR 0
temp            RES     1

w_temp          RES     1
status_temp     RES     1
cmp_int		RES	1

  GLOBAL done1


;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
	bra	start			; go to beginning of program


;------------------------------------------------------------------------
;
;  Interrupt Vector
;
;------------------------------------------------------------------------
                                                                                
INT_VECTOR   CODE    0x008               ; interrupt vector location
                                                                                
        movwf   w_temp
        swapf   STATUS,W
        movwf   status_temp

       btfsc   PIR2,CMIF 
	 goto icm1
	goto exit_int

icm1:
	bcf 	PIR2,CMIF
	bsf	cmp_int,0


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
  .sim "p18f6520.xpos = 60"
  .sim "p18f6520.ypos = 84"

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
  .sim "attach c1p C1P.pin portf5"
  .sim "node c1n"
  .sim "attach c1n C1N.pin portf6"
  .sim "node c2p"
  .sim "attach c2p C2P.pin portf3"
  .sim "node c2n"
  .sim "attach c2n C2N.pin portf4"

  .assert "cmcon == 0x07, 'FAILED 18f6520 CMCON POR'"	; Rest value
	nop
	clrf	PIR2
        movlw	CMODE7		; turn off comparator 
        movwf	CMCON
	movlw	0xff
	movwf	ADCON1		; analog off all channels
	movlw	0x78		; RF3-RF6 are inputs
	movwf	TRISF
;	mode 1 - one independant comparator with output
	movlw	CMODE1
	movwf	CMCON
	call	c1_out
	nop
;
;	mode 2 - two independant comparators
;
        movlw	CMODE2
        movwf	CMCON
	call	ind_comp
	nop
;
;	Two Independent Comparators with Outputs
;
        movlw   CMODE3
        movwf   CMCON
        call    ind_comp_out
        nop


;	Two Common Reference(C1+) Comparators
;
        movlw   CMODE4
        movwf   CMCON
        call    ref_com
        nop

;	Two Common Reference(C1+) Comparators with output
;
        movlw   CMODE5
        movwf   CMCON
        call    ref_com_out
        nop

;
;	test mode - Four Inputs muxed to two comparators with Vref
;
        movlw	CMODE6		; Comparator mux reference
        movwf	CMCON
	call	mux_vref
	nop

  .assert  "'*** PASSED Comparator on 18f6520'"
	goto	$


;	One Independent Comparator with Output
c1_out:
;	C1IN- (2.5)
;	C1IN+ (1.0) thus C1 out=0
;	C2in- (2.0) 
;	C2IN+ (2.8) but C2 off so out=0
  .assert "(cmcon & 0xc0) == 0x00, '*** FAILED 18f6520 c1_out, c1out=0'"
	nop
  .assert "(portf & 0x06) == 0x00, '*** FAILED 18f6520 c1_out, OUT1=0 OUT2=0'"
	nop
	bsf	CMCON,C1INV
  .assert "(cmcon & 0xc0) == 0x40, '*** FAILED 18f6520 c1_out, c1out=1'"
	nop
  .assert "(portf & 0x06) == 0x04, '*** FAILED 18f6520 c1_out, OUT1=1 OUT2=0'"
	nop
;	C1IN- (2.5)
;	C1IN+ (3.1) C1+ > C1- with invert thus C1 out=0
  .command "C1P.voltage = 3.1"
	nop
  .assert "(cmcon & 0xc0) == 0x00, '*** FAILED 18f6520 c1_out INVERT , c1out=0'"
	nop
  .assert "(portf & 0x06) == 0x00, '*** FAILED 18f6520 c1_out, INVERT OUT1=0 OUT2=0'"
	nop

	return


;	Two independent Comparators
ind_comp:
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

  .assert "(cmcon & 0xc0) == 0xc0, 'FAILED 18f6520 2 ind comp CIS=0 C1OUT=1 C2OUT=1'"
	nop
	bsf	CMCON,CIS
  .assert "(cmcon & 0xc0) == 0xc0, 'FAILED 18f6520 2 ind comp CIS=1 C1OUT=1 C2OUT=1'"
	nop
;	C2+ < C2- C2 out=0
  .command "C2P.voltage = 1.4"
	nop
	nop
  .assert "(cmcon & 0xc0) == 0x40, 'FAILED 18f6520 2 ind comp C1OUT=1 C2OUT=0'"
	nop
;	C1+ < C1- C1 out=0
;	C2+ > C2- C2 out=1
  .command "C1P.voltage = 2.0"
	nop
  .command "C2P.voltage = 2.5"
	nop
  .assert "(cmcon & 0xc0) == 0x80, 'FAILED 18f6520 2 ind comp C1OUT=0 C2OUT=1'"
	nop
;		both low
;	C2+ < C2- C2 out=0
  .command "C2P.voltage = 1.4"
	nop
  .assert "(cmcon & 0xc0) == 0x00, 'FAILED 18f6520 2 ind comp  C1OUT=0 C2OUT=0'"
	nop
	return

;	Two independent Comparators with outputs
ind_comp_out:
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

  .assert "(cmcon & 0xc0) == 0xc0, '*** FAILED 18f6520 ind_comp_out CIS=0 C1OUT=1 C2OUT=1'"
	nop
  .assert "(portf & 0x06) == 0x06, '*** FAILED 18f6520 ind_comp_out CIS=0 port C1OUT=1 C2OUT=1'"
	nop
	bsf	CMCON,CIS
  .assert "(cmcon & 0xc0) == 0xc0, '*** FAILED 18f6520 ind_comp_out CIS=1 C1OUT=1 C2OUT=1'"
	nop
  .assert "(portf & 0x06) == 0x06, '*** FAILED 18f6520 ind_comp_out CIS=1 port C1OUT=1 C2OUT=1'"
	nop
;	C2+ < C2- C2 out=0
  .command "C2P.voltage = 1.4"
	nop
	nop
  .assert "(cmcon & 0xc0) == 0x40, '*** FAILED 18f6520 ind_comp_out C1OUT=1 C2OUT=0'"
	nop
  .assert "(portf & 0x06) == 0x04, '*** FAILED 18f6520 ind_comp_out port C1OUT=1 C2OUT=0'"
	nop
;	C1+ < C1- C1 out=0
;	C2+ > C2- C2 out=1
  .command "C1P.voltage = 2.0"
	nop
  .command "C2P.voltage = 2.5"
	nop
  .assert "(cmcon & 0xc0) == 0x80, '*** FAILED 18f6520 ind_comp_out C1OUT=0 C2OUT=1'"
	nop
  .assert "(portf & 0x06) == 0x02, '*** FAILED 18f6520 ind_comp_out port C1OUT=0 C2OUT=1'"
	nop
;		both low
;	C2+ < C2- C2 out=0
  .command "C2P.voltage = 1.4"
	nop
  .assert "(cmcon & 0xc0) == 0x00, '*** FAILED 18f6520 ind_comp_out  C1OUT=0 C2OUT=0'"
	nop
  .assert "(portf & 0x06) == 0x00, '*** FAILED 18f6520 ind_comp_out port C1OUT=0 C2OUT=0'"
	nop
	return


;
;	Four Inputs muxed to two comparators with Vref
;
mux_vref:
  .command "C1P.voltage = 1.0"
	nop
  .command "C1N.voltage = 2.5"
	nop
  .command "C2P.voltage = 2.8"
	nop
  .command "C2N.voltage = 2.0"
	nop
	; C1IN- = 2.5 + = 2.29	out = 0
	; C2IN- = 2.0 + = 2.29  out = 1
	movlw	0xAB		; enable Vref 11 low range 2.29v
	movwf	CVRCON
  .assert "(cmcon & 0xc0) == 0x80, 'FAILED 18f6520 mux with vref C1OUT=0 C2OUT=1'"
	nop
  .assert "(pir2 & 0x40) == 0x40, 'FAILED 18f6520 mux with vref CMIF=1'"
	nop
	clrf	PIR2
	movlw	0x8B		; enable Vref 11 high range 2.92v
        BANKSEL CVRCON
	movwf	CVRCON
	; C1IN- = 2.5 + = 2.92	out = 1
	; C2IN- = 2.0 + = 2.92  out = 1
  .assert "(cmcon & 0xc0) == 0xc0, 'FAILED 18f6520 mux with vref C1OUT=1 C2OUT=1'" 
	nop
	movlw   0x03
	movwf	TRISC

  .assert "(cmcon & 0xc0) == 0xc0, 'FAILED 18f6520 mux with vref C1OUT=1 C2OUT=1'"
	nop
  .assert "(pir2 & 0x40) == 0x40, 'FAILED 18f6520 CMIF=1'"
	nop
	bcf	PIR2,CMIF
  .assert "(pir2 & 0x40) == 0x00, 'FAILED 18f6520 CMIF=0'"
	nop
  .command "C1N.voltage = 3.0"	; drive comp1 low (C1IN- > Vref)
	nop
  .assert "(cmcon & 0xc0) == 0x80, 'FAILED 18f6520 mux with vref C1OUT=0 C2OUT=1'"
        nop
  .assert "(pir2 & 0x40) == 0x40, 'FAILED 18f6520 CMIF=1 C1 change'"
	nop
	bcf	PIR2,CMIF
	bsf	CMCON,C1INV		; invert output
	bsf	CMCON,C2INV
  .assert "(cmcon & 0xc0) == 0x40, 'FAILED 18f6520 mux with vref invert C1OUT=1 C2OUT=0'"
	nop
  .assert "(pir2 & 0x40) == 0x40, 'FAILED 40f6520 CMIF=1'"
	nop

	bcf	PIR2,CMIF
  .command "C2N.voltage = 3.5"		; C2IN- > Vref
	nop
  .command "C1N.voltage = 2.5"		; C1IN- < Vref
	nop
  .command "C1P.voltage = 3.1"		; C1IN+ > Vref
	nop
  .assert "(cmcon & 0xc0) == 0x80, 'FAILED 18f6520 mux with vref new inputs C1OUT=0 C2OUT=1'"
	nop
  .assert "(pir2 & 0x40) == 0x40, 'FAILED 40f6520 mux with vref CMIF=1 C1, C2 change'"
	nop
	bsf	CMCON,CIS	; switch pins
  .assert "(cmcon & 0xc0) == 0x40, 'FAILED 18f6520 mux with vref new inputs C1OUT=1 C2OUT=0'"
	nop

 	movlw	0xc7		; clear invert, CIS bits
	andwf	CMCON,F
  .assert "(cmcon & 0xc0) == 0x40, 'FAILED 18f6520 mux with vref invert normal C1OUT=1 C2OUT=0'"
	nop
	clrf	PIR2
	clrf	cmp_int
	bsf	PIE2,CMIE	; enable Comparator interrupts
	bsf	INTCON,PEIE	; enable Peripheral interrupts
	bsf	INTCON,GIE	; and global interrupts
	bsf	CMCON,C1INV	; generate an interrupt
	btfsc	cmp_int,0
	goto	done1
        nop
  .assert  "'*** FAILED Comparator no interrupt 18f6520'"
	nop
	goto	$
done1:
	return
;
;	Two common reference Comparators
;
;	C1IN+(2.5V) to both comparator +
;       C1IN-(3.1V),connected to C1 - out=0
;	C2in-(1.5V) connected to C2 - out=1
ref_com:
  .command "C2P.voltage = 0.5"
	nop
  .command "C2N.voltage = 1.5"
	nop
  .command "C1P.voltage = 2.5"
	nop
  .command "C1N.voltage = 3.1"
	nop

  .assert "(cmcon & 0xc0) == 0x80, '*** FAILED 18f6520 ref_com CIS=0 C1OUT=0 C2OUT=1'"
	nop
	bsf	CMCON,CIS
  .assert "(cmcon & 0xc0) == 0x80, '*** FAILED 18f6520 ref_com CIS=1 C1OUT=0 C2OUT=1'"
	nop
;	C2+ > C1- C1 out=1
;	C2+ < C2- C2 out=0
  .command "C1N.voltage = 1.7"
	nop
  .command "C2N.voltage = 2.7"
	nop
  .assert "(cmcon & 0xc0) == 0x40, '*** FAILED 18f6520 ref_com C1OUT=1 C2OUT=0'"
	nop
;	C2+ > C1- C1 out=1
;	C2+ > C2- C2 out=1
  .command "C2N.voltage = 2.3"
	nop
  .assert "(cmcon & 0xc0) == 0xc0, '*** FAILED 18f6520 ref_com C1OUT=1 C2OUT=1'"
	nop
	return
;
;	Two common reference Comparators with outputs
;
;	C1IN+(2.5V) to both comparator +
;       C1IN-(3.1V),connected to C1 - out=0
;	C2in-(1.5V) connected to C2 - out=1
ref_com_out:
  .command "C2P.voltage = 0.5"
	nop
  .command "C2N.voltage = 1.5"
	nop
  .command "C1P.voltage = 2.5"
	nop
  .command "C1N.voltage = 3.1"
	nop

  .assert "(cmcon & 0xc0) == 0x80, '*** FAILED 18f6520 ref_com_out CIS=0 C1OUT=0 C2OUT=1'"
	nop
  .assert "(portf & 0x06) == 0x02, '*** FAILED 18f6520 ref_com_out CIS=0 port C1OUT=0 C2OUT=1'"
	nop
	bsf	CMCON,CIS
  .assert "(cmcon & 0xc0) == 0x80, '*** FAILED 18f6520 ref_com_out CIS=1 C1OUT=0 C2OUT=1'"
	nop
  .assert "(portf & 0x06) == 0x02, '*** FAILED 18f6520 ref_com_out CIS=1 port C1OUT=0 C2OUT=1'"
	nop
;	C2+ > C1- C1 out=1
;	C2+ < C2- C2 out=0
  .command "C1N.voltage = 1.7"
	nop
  .command "C2N.voltage = 2.7"
	nop
  .assert "(cmcon & 0xc0) == 0x40, '*** FAILED 18f6520 ref_com_out C1OUT=1 C2OUT=0'"
	nop
  .assert "(portf & 0x06) == 0x04, '*** FAILED 18f6520 ref_com_out CIS=0 port C1OUT=1 C2OUT=0'"
	nop
;	C2+ > C1- C1 out=1
;	C2+ > C2- C2 out=1
  .command "C2N.voltage = 2.3"
	nop
  .assert "(cmcon & 0xc0) == 0xc0, '*** FAILED 18f6520 ref_com_out C1OUT=1 C2OUT=1'"
	nop
  .assert "(portf & 0x06) == 0x06, '*** FAILED 18f6520 ref_com_out port C1OUT=1 C2OUT=1'"
	nop
	return
  end
