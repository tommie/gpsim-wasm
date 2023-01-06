
   ;;  Node test 
   ;;
   ;;  The purpose of this program is to verify that nodes
   ;; can interconnect I/O pins.


	list    p=12f675                ; list directive to define processor
	include <p12f675.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

        __CONFIG _CP_OFF & _WDT_ON &  _INTRC_OSC_NOCLKOUT & _PWRTE_ON &  _BODEN_OFF & _MCLRE_OFF


;------------------------------------------------------------------------
; gpsim command
.command macro x
  .direct "C", x
  endm


;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA_SHR
temp            RES     1

w_temp          RES     1
status_temp     RES     1
cmif_cnt	RES	1
tmr0_cnt	RES	1
tmr1_cnt	RES	1
eerom_cnt	RES	1
adr_cnt		RES	1
data_cnt	RES	1
adc_cnt		RES	1




;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlw  high  start               ; load upper byte of 'start' label
        movwf  PCLATH                    ; initialize PCLATH
        goto   start                     ; go to beginning of program

  .sim "module library libgpsim_modules"
   ; Use a pullup resistor as a voltage source
  .sim "module load pullup R1"
  .sim "R1.resistance = 10000.0"
  .sim "R1.voltage = 2.0"
  .sim "R1.xpos = 252"
  .sim "R1.ypos = 84"

   ; Use a pullup resistor as a voltage reference
  .sim "module load pullup R2"
  .sim "R2.resistance = 10000.0"
  .sim "R2.voltage = 4.0"
  .sim "R2.xpos = 252"
  .sim "R2.ypos = 120"

  .sim "node n1"
  .sim "attach n1 gpio2 gpio3"
  .sim "node n2"
  .sim "attach n2 gpio0 gpio5 R1.pin"
  .sim "node n3"
  .sim "attach n3 gpio1 gpio4 R2.pin"


;------------------------------------------------------------------------
;
;  Interrupt Vector
;
;------------------------------------------------------------------------
                                                                                
INT_VECTOR   CODE    0x004               ; interrupt vector location
                                                                                
        movwf   w_temp
        swapf   STATUS,W
	bcf	STATUS,RP0	; set bank 0
        movwf   status_temp

        btfsc   PIR1,CMIF	
	   goto	cmif_int

	btfsc	PIR1,TMR1IF
	   goto tmr1_int

	btfsc	PIR1,EEIF
	    goto ee_int

  	btfsc	PIR1,ADIF
	    goto adc_int

  	btfsc	INTCON,T0IF
	    goto tmr0_int

	.assert "'***FAILED p12f675 unexpected interrupt'"
	nop

; Interrupt from Comparator
cmif_int
	incf	cmif_cnt,F
	bcf	PIR1,CMIF
	goto	exit_int

; Interrupt from TMR0
tmr0_int
	incf	tmr0_cnt,F
	bcf	INTCON,T0IF
	goto	exit_int

; Interrupt from TMR1
tmr1_int
	incf	tmr1_cnt,F
	bcf	PIR1,TMR1IF
	goto	exit_int

; Interrupt from eerom
ee_int
	incf	eerom_cnt,F
	bcf	PIR1,EEIF
	goto	exit_int

; Interrupt from adc
adc_int
	incf	adc_cnt,F
	bcf	PIR1,ADIF
	goto	exit_int
	
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
	;
	; test pins in analog mode return 0 on register read
	BANKSEL TRISIO
	clrf	TRISIO
	BANKSEL GPIO
	movlw	0xff
	movwf	GPIO
   .assert "gpio == 0x28, '**FAILED 12f675  analog bits read 0'"
	nop
	movf	GPIO,W

	movlw	0xff
	movwf	CMCON		; turn off Comparator
    .assert "cmcon == 0x1f, '**FAILED 12f675 CMCON read 0 bits'"
	nop
	BANKSEL ANSEL
	clrf	ANSEL		; turn off ADC port selects
	BANKSEL GPIO
;
; test GPIO works as expected
;
	clrf	GPIO
	bsf	STATUS,RP0
	movlw	0x38
	movwf	TRISIO		;GPIO 0,1,2 output 3,4,5 input
	bcf	STATUS,RP0
  .assert "gpio == 0x00, 'GPIO = 0x00'"
	nop
	movlw	0x07
	movwf	GPIO		; drive 0,1,2  bits high
  .assert "gpio == 0x3f, 'GPIO = 0x3f'"
	nop
	bsf     STATUS,RP0
	movlw	0x07
	movwf	TRISIO  	; GPIO 4, 5 output 0,1,2,3 input (3 input only)
        bcf     STATUS,RP0
  .assert "gpio == 0x0c, 'GPIO = 0x0c'"
	nop
	movlw	0x38
	movwf	GPIO		; drive output bits high
  .assert "gpio == 0x3f, 'GPIO = 0x3f'"
	nop

	call test_compare
	call test_tmr1
	call test_eerom
	call test_adc

  .assert  "'*** PASSED 12f675 Functionality'"
	goto	$

