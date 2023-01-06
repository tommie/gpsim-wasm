
   ;;
   ;; This regression test exercises the 16f1705 CLC with a bit more rigour than the
   ;; 16F1503 version  
   ;; 
   ;;  Tests performed:
   ;;
   ;;  CONFIGURABLE LOGIC CELL (CLC) with multiple inputs gated
   ;;  PERIPHERAL PIN SELECT (PPS) 
   ;;     


	list    p=16f1705               ; list directive to define processor
	include <p16f1705.inc>          ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

.command macro x
  .direct "C", x
  endm

        __CONFIG  _CONFIG1, _CP_OFF & _FOSC_INTOSC & _MCLRE_OFF

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA_SHR
failures        RES     1
w_temp		RES	1
status_temp	RES	1
nco_int		RES	1
clc_int1	RES	1
clc_int2	RES	1
loop_count	RES	1


  GLOBAL loop_count, nco_int, status_temp, w_temp, clc_int1, clc_int2


;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlw  high  start               ; load upper byte of 'start' label
        movwf  PCLATH                    ; initialize PCLATH
        goto   start                     ; go to beginning of program

INT_VECTOR   CODE    0x004               ; interrupt vector location
	clrf	BSR
	goto	interrupts

  .sim ".xpos = 72"
  .sim ".ypos = 84"

  .sim "module library libgpsim_modules"
  ; Use a pullup resistor as a voltage source
  .sim "module load pullup V1"
  .sim "V1.resistance = 10000.0"
  .sim "V1.capacitance = 20e-12"
  .sim "V1.voltage=4.0"
  .sim "V1.xpos = 72"
  .sim "V1.ypos = 24"


  .sim "node clk"
  .sim "attach clk porta2 portc0"
   ;    Node Test
  .sim "node en_in"
  .sim "attach en_in porta1 portc1"

   .sim "scope.ch0 = 'portc0'"
   .sim "scope.ch1 = 'porta5'"
   .sim "scope.ch2 = 'porta4'"
   .sim "scope.ch3 = 'porta2'"
   .sim "scope.ch4 = 'portc1'"
   .sim "symbol cycleCounter=0"

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start

	banksel OPTION_REG
	movlw	0x03            ; TMR0 clocked every 16 instruction cycles
	movwf	OPTION_REG      ; Set timer 0 and RB pull-up options
	movlw	0x06
	movwf	TRISA           ; Configure port A
	movlw	0x00
	movwf	TRISC           ; Configure port C
	movlw	0xF0            ; Fosc at 32MHz via PLL
	movwf	OSCCON

	CLRF	failures	;Assume success

	BANKSEL PIE2
;	bsf	PIE2,NCO1IE
;	bsf	INTCON,GIE
;	bsf	PIE3,CLC1IE
;	bsf	PIE3,CLC2IE
;	bsf	INTCON,PEIE
;	clrf	WDTCON		; WDT to 1 ms

	BANKSEL ANSELA
	clrf	ANSELA
	clrf	ANSELC

	call	clc_t0
;	call	clc_t1
;	call	clc_t2
;        call    clc_in0
;	call    pwm_test
;	call	nco_clc
  .assert  "'*** PASSED p16f1705 CLC, PPS test'"
	nop
	GOTO	$



