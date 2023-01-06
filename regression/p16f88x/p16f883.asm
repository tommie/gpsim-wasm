
	list	p=16f883
        include <p16f883.inc>
        include <coff.inc>

        __CONFIG  _CONFIG1, _CP_OFF & _WDT_OFF &  _INTRC_OSC_NOCLKOUT &  _LVP_OFF & _BOR_OFF & _MCLRE_OFF

	;; The purpose of this program is to test gpsim's ability to 
	;; simulate a pic 16F883.
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
	  goto	check
   .assert "'FAILED 16F883 unexpected interrupt'"
	nop

;;	An A/D interrupt has occurred
check:
	bsf	t1,0		;Set a flag to indicate we got the int.
	bcf	PIR1,ADIF	;Clear the a/d interrupt

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

   .sim "p16f883.xpos = 72"
   .sim "p16f883.ypos = 72"

   .sim "break c 0x100000"
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


   ; V3 and node na0 required for A/D to see pin voltage
   ; this may be a bug RRR 5/06

   .sim "module load pullup V3"	
   .sim "V3.resistance = 10e6"
   .sim "V3.xpos = 240"
   .sim "V3.ypos = 120"

   .sim "node na0"
   .sim "attach na0 V3.pin porta0"
   .sim "node na1"
   .sim "attach na1 V1.pin porta1"

   .sim "node na3"
   .sim "attach na3 V2.pin porta3"


	;; Let's use the ADC's interrupt
    ; RA1 is an Analog Input.
    ; RA0, RA2 - RA6 are all configured as outputs.
    ;
    ; Use VDD and VSS for Voltage references.
    ;
    ; PCFG = 1110  == AN0 is the only analog input
    ; ADCS = 110   == FOSC/64
    ; ADFM = 0     == 6 LSB of ADRESL are 0.
    ;
                                                                                
        movlw   3
	BANKSEL ANSEL
	clrf	ANSELH
	movwf	ANSEL		; select AN0, AN1
	BANKSEL TRISA
        movwf   TRISA
	
        bsf     PIE1,ADIE       ;A2D interrupts
	bcf	STATUS,RP0	;adcon0 is in bank 0
        movlw   (1<<ADCS1) | (1<<ADON) | (1<<CHS0); Fosc/32, A2D on, Channel 1
        movwf   ADCON0
                                                                                
        bsf     INTCON,GIE      ;Global interrupts
        bsf     INTCON,PEIE     ;Peripheral interrupts

	
	call	Convert
   .assert "adresh == 0xff, 'FAILED 16F883 inital test'"
	nop

	;; The next test consists of misusing the A/D converter.
	;; TRISA is configured such that the I/O pins are digital outputs.
	;; Normally you want them to be configued as inputs. According to
	;; the data sheet, the A/D converter will measure the voltage produced
	;; by the digital I/O output:	 either 0 volts or 5 volts (or Vdd).
	;; [I wonder if this would be a useful way of measuring the power supply
	;; level in the event that there's an external reference connected to
	;; an3?]
	

	movlw   0
	bsf	STATUS,RP0
	movwf	TRISA		;Make the I/O's digital outputs
	movlw	0xff
	movwf	ADCON1		;Configure porta to be completely analog
  .assert "adcon1 == 0xb0, 'FAILED 16F883 invalid ADCON1 bits writable'"
	nop
	clrf	ADCON1		;Configure porta to be completely analog
	bcf	STATUS,RP0
	bcf	ADCON0,CHS0	;select AN0
	clrf	PORTA		;Drive the digital I/O's low

	;;
	;; First do a few conversion with porta configured as a digital output
	;; that is driving low
	;;
	
	call	Convert

  .assert "adresh == 0x00, 'FAILED 16F883 Digital low'"
	nop

	;;
	;; Now do some with the digital output high
	;;

	movlw	0xff
	movwf	PORTA
	
	call	Convert

  .assert "adresh == 0xff, 'FAILED 16F883 Digital high'"
	nop
	;;
	;; Now make the inputs analog (like they normally would be)
	;;


	bsf	ADCON0,CHS0	;select AN1
	bsf	STATUS,RP0
	movlw	0xff
	movwf	TRISA
	bcf	STATUS,RP0

	call	Convert

  .assert "adresh == 0xff, 'FAILED 16F883 AN1=5V'"
	nop

  .command "V1.voltage=1.0"

	call	Convert

   .assert "adresh == 0x33, 'FAILED 16F883 AN1=1V'"
	nop

	;;
	;; Now let's use the external analog signal connected to AN3
	;; as the + voltage reference

	bsf	STATUS,RP0
	bsf	ADCON1,VCFG0
	BANKSEL ANSEL
	bsf	ANSEL,3
	BANKSEL ADCON0

  .command "V2.voltage=2.0"

	call	Convert

   .assert "adresh == 0x80, 'FAILED 16F883 AN1=1V Vref+=2V'"
	nop

	bsf	STATUS,RP0
	bsf	ADCON1,ADFM
	bcf	STATUS,RP0
	call	Convert

   .assert "adresh == 0x02, 'FAILED 16F883 ADFM=1 AN1=1V Vref+=2V'"
	nop

  .assert  "'*** PASSED 16F883 a2d test'"
	
	goto	$-1



Convert:

	clrf	t1		;flag set by the interrupt routine
	
	bsf	ADCON0,GO	;Start the A/D conversion

	btfss	t1,0		;Wait for the interrupt to set the flag
	 goto	$-1

        movf    ADRESH,W                ;Read the high 8-bits of the result

	return

	end
