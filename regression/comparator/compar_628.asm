
   ;;  Comparator test 
   ;;
   ;;  The purpose of this program is to verify the
   ;;  comparator module of the processor


	list    p=16f628               ; list directive to define processor
	include <p16f628.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros


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
cmp_int		RES	1

  GLOBAL done1


;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlw  high  start               ; load upper byte of 'start' label
        movwf  PCLATH                    ; initialize PCLATH
	goto	start			; go to beginning of program


;------------------------------------------------------------------------
;
;  Interrupt Vector
;
;------------------------------------------------------------------------
                                                                                
INT_VECTOR   CODE    0x004               ; interrupt vector location
                                                                                
        movwf   w_temp
        swapf   STATUS,W
        movwf   status_temp

       btfsc   PIR1,CMIF 
	 goto icm1
	goto exit_int

icm1:
	bcf 	PIR1,CMIF
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
  .sim "p16f628.xpos = 60"
  .sim "p16f628.ypos = 84"

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
  .sim "attach c1p C1P.pin porta3"
  .sim "node c1n"
  .sim "attach c1n C1N.pin porta0"
  .sim "node c2p"
  .sim "attach c2p C2P.pin porta2"
  .sim "node c2n"
  .sim "attach c2n C2N.pin porta1"

  .assert "cmcon == 0x00, 'FAILED 16f628 CMCON POR'"	; Rest value
	nop
	BANKSEL PIR1
	clrf	PIR1
        movlw	CMODE7		; turn off comparator 
	BANKSEL CMCON
        movwf	CMCON
	movlw	0x0f		; RA0-RA3 are inputs
	BANKSEL TRISA
	movwf	TRISA

;	mode 1 - one independant comparator with output
	movlw	CMODE1
	BANKSEL CMCON
	movwf	CMCON
	call	mux_3in
	nop
;
;	test mode - Four Inputs muxed to two comparators with Vref
;
        movlw	CMODE2		; Comparator mux reference
        movwf	CMCON
	call	mux_vref
	nop
;	Two Common Reference(C2+) Comparators
;
        movlw   CMODE3
        movwf   CMCON
        call    ref_com2
        nop

;
;	mode 4 - two independant comparators
;
        movlw	CMODE4
        movwf	CMCON
	call	ind_comp
	nop

;	C2 only
;
        movlw   CMODE5
        movwf   CMCON
        call    c2_only
        nop

;
;	Two Common Reference Comparators with Outputs
;
        movlw	CMODE6
        movwf	CMCON
        call    ref2_com_out
	nop

  .assert  "'*** PASSED Comparator on 16f628'"
	goto	$


;
;	3 inputs mux to two comparators
mux_3in:
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

  .assert "(cmcon & 0xc0) == 0x80, 'FAILED 16f684 mux_3in CIS=0 C1OUT=0 C2OUT=1'"
	nop
	bsf	CMCON,CIS
  .assert "(cmcon & 0xc0) == 0xc0, 'FAILED 16f684 mux_3in CIS=1 C1OUT=1 C2OUT=1'"
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

  .assert "(cmcon & 0xc0) == 0xc0, 'FAILED 16f628 2 ind comp CIS=0 C1OUT=1 C2OUT=1'"
	nop
	bsf	CMCON,CIS
  .assert "(cmcon & 0xc0) == 0xc0, 'FAILED 16f628 2 ind comp CIS=1 C1OUT=1 C2OUT=1'"
	nop
;	C2+ < C2- C2 out=0
  .command "C2P.voltage = 1.4"
	nop
	nop
  .assert "(cmcon & 0xc0) == 0x40, 'FAILED 16f628 2 ind comp C1OUT=1 C2OUT=0'"
	nop
;	C1+ < C1- C1 out=0
;	C2+ > C2- C2 out=1
  .command "C1P.voltage = 2.0"
	nop
  .command "C2P.voltage = 2.5"
	nop
  .assert "(cmcon & 0xc0) == 0x80, 'FAILED 16f628 2 ind comp C1OUT=0 C2OUT=1'"
	nop
;		both low
;	C2+ < C2- C2 out=0
  .command "C2P.voltage = 1.4"
	nop
  .assert "(cmcon & 0xc0) == 0x00, 'FAILED 16f628 2 ind comp  C1OUT=0 C2OUT=0'"
	nop
	return
