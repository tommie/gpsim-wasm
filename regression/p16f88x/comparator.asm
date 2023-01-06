
	list	p=16f882
        include <p16f882.inc>
        include <coff.inc>

        __CONFIG  _CONFIG1, _CP_OFF & _WDT_OFF &  _INTRC_OSC_NOCLKOUT &  _LVP_OFF & _BOR_OFF & _MCLRE_OFF

	;; The purpose of this program is to test gpsim's ability to 
	;; simulate a pic 16F882.
	;; Specifically, the comparator is tested.

        errorlevel -302 

; Printf Command
.command macro x
  .direct "C", x
  endm

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA_SHR

int_cm1 RES 1
int_cm2 RES 1
x  RES  1
t1 RES  1
t2 RES  1
avg_lo RES  1
avg_hi RES  1
w_temp RES  1
status_temp RES  1


;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlw  high  start               ; load upper byte of 'start' label
        movwf  PCLATH                    ; initialize PCLATH
        goto   start                     ; go to beginning of program

INT_VECTOR   CODE    0x004               ; interrupt vector location
	;; 
	;; Interrupt
	;; 
	movwf	w_temp
	swapf	STATUS,W
	movwf	status_temp

	goto	interrupt

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start:

   .sim "p16f882.xpos = 72"
   .sim "p16f882.ypos = 72"

   .sim "module library libgpsim_modules"
   ; Use a pullup resistor as a voltage source
   .sim "module load pullup V1"
   .sim "V1.resistance = 100.0"
   .sim "V1.xpos = 240"
   .sim "V1.ypos = 72"

   .sim "module load pullup V2"
   .sim "V2.resistance = 100.0"
   .sim "V2.xpos = 84"
   .sim "V2.ypos = 24"
   .sim "V2.voltage = 2.5"


   ; V3 and node na0 required for A/D to see pin voltage
   ; this may be a bug RRR 5/06

   .sim "module load pullup V3"	
   .sim "V3.resistance = 100.0"
   .sim "V3.xpos = 240"
   .sim "V3.ypos = 120"
   .sim "V3.voltage = 4.5"

   .sim "node na0"
   .sim "attach na0 V3.pin porta0"
   .sim "node na1"
   .sim "attach na1 V1.pin porta1"

   .sim "node na3"
   .sim "attach na3 V2.pin porta3"


                                                                                
        movlw   9
	BANKSEL ANSEL
	clrf	ANSELH
	movwf	ANSEL		; select AN0, AN3
	BANKSEL TRISA
        movwf   TRISA
        movwf   TRISC

        bsf     INTCON,GIE      ;Global interrupts
        bsf     INTCON,PEIE     ;Peripheral interrupts
	BANKSEL PIE2
        bsf     PIE2,C1IE       ;CM1 interrupts
	call    test_compar
	call	test_tot1	; can C2 increment T1
  .assert  "'*** PASSED 16F882 comparator test'"
	goto	$-1

;
; Test Comparator2 gate control of Timer 1
;
test_tot1:
	BANKSEL T1CON		
	movlw	(1<<TMR1GE)|(1<<TMR1ON)		; T1 on with gate
	movwf	T1CON
	clrf	TMR1H
	clrf	TMR1L
	BANKSEL CM1CON0
	clrf	CM1CON0		; turn off C1
	bcf	CM2CON1,T1GSS	; T1 gate source is SYNCC2OUT
	movlw 	(1<<C2ON)|(1<<C2R)	; Enable Comparator use C2VRef, C12IN0
	movwf	CM2CON0
	bsf	CM2CON0,C1POL	; toggle ouput polarity,
	BANKSEL	TMR1L
	movf	TMR1L,W		; is Timer 1 running ?
	nop
	nop
	SUBWF	TMR1L,W
  .assert "W == 0, '*** FAILED T1 running gate off'"
	nop
	bsf	T1CON,T1GIV	; invert gate control
	movf	TMR1L,W		; is Timer 1 running ?
	nop
	nop
	SUBWF	TMR1L,W
  .assert "W != 0, '*** FAILED T1 gate invert'"
	nop
	; Test TMR1 IF set (bug 257)
	
	BANKSEL PIR1
	bcf	PIR1,TMR1IF
	BANKSEL TMR1H
	decf	TMR1H,F
	btfsc	TMR1H,7
	goto	$-1	

    .assert "(pir1 & 1) == 1, '*** FAILED P16F882 TMR1IF set'"
	nop
	
	BANKSEL CM2CON0
	bcf	CM2CON0,C1POL	; toggle comparator ouput polarity,
	BANKSEL	TMR1L
	movf	TMR1L,W		; is Timer 1 running ?
	nop
	nop
	SUBWF	TMR1L,W
  .assert "W == 0, '*** FAILED T1 running gate on'"
	nop
	return