; External source drives CLC1 & CLC2 as gated complimentary pair, using the CLC gate logic
clc_t0:
	BANKSEL PIE3		; No Interrupts
	bcf	PIE3,CLC1IE
	bcf	PIE3,CLC2IE
	; First set the PPS to connect the relevant pins
	banksel	RA4PPS
	movlw	0x04			; LC1OUT  -->   DCC_SIG_P
	movwf	RA4PPS
	movlw	0x05			; LC2OUT  -->   DCC_SIG_N
	movwf	RA5PPS
	banksel	CLCIN0PPS
	movlw	0x02			; RA2 (DCC_IN)  -->   CLCIN0
	movwf	CLCIN0PPS
	movlw	0x01			; RA1 (NFAULT)  -->   CLCIN1
	movwf	CLCIN1PPS
	movlw	0x00			; RA0 (BSTR_EN)  -->   CLCIN2
	movwf	CLCIN2PPS
	; Next configure the CLC input data selection
	banksel	CLC1CON
	movlw	0x00			; Feed CLCIN0
	movwf	CLC1SEL0		; ... to input 0 of both channels
	movwf	CLC2SEL0
	movlw	0x01			; Feed CLCIN1
	movwf	CLC1SEL1		; ... to input 1 of both channels
	movwf	CLC2SEL1
	movlw	0x02			; Feed CLCIN2
	movwf	CLC1SEL2		; ... to input 2 of both channels
	movwf	CLC2SEL2
	clrf	CLC1SEL3		; other inputs unused
	clrf	CLC2SEL3
	; Now configure the CLC data gating
	movlw	0x15			; Want all inputs high (enabled, no fault, +ve signal) so 
	movwf	CLC1GLS0		; any low means output off
	movlw	0x16			; For channel 2, want enabled, no fault, -ve signal
	movwf	CLC2GLS0
	clrf	CLC1GLS1		; All other gates unused
	clrf	CLC1GLS2
	clrf	CLC1GLS3
	clrf	CLC2GLS1
	clrf	CLC2GLS2
	clrf	CLC2GLS3
	; Then set the output polarities
	movlw	0x0F			; Invert all gates but not the output
	movwf	CLC1POL
	movwf	CLC2POL
	; Finally configure the CLC logic functions and enable
	movlw	0x82			; Use the 4-input AND because unused gates are all 1
	movwf	CLC1CON
	movwf	CLC2CON
	clrf	CLC3CON			; CLC3 unused

    .command "cycleCounter = cycles"
	banksel	PORTA
        bcf	PORTA,0
        bcf	PORTC,0
        bcf	PORTC,1
        nop
    .assert "((porta & 0x30) == 0x00), '**** FAILED p16f1705 CLC step1'"

        bsf	PORTC,0		; signal change but still disabled
        nop
    .assert "((porta & 0x30) == 0x00), '**** FAILED p16f1705 CLC step2'"

        bsf	PORTC,1		; external enable but not internal
        nop
    .assert "((porta & 0x30) == 0x00), '**** FAILED p16f1705 CLC step3'"

        bcf	PORTC,1		; internal enable but not external
        bsf	PORTA,0
        nop
    .assert "((porta & 0x30) == 0x00), '**** FAILED p16f1705 CLC step4'"

        bsf	PORTC,1		; fully enabled
        nop
    .assert "((porta & 0x30) == 0x10), '**** FAILED p16f1705 CLC step5'"

        bcf	PORTC,0
        nop
    .assert "((porta & 0x30) == 0x20), '**** FAILED p16f1705 CLC step6'"

	return
 
; T1 then drives CLC1 via or-xor cell
clc_t1:
	BANKSEL PIE3		; No Interrupts
	bcf	PIE3,CLC1IE
	bcf	PIE3,CLC2IE
        BANKSEL CLC1CON
	movlw	0x06		; DS1 = T1, DS2 not used
        movwf   CLC1SEL0	
        movlw   0x05		; DS3 , DS4 not used
	movwf   CLC1SEL1
	movlw 	(1<<LC1G1D1T)   ; G1 DS3
        movwf   CLC1GLS0
	clrf    CLC1GLS1	; G2 unconnected not used
	clrf    CLC1GLS2        ; G3 unconnected not used
	clrf    CLC1GLS3        ; G4 unconnected not used
	clrf    CLC1POL
	movlw	0xc9		; CLC1 on, output enable, +interrupt, or-xor
	movwf   CLC1CON
	clrf    CLC2CON		; make sure CLC2 is off
	BANKSEL T1CON
	movlw	0xff		; overflow after 256 instructions
	movwf   TMR1H
	clrf	TMR1L
	movlw	(1<<TMR1ON)
	movwf   T1CON		; start T1 using FOSC/4
    .command "cycleCounter = cycles"
	nop
	bcf	PIR3,CLC1IF
	btfss	PIR3,CLC1IF
	goto	$-1
    .assert "((cycles - cycleCounter) >= 256) && ((cycles - cycleCounter) <= 260), '**** FAILED p16f1503 T1 CLC'"
	nop

	bcf	T1CON,TMR1ON	; T1 off
	return

