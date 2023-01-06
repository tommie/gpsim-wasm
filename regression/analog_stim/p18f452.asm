	list    p=18f452                ; list directive to define processor
	include <p18f452.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

.command macro x
  .direct "C", x
  endm

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA
LoopCounter     RES     1
lastADRES       RES     1

a2dIntFlag	RES	1  ;LSB is set when an A2D interrupt occurs

  GLOBAL done
  GLOBAL a2dIntFlag, LoopCounter

;------------------------------------------------------------------------
STARTUP    CODE	0

	bra	Start

;------------------------------------------------------------------------
;
;  Interrupt Vector
;
;------------------------------------------------------------------------

INT_VECTOR   CODE    0x008               ; interrupt vector location


check_TMR0_interrupt:

	btfsc	PIR1,ADIF	;If A2D int flag is not set
	 btfsc	PIE1,ADIE	;Or the interrupt is not enabled
	goto a2dint

  .assert "'FAIL 18F452 unexpected interrupt'"
	nop
	RETFIE 1		; Then leave

;;	An A/D interrupt has occurred
a2dint:
	bsf	a2dIntFlag,0	;Set a flag to indicate we got the int.
	bcf	PIR1,ADIF	;Clear the a/d interrupt

ExitInterrupt:
	RETFIE	1


;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE


   .sim "module library libgpsim_modules"
   ; Use a pullup resistor as a voltage source
   .sim "module load pullup V1"
   .sim "V1.resistance = 100.0"

   .sim "node na0"
   ;.sim "attach na0 V1.pin porta0"

   .sim "stimulus asynchronous_stimulus"

   ;# Specify the initial state of the stimulus. Note that this
   ;# is the value the stimulus is BEFORE start_cycle.

   .sim "initial_state 0.0"

   ;# all times are with respect to the cpu's cycle counter

   .sim "start_cycle 1"

   ;# the asynchronous stimulus will roll over in 'period'
   ;# cycles. Delete this line if you don't want a roll over.

   .sim "period 7000"

   ;# gpsim assumes digital data by default
   .sim "analog"

   ;# Now specify the data points. Each point needs two values:
   ;# the time at which it changes and the value to which it
   ;# changes.
   ;#       t        v
   ;#     ----  ----------

   .sim "{ 1000,     0.0,"
   .sim "  2000,     1.0,"
   .sim "  3000,     2.0,"
   .sim "  4000,     3.0,"
   .sim "  5000,     4.0,"
   .sim "  6000,     5.0 }"

   .sim "name asy_analog"
   .sim "end"


   .sim "attach na0 asy_analog porta0"

Start:


    ; RA0 is an Analog Input.
    ; RA1 - RA5 are all configured as outputs.
    ;
    ; Use VDD and VSS for Voltage references.
    ;
    ; PCFG = 1110  == AN0 is the only analog input
    ; ADCS = 110   == FOSC/64
    ; ADFM = 0     == 6 LSB of ADRESL are 0.
    ;

	MOVLW	1<<RA0
	MOVWF	TRISA

	MOVLW	(1<<ADCS2) | (1<<PCFG1) | (1<<PCFG2) | (1<<PCFG3)
	MOVWF	ADCON1
	MOVLW	(1<<ADCS1) | (1<<ADON)
	MOVWF	ADCON0

	BSF	INTCON,GIE	;Global interrupts
	BSF	INTCON,PEIE	;Peripheral interrupts
	BSF	PIE1,ADIE	;A2D interrupts

        clrf    lastADRES
        movlw   5
        movwf   LoopCounter
Loop1:  
	RCALL	Convert
        ANDLW   0xff
        BZ      Loop1
  .assert  "W==0x33, 'FAILED Analog Stimulus Test'"
        decfsz  LoopCounter,F
         bra    Loop1

done:
  .assert  "'*** PASSED Analog Stimulus Test'"
        bra     $


Convert:
	BCF	a2dIntFlag,0	;Clear interrupt handshake flag

	BSF	ADCON0,GO

LWait:	
	BTFSS	a2dIntFlag,0	;Wait for the interrupt to set the flag
	 bra	LWait

	MOVF	lastADRES,W		;Read the high 8-bits of the result
        SUBWF   ADRESH,W
        ADDWF   lastADRES,F

	RETURN

        end
