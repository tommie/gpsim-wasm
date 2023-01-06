
	list	p=16f684
        include <p16f684.inc>
        include <coff.inc>

  __CONFIG _WDT_OFF

	;; The purpose of this program is to test gpsim's ability 
	;; to simulate a pic 16f684.
	;; Specifically, the a/d converter is tested.

        errorlevel -302 

; Printf Command
.command macro x
  .direct "C", x
  endm

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA_SHR

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

	bcf	STATUS,RP0	;adcon0 is in bank 0

	btfsc	INTCON,ADIE
	 btfsc	PIR1,ADIF
	goto	inta2d

  .assert "'FAILED 16F684 a2d unexpected interupt'"
	nop
	goto	check
;;	An A/D interrupt has occurred
inta2d:
	bsf	t1,0		;Set a flag to indicate we got the int.
	bcf	PIR1,ADIF	;Clear the a/d interrupt


check:
	swapf	status_temp,w
	movwf	STATUS
	swapf	w_temp,F
	swapf	w_temp,W
	retfie



;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start:

   .sim "module library libgpsim_modules"
   ; Use a pullup resistor as a voltage source
   .sim "module load pullup V1"
   .sim "V1.resistance = 100.0"
   .sim "V1.xpos = 252"
   .sim "V1.ypos = 84"


   .sim "node na0"
   .sim "attach na0 V1.pin porta0"

   .sim "module load pullup V2"
   .sim "V2.resistance = 100.0"
   .sim "V2.xpos = 252"
   .sim "V2.ypos = 144"


   .sim "node na1"
   .sim "attach na1 V2.pin porta1"

        BANKSEL PIR1
        clrf    PIR1
        movlw   0x07          ; turn off comparater 
        movwf   CMCON0
        BANKSEL ANSEL
        movlw	0x03		; an0 only 
	movwf	ANSEL 


	;; Let's use the ADC's interrupt
    ; RA0 is an Analog Input.
    ; RA1 - RA5 are all configured as outputs.
    ;
	movwf	TRISA
        bsf     PIE1,ADIE       ;A2D interrupts

    ; Use VDD and VSS for Voltage references.
    ;
    ; PCFG = 1110  == AN0 is the only analog input
    ; ADCS = 110   == FOSC/64
    ; ADFM = 0     == 6 LSB of ADRESL are 0.

        movlw	0x60	; FOSC/64
	movwf	ADCON1
    ;
    ; Left justify results, use Vdd Reference, use AN0, and turn on A2D
    ;
	BANKSEL ADCON0
	movlw  (1<<ADON)
	movwf	ADCON0
        bsf     INTCON,GIE      ;Global interrupts
        bsf     INTCON,PEIE     ;Peripheral interrupts

	
	call	Convert

  .assert "adresh == 0xff, 'Failed 16F684 a2d initial test'"
	nop

	;; The next test consists of misusing the A/D converter.
	;; TRISA is configured such that the I/O pins are digital outputs.
	;; Normally you want them to be configued as inputs. According to
	;; the data sheet, the A/D converter will measure the voltage produced
	;; by the digital I/O output:	 either 0 volts or 5 volts (or Vdd).
	;; [I wonder if this would be a useful way of measuring the power supply
	;; level in the event that there's an external reference connected to
	;; an3?]
	
  .command "V1.resistance=1e6"

	movlw   0
	bsf	STATUS,RP0
	movwf	TRISA		;Make the I/O's digital outputs
	movwf	ADCON1		;Configure porta to be completely analog
				;(note that this is unnecessary since that's
				;the condition they're in at power up.)
	bcf	STATUS,RP0

	movwf	PORTA		;Drive the digital I/O's low

	;;
	;; First do a few conversion with porta configured as a digital output
	;; that is driving low
	;;
	
	call	Convert

  .assert "adresh == 0x00, 'Failed 16F684 a2d Digital low'"
	nop

	;;
	;; Now do some with the digital output high
	;;

	movlw	0xff
	movwf	PORTA
	
	call	Convert

  .assert "adresh == 0xff, 'Failed 16F684 a2d Digital high'"
	nop
	;;
	;; Now make the inputs analog (like they normally would be)
	;;

  .command "V1.resistance=100.0"

	bsf	STATUS,RP0
	movlw	0xff
	movwf	PORTA
	bcf	STATUS,RP0

	call	Convert

  .assert "adresh == 0xff, 'Failed 16F684 a2d AN0=5V'"
	nop


  .command "V1.voltage=1.0"

	call	Convert

  .assert "adresh == 0x33, 'Failed 16F684 a2d AN0=1V'"
	nop
	;;
	;; Now let's use the external analog signal connected to AN1
	;; as the voltage reference

	BANKSEL ADCON0
	bsf	ADCON0,VCFG

  .command "V2.voltage=2.0"

	call	Convert
  .assert "adresh == 0x80, 'Failed 16F684 a2d AN0=1V Vref+=2V'"
	nop

  .assert  "'*** PASSED 16F684 a2d test'"
	
	goto	$-1



Convert:

	clrf	t1		;flag set by the interrupt routine
	
	bsf	ADCON0,GO	;Start the A/D conversion

	btfss	t1,0		;Wait for the interrupt to set the flag
	 goto	$-1

        movf    ADRESH,W                ;Read the high 8-bits of the result


	return

	end