; T1 then drives CLC1 via or-xor cell
clc_t2:
	BANKSEL PIE3		; No Interrupts
	bcf	PIE3,CLC1IE
	bcf	PIE3,CLC2IE
        BANKSEL CLC1CON
	movlw	0x07		; DS1 = T2, DS2 not used
        movwf   CLC1SEL0	
        movlw   0x05		; DS3 , DS4 not used
	movwf   CLC1SEL1
	movlw 	(1<<LC1G1D1T)   ; G1 DS3
        movwf   CLC1GLS0
	clrf    CLC1GLS1	; G2 unconnected - inverted = 1
	clrf    CLC1GLS2        ; G3 unconnected - inverted = 1
	clrf    CLC1GLS3        ; G4 unconnected - inverted = 1
        movlw   0x0e		;
	movwf   CLC1POL
	movlw	0xca	; CLC1 on, output enable, +interrupt, 4 input AND
	movwf   CLC1CON
	clrf    CLC2CON		; make sure CLC2 is off
	BANKSEL T2CON
	movlw	0x10
	movwf   PR2
	clrf	TMR2
	movlw	(1<<TMR2ON)	; T2 on 1 prescale
	movwf   T2CON		; start T1 using FOSC/4
    .command "cycleCounter = cycles"
	nop
	bcf	PIR3,CLC1IF
	btfss	PIR3,CLC1IF
	goto	$-1
    .assert "((cycles - cycleCounter) >= 16) && ((cycles - cycleCounter) <= 20), '**** FAILED p16f1503 T2 CLC'"
	nop

	bcf	T2CON,TMR2ON	; T2 off
	return
 
