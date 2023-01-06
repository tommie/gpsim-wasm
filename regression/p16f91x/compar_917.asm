
   ;;  Comparator test 
   ;;
   ;;  The purpose of this program is to verify the
   ;;  comparator module of the processor


	list    p=16f917               ; list directive to define processor
	include <p16f917.inc>           ; processor specific variable definitions
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
cm2_int		RES	1

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

       btfsc   PIR2,C1IF 
	 goto icm1
       btfsc   PIR2,C2IF 
	 goto icm2
	goto exit_int

icm1:
	bcf 	PIR2,C1IF
	bsf	cmp_int,0
	goto	exit_int

icm2:
	bcf 	PIR2,C2IF
	bsf	cm2_int,0


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
  .sim "p16f917.xpos = 60"
  .sim "p16f917.ypos = 84"

  .sim "module load pullup AN3"
  .sim "AN3.capacitance = 0"
  .sim "AN3.resistance = 10000"
  .sim "AN3.voltage = 3."
  .sim "AN3.xpos = 72"
  .sim "AN3.ypos = 24"

  .sim "module load pullup AN0"
  .sim "AN0.capacitance = 0"
  .sim "AN0.resistance = 10000"
  .sim "AN0.voltage = 2.5"
  .sim "AN0.xpos = 228"
  .sim "AN0.ypos = 36"

  .sim "module load pullup AN2"
  .sim "AN2.capacitance = 0"
  .sim "AN2.resistance = 10000"
  .sim "AN2.voltage = 2.8"
  .sim "AN2.xpos = 252"
  .sim "AN2.ypos = 96"

  .sim "module load pullup AN1"
  .sim "AN1.capacitance = 0"
  .sim "AN1.resistance = 10000"
  .sim "AN1.voltage = 2"
  .sim "AN1.xpos = 228"
  .sim "AN1.ypos = 144"

  .sim "node c1p"
  .sim "attach c1p AN3.pin porta3"
  .sim "node c1n"
  .sim "attach c1n AN0.pin porta0"
  .sim "node c2p"
  .sim "attach c2p AN2.pin porta2"
  .sim "node c2n"
  .sim "attach c2n AN1.pin porta1"

  .assert "cmcon0 == 0x00, 'FAILED 16f917 CMCON0 POR'"	; Rest value
	nop
	BANKSEL PIR2
	clrf	PIR2
        movlw	CMODE7		; turn off comparator 
	BANKSEL CMCON0
        movwf	CMCON0
	clrf	ANSEL
	movlw	0x0f		; RA0-RA3 are inputs
	movwf	TRISA
;	mode 1 - Three Inputs Multiplexed to Two Comparators
	movlw	CMODE1
	movwf	CMCON0
	call	c1_out
	nop
;
;	mode 2 - Four Inputs Multiplexed to Two Comparators with Vref
;
        movlw	CMODE2
        movwf	CMCON0
	call	mux_vref
	nop
;
;	mode 3 Two Common Reference Comparator
;
        movlw   CMODE3
        movwf   CMCON0
        call    ref_com
        nop


;	mode 4 Two Independent Comparators
;
        movlw   CMODE4
        movwf   CMCON0
        call    ind_comp
        nop

;	mode 5 One Independent Comparator with Reference Option
;
        movlw   CMODE5
        movwf   CMCON0
        call    one_comp
        nop

;
;	mode 6 Two Common Reference Comparators with Outputs
;
        movlw	CMODE6		
        movwf	CMCON0
	call	ref_com_out
	nop

  .assert  "'*** PASSED p16f917 Comparator'"
	goto	$


;	One Independent Comparator with Reference Option
;
;	C2- AN1 (0.65)
;	C2+ AN2 (0.7) or V0.6
one_comp:
  .command "AN2.voltage = 0.7"
	nop
  .command "AN1.voltage = 0.65"
	nop
  .assert "(cmcon0 & 0xc0) == 0x80, '*** FAILED 16f917 one_comp, c2out=1'"
	nop
	bsf	CMCON0,CIS
  .assert "(cmcon0 & 0xc0) == 0x00, '*** FAILED 16f917 one_comp, c2out=0'"
	nop

	return
;	Three Inputs Multiplexed to Two Comparators
c1_out:
;	C1IN- AN0 (2.5)
;	C1IN+ AN2 (2.8) thus C1 out=1
;	C2in- AN1 (2.0) 
;	C2IN+ AN2 (2.8) thus C2 out=1
  .assert "(cmcon0 & 0xc0) == 0xc0, '*** FAILED 16f917 c1_out, cxout=1'"
	nop
  .assert "(porta & 0x30) == 0x00, '*** FAILED 16f917 c1_out, OUT1=0 OUT2=0'"
	nop
	bsf	CMCON0,C1INV
	bsf	CMCON0,C2INV
