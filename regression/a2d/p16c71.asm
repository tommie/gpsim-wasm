
	list	p=16c71
        include <p16c71.inc>
        include <coff.inc>

  __CONFIG _WDT_OFF

	;; The purpose of this program is to test gpsim's ability to simulate a pic 16c71.
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
	 btfsc	ADCON0,ADIF
	goto	a2dint

   .assert "'p16c71 FAIL unexpected interrupt'"
	nop

;;	An A/D interrupt has occurred
a2dint:
	bsf	t1,0		;Set a flag to indicate we got the int.
	bcf	ADCON0,ADIF	;Clear the a/d interrupt

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
   .sim "module load pullup V2"
   .sim "V2.voltage=2.0"
   .sim "V2.resistance = 100.0"

   .sim "node na0"
   .sim "attach na0 V1.pin porta0"
   .sim "node na1"
   .sim "attach na1 V2.pin porta3"

	;; Let's use the ADC's interrupt
	
	clrf	INTCON
	
#define FAST_CONVERSION		((1<<ADCS1) | (1<<ADCS0))
	movlw	FAST_CONVERSION | (1<<ADON)
	movwf	ADCON0

#define ENABLE_INTS		((1<<GIE) | (1<<ADIE))
	movlw	ENABLE_INTS
	movwf	INTCON

	;; Upon power up, porta is configured for analog inputs.
	;; (how many times have I been burnt by this???).	
	;; So let's test this out to see if it's true:

	call	Convert

   .assert "adres == 0xff, 'FAILED 16C71 power-on result'"
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
	movwf	PORTA		;Make the I/O's digital outputs
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

   .assert "adres == 0x00, 'FAILED 16C71 Digital low'"
	nop
	;;
	;; Now do some with the digital output high
	;;

	movlw	0xff
	movwf	PORTA
	
	call	Convert

   .assert "adres == 0xff, 'FAILED 16C71 Digital high'"
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

   .assert "adres == 0xff, 'FAILED 16C71 AN0=5V'"
	nop
  .command "V1.voltage=1.0"

	call	Convert
   .assert "adres == 0x33, 'FAILED 16C71 AN0=1V'"
	nop

	;;
	;; Now let's use the external analog signal connected to AN3
	;; as the voltage reference

	bsf	STATUS,RP0
	movlw	1<<PCFG0
	movwf	ADCON1 ^ 0x80
	bcf	STATUS,RP0

	call	Convert
   .assert "adres == 0x80, 'FAILED 16C71 AN0=1V Vref+=2V'"
	nop

  .assert  "'*** PASSED 16C71 a2d test'"
	
	goto	$-1



Convert:

	clrf	t1		;flag set by the interrupt routine
	
	bsf	ADCON0,GO	;Start the A/D conversion

	btfss	t1,0		;Wait for the interrupt to set the flag
	 goto	$-1

	movf	ADRES,W		;Read the result

	return

	end