;	One (C2) independent Comparators
c2_only:
;
;		Both Comparator true
;       C1IN-(2.1V), C1 -
;	C1IN+(3.1V)  C1 + but C1 off thus C1 out=0
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

  .assert "(cmcon & 0xc0) == 0x80, 'FAILED 16f628  c2 only CIS=0 C1OUT=0 C2OUT=1'"
	nop
	bsf	CMCON,CIS
  .assert "(cmcon & 0xc0) == 0x80, 'FAILED 16f628  c2 only CIS=1 C1OUT=0 C2OUT=1'"
	nop
;	C2+ < C2- C2 out=0
  .command "C2P.voltage = 1.4"
	nop
	nop
  .assert "(cmcon & 0xc0) == 0x00, 'FAILED 16f628 c2 only C1OUT=0 C2OUT=0'"
	nop
;	C1+ < C1- C1 out=0
;	C2+ > C2- C2 out=1
  .command "C1P.voltage = 2.0"
	nop
  .command "C2P.voltage = 2.5"
	nop
  .assert "(cmcon & 0xc0) == 0x80, 'FAILED 16f628 c2 only C1OUT=0 C2OUT=1'"
	nop
;		both low
;	C2+ < C2- C2 out=0
  .command "C2P.voltage = 1.4"
	nop
  .assert "(cmcon & 0xc0) == 0x00, 'FAILED 16f628 2 c2 only  C1OUT=0 C2OUT=0'"
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
	BANKSEL VRCON
	movwf	VRCON
  .assert "(cmcon & 0xc0) == 0x80, 'FAILED 16f628 mux with vref C1OUT=0 C2OUT=1'"
	nop
  .assert "(pir1 & 0x40) == 0x40, 'FAILED 16f628 mux with vref CMIF=1'"
	nop
	BANKSEL PIR1
	clrf	PIR1
	movlw	0x8B		; enable Vref 11 high range 2.92v
        BANKSEL VRCON
	movwf	VRCON
	; C1IN- = 2.5 + = 2.92	out = 1
	; C2IN- = 2.0 + = 2.92  out = 1
  .assert "(cmcon & 0xc0) == 0xc0, 'FAILED 16f628 mux with vref C1OUT=1 C2OUT=1'" 
	nop

  .assert "(cmcon & 0xc0) == 0xc0, 'FAILED 16f628 mux with vref C1OUT=1 C2OUT=1'"
	nop
  .assert "(pir1 & 0x40) == 0x40, 'FAILED 16f628 CMIF=1'"
	nop
	BANKSEL PIR1
	bcf	PIR1,CMIF
  .assert "(pir1 & 0x40) == 0x00, 'FAILED 16f628 CMIF=0'"
	nop
  .command "C1N.voltage = 3.0"	; drive comp1 low (C1IN- > Vref)
	nop
  .assert "(cmcon & 0xc0) == 0x80, 'FAILED 16f628 mux with vref C1OUT=0 C2OUT=1'"
        nop
  .assert "(pir1 & 0x40) == 0x40, 'FAILED 16f628 CMIF=1 C1 change'"
	nop
	bcf	PIR1,CMIF
	BANKSEL CMCON
	bsf	CMCON,C1INV		; invert output
	bsf	CMCON,C2INV
  .assert "(cmcon & 0xc0) == 0x40, 'FAILED 16f628 mux with vref invert C1OUT=1 C2OUT=0'"
	nop
  .assert "(pir1 & 0x40) == 0x40, 'FAILED 40f2321 CMIF=1'"
	nop

	BANKSEL PIR1
	bcf	PIR1,CMIF
  .command "C2N.voltage = 3.5"		; C2IN- > Vref
	nop
  .command "C1N.voltage = 2.5"		; C1IN- < Vref
	nop
  .command "C1P.voltage = 3.1"		; C1IN+ > Vref
	nop
  .assert "(cmcon & 0xc0) == 0x80, 'FAILED 16f628 mux with vref new inputs C1OUT=0 C2OUT=1'"
	nop
  .assert "(pir1 & 0x40) == 0x40, 'FAILED 40f2321 mux with vref CMIF=1 C1, C2 change'"
	nop
	BANKSEL CMCON
	bsf	CMCON,CIS	; switch pins
  .assert "(cmcon & 0xc0) == 0x40, 'FAILED 16f628 mux with vref new inputs C1OUT=1 C2OUT=0'"
	nop

 	movlw	0xc7		; clear invert, CIS bits
	andwf	CMCON,F
  .assert "(cmcon & 0xc0) == 0x40, 'FAILED 16f628 mux with vref invert normal C1OUT=1 C2OUT=0'"
	nop
	BANKSEL PIR1
	clrf	PIR1
	clrf	cmp_int
	BANKSEL PIE1
	bsf	PIE1,CMIE	; enable Comparator interrupts
	bsf	INTCON,PEIE	; enable Peripheral interrupts
	bsf	INTCON,GIE	; and global interrupts
  	BANKSEL CMCON
	bsf	CMCON,C1INV	; generate an interrupt
	btfsc	cmp_int,0
	goto	done1
        nop
  .assert  "'*** FAILED Comparator no interrupt 16f628'"
	nop
	goto	$