;	Invert c1,c1 out=0
  .assert "(cmcon0 & 0xc0) == 0x00, '*** FAILED 16f917 c1_out, invert cxout=01'"
	nop
  .assert "(porta & 0x30) == 0x00, '*** FAILED 16f917 c1_out, OUT1=0 OUT2=0'"
	nop

;	C1IN- AN3 (3.)
;	C1IN+ (2.8) with invert thus C1 out=1
	bsf	CMCON0,CIS	; switch c1 Vin-
  .assert "(cmcon0 & 0xc0) == 0x40, '*** FAILED 16f917 c1_out INVERT , c1out=0'"
	nop
  .assert "(porta & 0x30) == 0x00, '*** FAILED 16f917 c1_out, INVERT OUT1=0 OUT2=0'"
	nop

	return


;	Two independent Comparators
ind_comp:
;
;		Both Comparator true
;       C1IN-AN0(2.1V), C1 -
;	C1IN+AN3(3.1V)  C1 + thus C1 out=1
;	C2in-AN1(1.5V) connected to C2 -
;	C2IN+AN2(2.5V) C2 + thus C2 out=1
  .command "AN2.voltage = 2.5"
	nop
  .command "AN1.voltage = 1.5"
	nop
  .command "AN3.voltage = 3.1"
	nop
  .command "AN0.voltage = 2.1"
	nop

  .assert "(cmcon0 & 0xc0) == 0xc0, 'FAILED 16f917 2 ind comp CIS=0 C1OUT=1 C2OUT=1'"
	nop
	bsf	CMCON0,CIS
  .assert "(cmcon0 & 0xc0) == 0xc0, 'FAILED 16f917 2 ind comp CIS=1 C1OUT=1 C2OUT=1'"
	nop
;	C2+ < C2- C2 out=0
  .command "AN2.voltage = 1.4"
	nop
	nop
  .assert "(cmcon0 & 0xc0) == 0x40, 'FAILED 16f917 2 ind comp C1OUT=1 C2OUT=0'"
	nop
;	C1+ < C1- C1 out=0
;	C2+ > C2- C2 out=1
  .command "AN3.voltage = 2.0"
	nop
  .command "AN2.voltage = 2.5"
	nop
  .assert "(cmcon0 & 0xc0) == 0x80, 'FAILED 16f917 2 ind comp C1OUT=0 C2OUT=1'"
	nop
;		both low
;	C2+ < C2- C2 out=0
  .command "AN2.voltage = 1.4"
	nop
  .assert "(cmcon0 & 0xc0) == 0x00, 'FAILED 16f917 2 ind comp  C1OUT=0 C2OUT=0'"
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
  .command "AN2.voltage = 2.5"
	nop
  .command "AN1.voltage = 1.5"
	nop
  .command "AN3.voltage = 3.1"
	nop
  .command "AN0.voltage = 2.1"
	nop

  .assert "(cmcon0 & 0xc0) == 0xc0, '*** FAILED 16f917 ind_comp_out CIS=0 C1OUT=1 C2OUT=1'"
	nop
  .assert "(porta & 0x30) == 0x30, '*** FAILED 16f917 ind_comp_out CIS=0 port C1OUT=1 C2OUT=1'"
	nop
	bsf	CMCON0,CIS
  .assert "(cmcon0 & 0xc0) == 0xc0, '*** FAILED 16f917 ind_comp_out CIS=1 C1OUT=1 C2OUT=1'"
	nop
  .assert "(porta & 0x30) == 0x30, '*** FAILED 16f917 ind_comp_out CIS=1 port C1OUT=1 C2OUT=1'"
	nop
;	C2+ < C2- C2 out=0
  .command "AN2.voltage = 1.4"
	nop
	nop
  .assert "(cmcon0 & 0xc0) == 0x40, '*** FAILED 16f917 ind_comp_out C1OUT=1 C2OUT=0'"
	nop
  .assert "(porta & 0x30) == 0x10, '*** FAILED 16f917 ind_comp_out port C1OUT=1 C2OUT=0'"
	nop
