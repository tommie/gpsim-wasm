;
;*******************Include file**********************************
	list    p=12f1822
	include <p12f1822.inc>
        include <coff.inc>              ; Grab some useful macros

.command macro x
  .direct "C", x
  endm

;--------------------------------------------------------
; config word(s)
;--------------------------------------------------------
	__config _CONFIG1, 0xffa4
	__config _CONFIG2, 0xdfff

;--------------------------------------------------------
; items in internal ram 
;--------------------------------------------------------
GPR_DATA	UDATA_SHR

result		RES	2
delay_count	RES	1
delay_temp	RES	1

;--------------------------------------------------------
; reset vector 
;--------------------------------------------------------
STARTUP	code	0x0000
	goto	start

	;; We use 2 ananlogue file inputs. Pin A1 gets an analogue value,
	;; and pin A3 is use to signal when pin A1 is ready to be probed.
   .sim "module library libgpsim_modules"
   .sim "module load FileStimulus S1"
   ;.sim "set v"
   .sim "S1.file = 'stim_an.sig'"

   .sim "node na1"
   .sim "attach na1 S1.pin porta1"

   .sim "module load FileStimulus S2"
   .sim "S2.file = 'stim_clk.sig'"

   .sim "node na3"
   .sim "attach na3 S2.pin porta3"
   .sim "break c 0x10000"

;--------------------------------------------------------
; code
;--------------------------------------------------------
MAIN	code 0x0020
start:
	;;  Initialisation
	banksel	ADCON1
	bsf	ADCON1, ADFM	; Output is right justified
	bsf	ADCON0, 2	; Input is on pin PORTA1
	bsf	ADCON0, ADON	; Enable ADC

	clrf	result
	clrf	result+1

	banksel	PORTA
main_loop:
	btfss	PORTA, 3	; Wait until AN0 is set
	 goto	main_loop

	bcf	PIR1, ADIF
	banksel	ADCON0
	bsf	ADCON0, GO	; Take a measurement
	btfsc	ADCON0, GO
	 goto	$-1

	movf	ADRESH, W	; A 0 value terminates the set
	iorwf	ADRESL, W
	bz	finish

	movf	ADRESL, W	; Add the value sampled to the end result
	addwf	result, F
	btfsc	STATUS, C
	 incf	result+1, F
	movf	ADRESH, W
	addwf	result+1, F

	banksel	PORTA
	btfsc	PORTA, 3	; Wait until the cycle is over
	 goto	$-1
	goto	main_loop

finish:	
	movf	result,   W
	iorwf	result+1, W

  .assert  "W==0xff, '*** FAILED File Stimulus Test'"
	nop
  .assert  "W!=0xff, '*** PASSED File Stimulus Test'"
	nop
	bra	$

	end
