
	list	p=16f917
        include <p16f917.inc>
        include <coff.inc>

        __CONFIG   _CP_OFF & _WDT_OFF &  _INTRC_OSC_NOCLKOUT ;& _MCLRE_OFF

	;; The purpose of this program is to test gpsim's ability to 
	;; simulate a pic 16F917.
	;; Specifically, the a/d converter is tested.

        errorlevel -302 

; Printf Command
.command macro x
  .direct "C", x
  endm

;----------------------------------------------------------------------
;----------------------------------------------------------------------
INT_VAR		IDATA	0x20
zero		DW	0x1f
one		DW	0x06
two		DW	0x5b
three		DW	0x4f

GPR_DATA                UDATA_SHR 0x70

x  		RES  1
t1 		RES  1
t2 		RES  1
avg_lo 		RES  1
avg_hi 		RES  1
w_temp 		RES  1
status_temp 	RES  1
eerom_cnt       RES  1
adr_cnt         RES  1
data_cnt	RES  1


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
        btfsc   PIR1,EEIF
          goto ee_int


	btfsc	INTCON,ADIE
	 btfsc	PIR1,ADIF
	  goto	check
   .assert "\"FAILED 16F917 unexpected interrupt\""
	nop

;;	An A/D interrupt has occurred
check:
	bsf	t1,0		;Set a flag to indicate we got the int.
	bcf	PIR1,ADIF	;Clear the a/d interrupt

exit_int
	swapf	status_temp,w
	movwf	STATUS
	swapf	w_temp,F
	swapf	w_temp,W
	retfie

; Interrupt from eerom
ee_int
        incf    eerom_cnt,F
        bcf     PIR1,EEIF
        goto    exit_int



;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start:

   .sim "p16f917.xpos = 120"
   .sim "p16f917.ypos = 36"

   .sim "module library libgpsim_modules"
   .sim "module library libgpsim_extras"


   .sim "module load lcd_7seg lcd1"
   .sim "lcd1.xpos = 300"
   .sim "lcd1.ypos = 108"

   .sim "module load lcd_7seg lcd2"
   .sim "lcd2.xpos = 300"
   .sim "lcd2.ypos = 276"


   .sim "module load pullup V1"
   .sim "V1.voltage = 1.0"
   .sim "V1.resistance = 1000.0"
   .sim "V1.xpos = 84"
   .sim "V1.ypos = 420"

   .sim "module load pullup V2"
   .sim "V2.voltage = 2.0"
   .sim "V2.resistance = 1000.0"
   .sim "V2.xpos = 84"
   .sim "V2.ypos = 372"

   .sim "module load pullup V3"
   .sim "V3.voltage = 3.0"
   .sim "V3.resistance = 1000.0"
   .sim "V3.xpos = 84"
   .sim "V3.ypos = 324"

   .sim "node vlcd3 vlcd2 vlcd1"
   .sim "attach vlcd3 V3.pin  portc2" 
   .sim "attach vlcd2 V2.pin  portc1" 
   .sim "attach vlcd1 V1.pin  portc0" 

   .sim "node Com0 Com1 nSeg0 nSeg1 nSeg2 nSeg3 nSeg4 nSeg5 nSeg6"

   .sim "attach Com0 lcd1.cc portb4 "
   .sim "attach Com1 lcd2.cc portb5 "
   .sim "attach nSeg0 lcd1.seg0 lcd2.seg0 portb0"
   .sim "attach nSeg1 lcd1.seg1 lcd2.seg1 portb1"
   .sim "attach nSeg2 lcd1.seg2 lcd2.seg2 portb2"
   .sim "attach nSeg3 lcd1.seg3 lcd2.seg3 portb3"
   .sim "attach nSeg4 lcd1.seg4 lcd2.seg4 porta4"
   .sim "attach nSeg5 lcd1.seg5 lcd2.seg5 porta5"
   .sim "attach nSeg6 lcd1.seg6 lcd2.seg6 portc3"
   .sim "scope.ch0 = \"portb4\""
   .sim "scope.ch1 = \"portb0\""
   

	BANKSEL WPUB
	clrf	WPUB
	BANKSEL	OSCCON
	bsf	OSCCON,IRCF0	; 8Mhz RC clock
	BANKSEL LCDSE0
	bcf	LCDCON,VLCDEN
	bsf	LCDCON,VLCDEN
	movlw	0x7f
	movwf	LCDSE0	
	bsf	LCDPS,LP1	; LPx = 2
;	movlw	(1<<LCDEN) | (1<<VLCDEN)	; static
;	movlw	(1<<LCDEN) | (1<<VLCDEN) | (1<<LMUX0) ; mux=1/2
	movlw	(1<<LCDEN) | (1<<VLCDEN) | (1<<LMUX1); mux=1/3
;	movlw	(1<<LCDEN) | (1<<VLCDEN) | (1<<LMUX0) | (1<<LMUX1); mux=1/4
	movwf	LCDCON
	btfss	LCDPS,WA
	goto	$-1
	movlw	0xa
        call    decode_segments
	movwf	LCDDATA0
	movlw	0x5
	call    decode_segments
	movwf	LCDDATA3
	goto 	$

decode_segments
        addwf   PCL,f
        retlw   0x3f            ; 0
        retlw   0x06            ; 1
        retlw   0x5b            ; 2
        retlw   0x4F            ; 3
        retlw   0x66            ; 4
        retlw   0x6d            ; 5
        retlw   0x7c            ; 6
        retlw   0x07            ; 7
        retlw   0x7f            ; 8
        retlw   0x67            ; 9
        retlw   0x77            ; A
        retlw   0x7c            ; B
        retlw   0x58            ; C
        retlw   0x5e            ; D
        retlw   0x79            ; E
        retlw   0x71            ; F
        
        end


	end