;; FRC drives CLC2 via and-or cell 
;; LC2_OUT then drives CLC1 via or-xor cell
;; LC1_OUT drives NCO
;clc_frc_nco:
;	BANKSEL PIE3		; No CLC interrupt so sleep to NCO interrupt
;	bcf	PIE3,CLC1IE
;	bcf	PIE3,CLC2IE
;        BANKSEL CLC1CON
;        clrf    CLC1SEL0	; DS1, DS2 not used
;        movlw   0x01		; DS3 = LC2_OUT
;	movwf   CLC1SEL1
;	movlw 	(1<<LC1G1D3T)   ; G1 DS3
;        movwf   CLC1GLS0
;	clrf    CLC1GLS1	; G2 unconnected not used
;	clrf    CLC1GLS2        ; G3 unconnected not used
;	clrf    CLC1GLS3        ; G4 unconnected not used
;	clrf    CLC1POL
;	movlw	0xc9		; CLC1 on, output enable, +interrupt, or-xor
;	movwf   CLC1CON
;
;        clrf    CLC2SEL0	; DS1, DS2 not used
;        movlw   0x05		; DS3 = FRC
;	movwf   CLC2SEL1
;	movlw 	(1<<LC2G1D3T)   ; G1 DS3
;        movwf   CLC2GLS0
;	clrf    CLC2GLS1	; G2 TRUE (unconnected output inverted)
;	clrf    CLC2GLS2        ; G3 unconnected not used
;	clrf    CLC2GLS3        ; G4 unconnected not used
;	movlw   (1<<LC2G2POL)   ; invert G2 only
;	movwf   CLC2POL
;	movlw	0xc8		; CLC2 on, output enable, +interrupt, and-or
;	movwf   CLC2CON
;
;	BANKSEL NCO1INCL
;	movlw	0x80 
;	movwf	NCO1INCH
;	movlw	0x00
;	movwf	NCO1INCL
;	movlw	0x12		; clock=LC1_OUT, out pulse 1 clock periods
;	movwf	NCO1CLK
;	movlw	0x05
;	movwf	loop_count
;	; enable NCO with output Pulse freq mode
;        movlw	(1<<N1EN)|(1<<N1OE) |(1<<N1PFM)
;	movwf	NCO1CON
;    .command "cycleCounter = cycles"
;	nop
;	sleep
;	nop
;        ; NCO wakeup  after (32+1) 600 kHz pulse = 55 usec
;	; AT FOSC = 16 Mhz 220 instuction cycles
;    .assert "((cycles - cycleCounter) >= 220) && ((cycles - cycleCounter) <= 230), '**** FAILED p16f1503 NCO FOSC loop'"
;	nop
;	clrf   NCO1CON		; turn of NCO
;	return
; 
;;	Frequency divider driven by CLC1IN0
;;       CLC2 haves frequency of CLC1
;clc_in0:
;
;;	BANKSEL APFCON
;;	bsf	APFCON,CLC1SEL	; CLC1 on portc5
;
;	BANKSEL PORTA
;	bsf	PORTA,4
;        BANKSEL CLC1CON
;	clrf	CLC1CON	;RRR test
;	clrf	CLC2CON	;RRR test
;
;	clrf	CLC2POL
;        movlw   0x40		; DS2 = LC1_OUT 
;        movwf   CLC2SEL0
;        movlw   0x01		; DS3 = LC2_OUT
;        movwf   CLC2SEL1
;        movlw   0x08		; G1(clock) D2S
;        movwf   CLC2GLS0
;        movlw   0x10		; G2(data) !DS3 (neg feedback)
;	movwf   CLC2GLS1
;        clrf    CLC2GLS2        ; G3 not used
;        clrf    CLC2GLS3        ; G4 not used
;	movlw   0xd4		; Enable, output pin, +interrupt
;	movwf   CLC2CON
;
;	clrf	CLC1POL
;	movlw	0x80		; G1(clock) D4S
;        movwf   CLC1GLS0
;	movlw	0x10	        ; G2(data)  !D3S (neg feedback)
;        movwf   CLC1GLS1
;        clrf    CLC1GLS2        ; G3 not used
;        clrf    CLC1GLS3        ; G4 not used
;        movlw	0		; Not used inputs Fosc, CLC1IN0 
;	movwf   CLC1SEL0
;        movlw	0x40		; D4S=CLC1IN0, D3S=LC1OUT
;	movwf   CLC1SEL1
;	movlw	0xd4		; Cell is 1-input D flip-flop with S and R
;        movwf	CLC1CON
;
;	BANKSEL PIR3		; CLC interrupts on
;	clrf	PIR3
;	BANKSEL PIE3		; CLC interrupts on
;	bsf	PIE3,CLC1IE
;	bsf	PIE3,CLC2IE
;	BANKSEL PORTA
;	movlw   0x8
;	movwf   loop_count
;        clrf    clc_int1
;        clrf    clc_int2
;clcloop:
;	bcf	PORTA,4
;	bsf	PORTA,4
;	decfsz  loop_count,F
;	goto    clcloop
;   .assert "clc_int1 == 4, '***FAILED P16f1503 CLC IN0 D flip-flop'"
;        nop
;   .assert "clc_int2 == 2, '***FAILED P16f1503 CLC IN0 D flip-flop'"
;	nop
;
;        BANKSEL CLC1CON
;        bsf	CLC1POL,LC1G3POL
;        bcf	CLC1POL,LC1G3POL
;	BANKSEL PIE3		; CLC interrupts on
;	bcf	PIE3,CLC1IE
;	bcf	PIE3,CLC2IE
;	return
;
;; NCO_out clocks CLC1 D flip-flop
;; LC1_OUT drives CLC2 D flip-flop
;nco_clc:
;	BANKSEL CLC1SEL1
;        movlw	0x00		; D4S=NCO_out, D3S=LC1OUT
;	movwf   CLC1SEL1
;	BANKSEL APFCON
;	bsf	APFCON,NCO1SEL	; NCO1 on porta4
;	BANKSEL NCO1INCL
;	clrf	NCO1INCL
;	movlw	0x20		; HFINTOSC clock, high 1 clock period
;	movwf	NCO1CLK
;
;	movlw	0x05
;	movwf	loop_count
;	; enable NCO with output Pulse freq mode
;        movlw	(1<<N1EN)|(1<<N1OE) |(1<<N1PFM)
;	movwf	NCO1CON
;	; set inch for about 64 instruction cycles per interrupt
;	movlw	0x10;
;	movwf	NCO1INCH
;	movlw	0x00
;	movwf	NCO1INCL
;	sleep
;	nop
;    .command "cycleCounter = cycles"
;	nop
;
;	sleep
;	nop
;	decfsz	loop_count,F
;	goto    $-3
;    .assert "((cycles - cycleCounter) >= 320) && ((cycles - cycleCounter) <= 324), '**** FAILED p16f1503 NCO FOSC loop'"
;
;	nop
;
;        movlw   (1<<N1CKS0)|(1<<N1CKS1)  ; use NCO1CLK input pin
;	movwf	NCO1CLK	
;	bcf	NCO1CON,N1PFM	; output toggle mode
;	movlw	0x80;
;	movwf	NCO1INCH
;	movlw	0x00
;	movwf	NCO1INCL
;	clrf	NCO1ACCU
;	clrf	NCO1ACCH
;	clrf	NCO1ACCL	; zero acc
;	BANKSEL PIE2
;	bcf	PIE2,NCO1IE	; disable interrupts
;	BANKSEL PIR2
;	bcf	PIR2,NCO1IF
;	clrf	loop_count
;inc_loop:
;	BANKSEL LATA
;	bsf	LATA,0
;	bcf	LATA,0
;	incf	loop_count,F
;	BANKSEL PIR2
;	btfss	PIR2,NCO1IF
;	goto	inc_loop
;	nop
;   .assert "loop_count == 0x22, '*** FAILED p16f1503 NCO1CLK pin increment'"
;	nop
;	bcf	PIR2,NCO1IE
;	
;	MOVLW	0xff
;	movwf   PIR2
;  .assert "pir2 == 0x6c, '*** FAILED P16f1503 PIR2 bits'"
;	nop
;	clrf	PIR2
;
;	return
;
;
;wait_tmr2:
;	BANKSEL PIR1
;	bcf	PIR1, TMR2IF
;        btfss   PIR1,TMR2IF
;	goto	$-1
;	return
;
;;  Must be called immediatley after in0_clc
;;	Frequency divider CLC1 driven by PWM4 as /2
;;       CLC2 haves frequency of CLC1
;pwm_test:
;	BANKSEL PIR3		; CLC interrupts on
;	clrf	PIR3
;	BANKSEL PIE3		; CLC interrupts on
;	bsf	PIE3,CLC1IE
;	bsf	PIE3,CLC2IE
;	clrf	clc_int1
;	clrf	clc_int2
;	BANKSEL CLC1SEL1
;        movlw	0x30		; D4S=pwm4, D3S=LC1OUT
;	movwf   CLC1SEL1
;	BANKSEL PR2
;	movlw	0x3f
;	movwf	PR2
;	BANKSEL PWM4CON
;	clrf	PWM4CON
;	movlw	0x0f
;	movwf	PWM4DCH      ; Duty cycle 25%
;	movlw	0xc0
;	movwf	PWM4DCL
;	movlw  	0x06          ; Start Timer2 with prescaler as 16
;	BANKSEL T2CON
;	movwf  	T2CON
;	clrf	TMR0
;	call	wait_tmr2
;	BANKSEL TRISA
;	bcf	TRISC,1
;        movlw   0x83          ; Tmr0 internal clock prescaler 16
;        movwf   OPTION_REG
;	BANKSEL PWM4CON
;	movlw	(1<<PWM4EN)|(1<<PWM4OE)
;	MOVWF	PWM4CON
;	call	wait_tmr2
;	BANKSEL TMR0
;	clrf    TMR0
;	BANKSEL PWM4CON
;	btfsc	PWM4CON,PWM4OUT
;	goto	$-1
;	.assert "tmr0 == 0x0f, '***FAIL p16f1503 CCP4 duty cycle'"
;	nop
;	call	wait_tmr2
;	.assert "tmr0 == 0x40, '***FAIL p16f1503 TMR2 == PR2'"
;	nop
;	call	wait_tmr2
;	call	wait_tmr2
;	call	wait_tmr2
;	call	wait_tmr2
;	call	wait_tmr2
;	call	wait_tmr2
;	.assert "clc_int1 == 4, '***FAILED p16f1503 pwm_test'"
;	nop
;	.assert "clc_int2 == 2, '***FAILED p16f1503 pwm_test'"
;	nop
;	BANKSEL PWM1DCH
;	bcf	PWM4CON,PWM4OE	; turn off PWM4 output pin
;	BANKSEL PIE3		; CLC interrupts on
;	bcf	PIE3,CLC1IE
;	bcf	PIE3,CLC2IE
;	return
;



FAILED:
  .assert  "'*** FAILED 16f1503 test'"
	INCF	failures,F

interrupts:
;	nop
;	btfsc	PIR2,NCO1IF
;	goto	nco_interrupt
;
;	btfsc	PIR3,CLC1IF
;	goto	clc1_int
;	btfsc	PIR3,CLC2IF
;	goto	clc2_int
;
;  .assert "'*** FAILED 16f1503 unknown interrupt'"
;	nop 
;
;clc1_int:
;	bcf	PIR3,CLC1IF
;	incf	clc_int1,F
;	goto    int_ret
;
;clc2_int:
;	bcf	PIR3,CLC2IF
;	incf	clc_int2,F
;	goto    int_ret
;
;
;nco_interrupt:
;	bcf     PIR2,NCO1IF
;	incf	nco_int,F

int_ret
	retfie
  end