;	C1+ < C1- C1 out=0
;	C2+ > C2- C2 out=1
  .command "AN3.voltage = 2.0"
	nop
  .command "AN2.voltage = 2.5"
	nop
  .assert "(cmcon0 & 0xc0) == 0x80, '*** FAILED 16f917 ind_comp_out C1OUT=0 C2OUT=1'"
	nop
  .assert "(porta & 0x30) == 0x20, '*** FAILED 16f917 ind_comp_out port C1OUT=0 C2OUT=1'"
	nop
;		both low
;	C2+ < C2- C2 out=0
  .command "AN2.voltage = 1.4"
	nop
  .assert "(cmcon0 & 0xc0) == 0x00, '*** FAILED 16f917 ind_comp_out  C1OUT=0 C2OUT=0'"
	nop
  .assert "(porta & 0x30) == 0x00, '*** FAILED 16f917 ind_comp_out port C1OUT=0 C2OUT=0'"
	nop
	return


;
;	Four Inputs muxed to two comparators with Vref
;
mux_vref:
  .command "AN3.voltage = 1.0"
	nop
  .command "AN0.voltage = 2.5"
	nop
  .command "AN2.voltage = 2.8"
	nop
  .command "AN1.voltage = 2.0"
	nop
	; C1IN- = AN0 2.5 + = 2.29	out = 0
	; C2IN- = AN1 2.0 + = 2.29  out = 1
        BANKSEL PIR2
        clrf    PIR2
	BANKSEL VRCON
	movlw	0xAB		; enable Vref 11 low range 2.29v
	movwf	VRCON
  .assert "(cmcon0 & 0xc0) == 0x80, 'FAILED 16f917 mux with vref C1OUT=0 C2OUT=1'"
	nop
  .assert "(pir2 & 0x60) == 0x40, 'FAILED 16f917 mux with vref C2IF=1'"
	nop
	BANKSEL PIR2
	clrf	PIR2
	movlw	0x8B		; enable Vref 11 high range 2.97v
        BANKSEL VRCON
	movwf	VRCON
	; C1IN- = 2.5 + = 2.97	out = 1 was 0
	; C2IN- = 2.0 + = 2.97  out = 1 was 1
  .assert "(cmcon0 & 0xc0) == 0xc0, 'FAILED 16f917 mux with vref C1OUT=1 C2OUT=1'" 
	nop

  .assert "(pir2 & 0x60) == 0x20, 'FAILED 16f917 mux with vref C1IF=1 C2IF =0'"
	nop
	BANKSEL PIR2
	bcf	PIR2,C1IF
  .assert "(pir2 & 0x60) == 0x00, 'FAILED 16f917 mux with vref C1IF=0, C2IF=0'"
	nop
  .command "AN0.voltage = 3.0"	; drive comp1 low (C1IN- > Vref)
	nop
	nop
  .assert "(cmcon0 & 0xc0) == 0x80, 'FAILED 16f917 mux with vref C1OUT=0 C2OUT=1'"
        nop
  .assert "(pir2 & 0x60) == 0x20, 'FAILED 16f917 mux with vref C1IF=1 C1 change'"
	nop
	bcf	PIR2,C1IF
	BANKSEL CMCON0
	bsf	CMCON0,C1INV		; invert output
	bsf	CMCON0,C2INV
  .assert "(cmcon0 & 0xc0) == 0x40, 'FAILED 16f917 mux with vref invert C1OUT=1 C2OUT=0'"
	nop
  .assert "(pir2 & 0x60) == 0x60, 'FAILED 16f917 mux with vref invert CxIF=1'"
	nop

	BANKSEL PIR2
	clrf	PIR2
  .command "AN1.voltage = 3.5"		; C2IN- > Vref
	nop
  .command "AN0.voltage = 2.5"		; C1IN- < Vref
	nop
  .command "AN3.voltage = 3.1"		; C1IN+ > Vref
	nop
  .assert "(cmcon0 & 0xc0) == 0x80, 'FAILED 16f917 mux with vref new inputs C1OUT=0 C2OUT=1'"
	nop
  .assert "(pir2 & 0x60) == 0x60, 'FAILED 16f917 mux with vref CxIF=1 C1, C2 change'"
	nop
	BANKSEL CMCON0
	bsf	CMCON0,CIS	; switch pins