test_compare:
  ;; Comparator Tests
  ;;
  ;;	Mode 1 (Comparator with Output)
  ;;		CIN- > CIN+ COUT  = 0 and gpio2 low unless CINV=1
  ;;		    CIS has no effect on output
  ;;		CIN- < CIN+ gives output
  ;;	Mode 3 (Comparator with output and Vref)
  ;;		CIN- high COUT = 0
  ;;		CІN- low  COUT = 1
  ;;		    CIS has no effect on COUT
  ;;	Mode 5 (Mux input with output and Vref)
  ;;		CIN- low, CIS = 0 COUT = 1
  ;;		CIN+ high (CIN- low) with CIS = 0 has no effect on COUT
  ;;		CIN+ high, CIN- low, CIS 1 COUT = 0
  ;;		CIN+ low (CIN- high) CIS 1 COUT = 1
  ;;		CIN+ high (CIN- high) CIS 1 COUT = 0
  ;;		enable and catch interrupt when CINV set
;
;	Test mode 1 Comparator with output
;
	bsf     STATUS,RP0
	movlw	0x03
	movwf	TRISIO  	; GPIO 2, 4, 5 output 0,1,3 input (3 input only)
        bcf     STATUS,RP0
	movlw  0x20
	movwf  GPIO	; AN0 1 AN1 0
	movlw  0x01
	movwf  CMCON
   .assert "cmcon == 1, '*** FAILED 12f675 cmcon Comp mode 1 Cinv 0 v+ > v-'"
	nop
   .assert "(gpio & 0x04) == 0, '*** FAILED 12f675 gpio2 Comp mode 1 Cinv 0 v+ > v-'"
	nop
	bsf	CMCON,CINV		; Cinv = 1
   .assert "cmcon == 0x51, '*** FAILED 12f675 cmcon Comp mode 1 Cinv 1 v+ > v-'"
	nop
	bsf	CMCON,CIS		; CIS = 1
   .assert "cmcon == 0x59, '*** FAILED 12f675 cmcon Comp mode 1 Cinv 1 CIS 1 v+ > v-'"
	nop
   .assert "(gpio & 0x04) == 0x04, '*** FAILED 12f675 gpio2 Comp mode 1 Cinv 1 v+ > v-'"
	nop
	movlw  0x01
	movwf  CMCON		; clear CINV and CIS
	movlw  0x10
	movwf  GPIO	; AN0 0 AN1 1
   .assert "cmcon == 0x41, '*** FAILED 12f675 Comp mode1 v+ < v-'"
	nop
   .assert "(gpio & 0x04) == 0x04, '*** FAILED 12f675 gpio2 Comp mode 1 Cinv 1 v+ < v-'"
	nop
	bsf	CMCON,CINV
   .assert "cmcon == 0x11, '*** FAILED 12f675 cmcon Comp mode 1 Cinv 1 v+ < v-'"
	nop
   .assert "(gpio & 0x04) == 0, '*** FAILED 12f675 gpio2 Comp mode 1 Cinv 1 v+ < v-'"
	nop
	nop
;
;	Mode 3 internal reference with output
;
	BANKSEL VRCON
	bsf	VRCON,VREN		; Voltage reference high range
	BANKSEL CMCON
	movlw	0x03
	movwf	CMCON
   .assert "cmcon == 0x03, '*** FAILED 12f675 cmcon Comp mode 3 Cinv 0 AN1 5V'"
	nop
	bcf	GPIO,4		; drive AN1 to low
   .assert "cmcon == 0x43, '*** FAILED 12f675 cmcon Comp mode 3 Cinv 0 AN1 0V'"
	nop
	bsf	CMCON,CIS		; CIS should not change output
   .assert "cmcon == 0x4b, '*** FAILED 12f675 cmcon Comp mode 3 Cinv 0 Cis 1 AN1 0V'"
	nop
	