test_compar:
	BANKSEL CM1CON0
	movlw	0xff
	movwf   CM2CON1		; test writable bits
  .assert "cm2con1 == 0x33, '*** FAILED 16f882 cm2con1 writable bits'"
	nop
        clrf	CM2CON1
	bsf	CM1CON0,C1POL	; toggle ouput polarity, comparator not ON
  .assert "cm1con0 == 0x50, '*** FAILED 16f88x cm1con0 off toggle'"
	nop
  .assert "cm2con1 == 0x80, '*** FAILED 16f88x cm2con1 mirror C1OUT'"
	nop

	movlw 	(1<<C1ON)	; Enable Comparator use C1IN+, C12IN0
	movwf	CM1CON0
  .assert "cm1con0 == 0x80, '*** FAILED 16f88x cm1con0 ON=1'"
	nop
	bsf	CM1CON0,C1POL	; toggle ouput polarity
  .assert "cm1con0 == 0xd0, '*** FAILED 16f88x cm1con0 ON=1 POL=1'"
	nop
	bsf	CM2CON0,C1POL	; toggle ouput polarity, not ON
  .assert "cm2con0 == 0x50, '*** FAILED 16f88x cm2con0 ON=0 POL=1'"
	nop
  .assert "cm2con1 == 0xc0, '*** FAILED 16f88x cm2con1 mirror C2OUT'"
	nop
	bsf	CM1CON0,C1OE	; C1OUT t0 RA4
   .assert "(porta & 0x10) == 0x10, '*** FAILED 16f88x compare RA4 not high'"
	nop 
	bcf	CM1CON0,C1POL	; toggle ouput polarity
   .assert "(porta & 0x10) == 0x00, '*** FAILED 16f88x compare RA4 not low'"
	nop
	; Test change in voltage detected
   .command "V3.voltage = 2.0"
	nop
   .assert "(porta & 0x10) == 0x10, '*** FAILED 16f88x compare RA4 not high'"
	nop
	bsf	CM1CON0,C1R	; C1IN+ is voltage reference
	nop
	bsf	CM2CON1,C1RSEL  ; Vref from Comparator Reference
	nop
	movf	CM1CON0,W	; FIXME should not be needed
	nop
	return



interrupt:

	bcf	STATUS,RP0	;select bank 0
	bcf	STATUS,RP1	

	btfsc   PIR2,C1IF
	  goto  int_com1
	btfsc   PIR2,C2IF
	  goto  int_com2

	btfsc	INTCON,ADIE
	 btfsc	PIR1,ADIF
	  goto	check
   .assert "'*** FAILED 16F882 unexpected interrupt'"
	nop
back_interrupt:
	swapf	status_temp,w
	movwf	STATUS
	swapf	w_temp,F
	swapf	w_temp,W
	retfie

;;	An A/D interrupt has occurred
check:
	bsf	t1,0		;Set a flag to indicate we got the int.
	bcf	PIR1,ADIF	;Clear the a/d interrupt
	goto	back_interrupt

;;	Compatator 1 interrupt
int_com1:
	bsf	int_cm1,0
	bcf	PIR2,C1IF
	goto	back_interrupt


;;	Compatator 2 interrupt
int_com2:
	bsf	int_cm2,0
	bcf	PIR2,C2IF
	goto	back_interrupt





	end
