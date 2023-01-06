	list    p=18f4321               ; list directive to define processor
	include <p18f4321.inc>          ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

.command macro x
  .direct "C", x
  endm

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA
temp            RES     1
temp1           RES     1
temp2           RES     1
failures        RES     1

a2dIntFlag	RES	1  ;LSB is set when an A2D interrupt occurs

  GLOBAL done
  GLOBAL a2dIntFlag

  CONFIG MCLRE=OFF

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

  .assert "'FAIL 18F4321 unexpected interrupt'"
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

    .sim "p18f4321.xpos = 192"
    .sim "p18f4321.ypos = 48"


   .sim "module library libgpsim_modules"
   ; Use a pullup resistor as a voltage source
   .sim "module load pullup V1"
   .sim "V1.resistance = 100.0"
   .sim "V1.xpos = 48"
   .sim "V1.ypos = 24"

   .sim "module load pullup V2"
   .sim "V2.resistance = 100.0"
   .sim "V2.xpos = 48"
   .sim "V2.ypos = 204"

   ; V3 and na1 required for A/D to see voltage bug ?
   ; RRR 5/06
   .sim "module load pullup V3"
   .sim "V3.resistance = 10e6"
   .sim "V3.xpos = 48"
   .sim "V3.ypos = 120"

   .sim "node na0"
   .sim "attach na0 V1.pin porta0"
   .sim "node na1"
   .sim "attach na1  V3.pin porta1"
   .sim "node na3"
   .sim "attach na3 V2.pin porta3"


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
	clrf	TRISA
	MOVLW	0xff
	movwf	PORTA
	movf	PORTA,W

	MOVLW	1<<RA0
	MOVWF	TRISA

	MOVLW	(1<<PCFG1) | (1<<PCFG2) | (1<<PCFG3)
	MOVWF	ADCON1
        MOVLW	(1<<ADCS1) | (1<<ADCS2)
	MOVWF	ADCON2
	MOVLW	(1<<ADON)
	MOVWF	ADCON0

	BSF	INTCON,GIE	;Global interrupts
	BSF	INTCON,PEIE	;Peripheral interrupts
	BSF	PIE1,ADIE	;A2D interrupts

	RCALL	Convert

  .assert "adresh == 0xff, 'FALIED 18F4321 a2d AN0=5V'"
	nop

        ;; The next test consists of misusing the A/D converter.
        ;; TRISA is configured such that the I/O pins are digital outputs.
        ;; Normally you want them to be configued as inputs. According to
        ;; the data sheet, the A/D converter will measure the voltage produced
        ;; by the digital I/O output:    either 0 volts or 5 volts (or Vdd).
        ;; [I wonder if this would be a useful way of measuring the power supply        ;; level in the event that there's an external reference connected to
        ;; an3?]
                                                                                
                                                                                
;  .command "V1.resistance=1e6"
                                                                                
        movlw   0
        movwf   TRISA           ;Make the I/O's digital outputs
        movwf   ADCON1          ;Configure porta to be completely analog
	bsf	ADCON0,CHS0	;Use AN1
                                                                                
        movwf   PORTA           ;Drive the digital I/O's low

	RCALL	Convert

  .assert "adresh == 0x00, 'FAILED 18F4321 Digital low'"
	nop

	movlw	0x02
	movwf	PORTA		; drive bit 1 high

	RCALL	Convert

  .assert "adresh == 0xff, 'FAILED 18F4321 Digital high'"
	nop

        movlw   0xff
        movwf   TRISA           ;Make the I/O's inputs
	bcf	ADCON0,CHS0	;Use AN0

  .command "V1.voltage=1.0"
                                                                                
        rcall    Convert
                                                                                
   .assert "adresh == 0x33, 'FAILED 18F4321 AN0=1V'"
        nop

  .command "V2.voltage=2.0"
	bsf	ADCON1,VCFG0	; RA3 is Vref
        rcall    Convert
                                                                                
   .assert "adresh == 0x80, 'FAILED 18F4321 AN0=1V Vref+=2V'"
        nop

done:
  .assert  "'*** PASSED 18F4321 a2d test'"
        bra     $


Convert:
	BCF	a2dIntFlag,0	;Clear interrupt handshake flag

	BSF	ADCON0,GO

LWait:	
	BTFSS	a2dIntFlag,0	;Wait for the interrupt to set the flag
	 bra	LWait

	MOVF	ADRESH,W		;Read the high 8-bits of the result

	RETURN

        end