;
;	test mode 5 - MUX input with Vref and Output
;
	movlw	0x05
	movwf	CMCON		; mode 5 and AN1
   .assert "cmcon == 0x45, '*** FAILED 12f675 cmcon Comp mode 5 CIS 0 AN0 0V AN1 0V'"
	nop
	bsf	GPIO,5	; Drive AN0 high (AN1 low)
   .assert "cmcon == 0x45, '*** FAILED 12f675 cmcon Comp mode 5 CIS 0 AN0 5V AN1 0V'"
	nop
	bsf	CMCON,CIS	; select AN0 (GPIO0)
   .assert "cmcon == 0x0d, '*** FAILED 12f675 cmcon Comp mode 5 CIS 1 AN0 5V AN1 0V'"
	nop
	movlw	0x10	; Drive AN0 low AN1 High
	movwf	GPIO
   .assert "cmcon == 0x4d, '*** FAILED 12f675 cmcon Comp mode 5 CIS 1 AN0 0V AN1 5V'"
	nop
	bsf	GPIO,5	; Drive AN0 high (AN1 high)
   .assert "cmcon == 0x0d, '*** FAILED 12f675 cmcon Comp mode 5 CIS 1 AN0 5V AN1 5V'"
	nop
;
;	test comparator interupts
;
	bcf	PIR1,CMIF
	BANKSEL PIE1
	movlw	(1 << GIE) | ( 1 << PEIE )
	movwf	INTCON
	bsf	PIE1,CMIE
	BANKSEL CMCON
	bsf	CMCON,CINV	; this should change output and cause interrupt
cm_loop:
	movf 	cmif_cnt,W
	btfsc	STATUS,Z
	goto	cm_loop

	clrf	INTCON
	movlw	0x07	; turn off comparator
	movwf	CMCON

	return

test_tmr1:
        ;; Here are the tests performed:
        ;;
        ;; -- TMR1L and TMR1H can be read and written
        ;; -- TMR1 driven Fosc/4 with prescale of 8 and generates an interrupt

        ; Load TMR1H, TMR1L with 0x8000 
        CLRF    TMR1L
        MOVLW   0x80
        MOVWF   TMR1H

        BCF     PIR1,TMR1IF     ;Clear any TMR1 pending interrupt
	BANKSEL PIE1
        BSF     PIE1,TMR1IE     ;Enable TMR1 interrupts
	BANKSEL TMR1L
        BSF     INTCON,PEIE     ;Enable Peripheral interrupts
        BSF     INTCON,GIE      ;Enable Global interrupts

  ; TMR1 not running yet, TMR1H and TMR1L should be unchanged
        MOVF    TMR1L,W         ; test read
   .assert "W==0, '*** FAILED 12f675 TMR1 test TMR1L read'"
        nop
        MOVF    TMR1H,W
   .assert "W==0x80, '*** FAILED 12f675 TMR1 test TMR1H read'"
        nop
;  at 4Mhz prescale of 8 Fosc/4 should interrupt in 0.1 seconds
	movlw	0xcf
	movwf	TMR1H
	movlw	0x2c
	movwf	TMR1L
	MOVLW   (1<<T1CKPS1) | (1<<T1CKPS0) | (1<<TMR1ON)
        MOVWF   T1CON
	CLRWDT
tmr1_loop:
	movf 	tmr1_cnt,W
	btfsc	STATUS,Z
	goto	tmr1_loop
;
;	Test gate enable
;
	BANKSEL TRISIO
	movlw	0x38
	movwf	TRISIO		;GPIO 0,1,2 output 3,4,5 input
	bcf	OPTION_REG,T0CS ; run TMR0 off Fosc/4
	BANKSEL GPIO
	movlw	(1<<GIE) | (1<<T0IE) | (1<<PEIE)
	movwf	INTCON		; enable T0 interupt
	bsf	GPIO,1		; Pin 4 high
	bsf	T1CON,TMR1GE	; Enable Gate Control
	call	test_tmr1_off
	bcf	GPIO,1		; Pin 4 low tmr1, tmr1 running
	call	test_tmr1_on
	bsf	GPIO,1		; Pin 4 high, stop TMR1
	call	test_tmr1_off
	bcf	T1CON,TMR1GE	; Gate Control off, start TMR1
	call	test_tmr1_on

;
;	WDT rather than tmr1 whould wake this sleep
;
        sleep
   .assert "(status & 0x18) == 0, '*** FAILED 12f675 TMR1 test - WDT wake'"
        nop
        BANKSEL PIE1
        BcF     PIE1,TMR1IE     ;Disable TMR1 interrupts
        BANKSEL TMR1L

	return