;
;	C1 - AN3 (3.1) + Vref (2.97) invert out = 1
;	C2 - AN2 (2.8) + Vref (2.97) invert out = 0
;
	bcf	PIR2,C1IF
  .assert "(cmcon0 & 0xc0) == 0x40, 'FAILED 16f917 mux with vref new inputs C1OUT=1 C2OUT=0'"
	nop

 	movlw	0xc7		; clear invert, CIS bits
	andwf	CMCON0,F
  .assert "(cmcon0 & 0xc0) == 0x40, 'FAILED 16f917 mux with vref invert normal C1OUT=1 C2OUT=0'"
	nop
	BANKSEL PIR2
	clrf	PIR2
	clrf	cmp_int
	BANKSEL PIE2
	bsf	PIE2,C1IE	; enable Comparator interrupts
	bsf	PIE2,C2IE	; enable Comparator interrupts
	bsf	INTCON,PEIE	; enable Peripheral interrupts
	bsf	INTCON,GIE	; and global interrupts
  	BANKSEL CMCON0
	bsf	CMCON0,C1INV	; generate an interrupt
	btfsc	cmp_int,0
	goto	done1
        nop
  .assert  "'*** FAILED Comparator no interrupt 16f917 mux with vref '"
	nop
	goto	$
done1:
	return
;
;	Two common reference Comparators
;
;	C1IN+ AN2(2.5V) to both comparator +
;       C1IN-AN0(3.1V),connected to C1 - out=0
;	C2in-AN1(1.5V) connected to C2 - out=1
ref_com:
  .command "AN2.voltage = 2.5"
	nop
  .command "AN1.voltage = 1.5"
	nop
  .command "AN3.voltage = 0.5"
	nop
  .command "AN0.voltage = 3.1"
	nop

  .assert "(cmcon0 & 0xc0) == 0x80, '*** FAILED 16f917 ref_com CIS=0 C1OUT=0 C2OUT=1'"
	nop
	bsf	CMCON0,CIS
  .assert "(cmcon0 & 0xc0) == 0x80, '*** FAILED 16f917 ref_com CIS=1 C1OUT=0 C2OUT=1'"
	nop
;	C2+ > C1- C1 out=1
;	C2+ < C2- C2 out=0
  .command "AN0.voltage = 1.7"
	nop
  .command "AN1.voltage = 2.7"
	nop
  .assert "(cmcon0 & 0xc0) == 0x40, '*** FAILED 16f917 ref_com C1OUT=1 C2OUT=0'"
	nop
;	C2+ > C1- C1 out=1
;	C2+ > C2- C2 out=1
  .command "AN1.voltage = 2.3"
	nop
  .assert "(cmcon0 & 0xc0) == 0xc0, '*** FAILED 16f917 ref_com C1OUT=1 C2OUT=1'"
	nop
	return
;
;	Two common reference Comparators with outputs
;
;	C1IN+AN2(2.5V) to both comparator +
;       C1IN-AN0(3.1V),connected to C1 - out=0
;	C2in-AN1(1.5V) connected to C2 - out=1
ref_com_out:
  .command "AN3.voltage = 0.5"
	nop
  .command "AN1.voltage = 1.5"
	nop
  .command "AN2.voltage = 2.5"
	nop
  .command "AN0.voltage = 3.1"
	nop

  .assert "(cmcon0 & 0xc0) == 0x80, '*** FAILED 16f917 ref_com_out CIS=0 C1OUT=0 C2OUT=1'"
	nop
  .assert "(porta & 0x30) == 0x20, '*** FAILED 16f917 ref_com_out CIS=0 port C1OUT=0 C2OUT=1'"
	nop
	bsf	CMCON0,CIS
  .assert "(cmcon0 & 0xc0) == 0x80, '*** FAILED 16f917 ref_com_out CIS=1 C1OUT=0 C2OUT=1'"
	nop
  .assert "(porta & 0x30) == 0x20, '*** FAILED 16f917 ref_com_out CIS=1 port C1OUT=0 C2OUT=1'"
	nop
;	C2+ > C1- C1 out=1
;	C2+ < C2- C2 out=0
  .command "AN0.voltage = 1.7"
	nop
  .command "AN1.voltage = 2.7"
	nop
  .assert "(cmcon0 & 0xc0) == 0x40, '*** FAILED 16f917 ref_com_out C1OUT=1 C2OUT=0'"
	nop
  .assert "(porta & 0x30) == 0x10, '*** FAILED 16f917 ref_com_out CIS=0 port C1OUT=1 C2OUT=0'"
	nop
;	C2+ > C1- C1 out=1
;	C2+ > C2- C2 out=1
  .command "AN1.voltage = 2.3"
	nop
  .assert "(cmcon0 & 0xc0) == 0xc0, '*** FAILED 16f917 ref_com_out C1OUT=1 C2OUT=1'"
	nop
  .assert "(porta & 0x30) == 0x30, '*** FAILED 16f917 ref_com_out port C1OUT=1 C2OUT=1'"
	nop
	return
  end