done1:
	return
;
;	Two common reference Comparators
;
;	C2IN+(2.5V) to both comparator +
;       C1IN-(3.1V),connected to C1 - out=0
;	C2in-(1.5V) connected to C2 - out=1
ref_com2:
  .command "C2P.voltage = 2.5"
	nop
  .command "C2N.voltage = 1.5"
	nop
  .command "C1P.voltage = 0.5"
	nop
  .command "C1N.voltage = 3.1"
	nop

  .assert "(cmcon & 0xc0) == 0x80, '*** FAILED 16f628 ref_com CIS=0 C1OUT=0 C2OUT=1'"
	nop
	bsf	CMCON,CIS
  .assert "(cmcon & 0xc0) == 0x80, '*** FAILED 16f628 ref_com CIS=1 C1OUT=0 C2OUT=1'"
	nop
;	C2+ > C1- C1 out=1
;	C2+ < C2- C2 out=0
  .command "C1N.voltage = 1.7"
	nop
  .command "C2N.voltage = 2.7"
	nop
  .assert "(cmcon & 0xc0) == 0x40, '*** FAILED 16f628 ref_com C1OUT=1 C2OUT=0'"
	nop
;	C2+ > C1- C1 out=1
;	C2+ > C2- C2 out=1
  .command "C2N.voltage = 2.3"
	nop
  .assert "(cmcon & 0xc0) == 0xc0, '*** FAILED 16f628 ref_com C1OUT=1 C2OUT=1'"
	nop
	return
;
;	Two common reference Comparators with outputs
;
;	C2IN+(2.5V) to both comparator +
;       C1IN-(3.1V),connected to C1 - out=0
;	C2in-(1.5V) connected to C2 - out=1
ref2_com_out:
  .command "C2P.voltage = 2.5"
	nop
  .command "C2N.voltage = 1.5"
	nop
  .command "C1P.voltage = 4.5"
	nop
  .command "C1N.voltage = 3.1"
	nop

	BANKSEL	TRISA		; RA3 is output
	bcf	TRISA,3
  .assert "(cmcon & 0xc0) == 0x80, '*** FAILED 16f628 ref2_com_out CIS=0 C1OUT=0 C2OUT=1'"
	nop
  .assert "(porta & 0x18) == 0x10, '*** FAILED 16f628 ref2_com_out CIS=0 port C1OUT=0 C2OUT=1'"
	nop
	BANKSEL CMCON
	bsf	CMCON,CIS
  .assert "(cmcon & 0xc0) == 0x80, '*** FAILED 16f628 ref2_com_out CIS=1 C1OUT=0 C2OUT=1'"
	nop
  .assert "(porta & 0x18) == 0x10, '*** FAILED 16f628 ref2_com_out CIS=1 port C1OUT=0 C2OUT=1'"
	nop
;	C2+ > C1- C1 out=1
;	C2+ < C2- C2 out=0
  .command "C1N.voltage = 1.7"
	nop
  .command "C2N.voltage = 2.7"
	nop
  .assert "(cmcon & 0xc0) == 0x40, '*** FAILED 16f628 ref2_com_out C1OUT=1 C2OUT=0'"
	nop
  .assert "(porta & 0x18) == 0x08, '*** FAILED 16f628 ref2_com_out CIS=0 port C1OUT=1 C2OUT=0'"
	nop
;	C2+ > C1- C1 out=1
;	C2+ > C2- C2 out=1
  .command "C2N.voltage = 2.3"
	nop
  .assert "(cmcon & 0xc0) == 0xc0, '*** FAILED 16f628 ref2_com_out C1OUT=1 C2OUT=1'"
	nop
  .assert "(porta & 0x18) == 0x18, '*** FAILED 16f628 ref2_com_out port C1OUT=1 C2OUT=1'"
	nop
	return
  end