;
; test that tmr1 is stopped
;
test_tmr1_off:
	movlw	0xc0
	movwf	TMR0
	movf	TMR1L,W		; Hold TMR1l 
	clrf	tmr0_cnt
tmr1_loop2:
	btfss	tmr0_cnt,0
	goto	tmr1_loop2

   .assert "tmr1l == W, '*** FAILED 12f675 TMR1 test - TMR1 stop'"
	nop
	return

;
; test that tmr1 is running
;
test_tmr1_on:
	movlw	0xc0
	movwf	TMR0
	movf	TMR1L,W
	clrf	tmr0_cnt
tmr1_loop3:
	btfss	tmr0_cnt,0
	goto	tmr1_loop3

   .assert "tmr1l != W, '*** FAILED 12f675 TMR1 test - TMR1 running'"
	nop
	return

test_eerom:
  ;
  ;	test can write and read to all 128 eeprom locations
  ;	using intterupts
        clrf    adr_cnt
        clrf    data_cnt
;  setup interrupts
        bsf     INTCON,PEIE
        bsf     INTCON,GIE
	BANKSEL PIE1
	bsf	PIE1,EEIE
	BANKSEL	PIR1
;
;	write to EEPROM starting at EEPROM address 0
;	value of address as data using interrupts to
;	determine write complete. 
;	read and verify data

l1:     
        movf    adr_cnt,W
	clrf	eerom_cnt
	BANKSEL	EEADR
        movwf   EEADR 
        movf    data_cnt,W
        movwf   EEDATA

        bcf     INTCON,GIE      ;Disable interrupts while enabling write

        bsf     EECON1,WREN    ;Enable eeprom writes

        movlw   0x55            ;Magic sequence to enable eeprom write
        movwf   EECON2
        movlw   0xaa
        movwf   EECON2

        bsf     EECON1,WR      ;Begin eeprom write

        bsf     INTCON,GIE      ;Re-enable interrupts
        
	BANKSEL	PIR1
        clrf    STATUS          ; Bank 0
        movf   eerom_cnt,W
	skpnz
        goto   $-2
;
;	read what we just wrote
;
	
        movf    adr_cnt,W

	BANKSEL	EEADR
	movwf   EEADR
	bsf	EECON1,RD	; start read operation
	movf	EEDATA,W	; Read data
	BANKSEL	PIR1

	xorwf	data_cnt,W	; did we read what we wrote ?
	skpz
	goto eefail

        incf    adr_cnt,W
        andlw   0x7f
        movwf   adr_cnt
	movwf	data_cnt

        skpz
         goto   l1

	return

eefail:
  .assert "'***FAILED 12f675 eerom write/read error'"
	nop

test_tmr0:
	return
	
test_adc:

  ; ADCS = 110 Fosc/64
  ; ANS = 1 AN0 is only analog port
	movlw	(1 << ADCS2) | (1 << ADCS1) | (1 << ANS0)
	BANKSEL ANSEL
	movwf	ANSEL
	movlw	0x3f
	movwf	TRISIO		; All pins input
	BANKSEL ADCON0
	bsf	ADCON0,ADON	; enable ADC

        bsf     INTCON,GIE      ;Global interrupts
        bsf     INTCON,PEIE     ;Peripheral interrupts
        call    Convert
   .assert "W == 0x66, '*** FAILED 12f675 ADC test - AN0 = 2 Vref = Vddi ADFM=0'"
	nop

	BANKSEL ANSEL
	bsf	ANSEL,ANS1	; AN1 is analog
	BANKSEL ADCON0
	bsf	ADCON0,VCFG	; AN1 as hi Vref 
        call    Convert
   .assert "W == 0x80, '*** FAILED 12f675 ADC test - AN0 = 2 Vref = AN1 = 4 ADFM=0'"
	nop
	bsf	ADCON0,ADFM	; AN1 as hi Vref , ADFM=1
        call    Convert
   .assert "W == 0x02, '*** FAILED 12f675 ADC test - AN0 = 2 Vref = AN1 = 4 ADFM=0'"
	nop
	bsf	ADCON0,CHS1	; select shannel 2 which is not ADC analog pin
	call	Convert

	BANKSEL ANSEL
	clrf	ANSEL		; return ports to digital I/O
	BANKSEL ADCON0

	return

Convert:

        clrf    adc_cnt              ;flag set by the interrupt routine

        bsf     ADCON0,GO       ;Start the A/D conversion

        btfss   adc_cnt,0            ;Wait for the interrupt to set the flag
         goto   $-1

        movf    ADRESH,W                ;Read the high 8-bits of the result

        return

	
  end
