
   ;;  Node test 
   ;;
   ;;  The purpose of this program is to verify that nodes
   ;; can interconnect I/O pins.


	list    p=16f873A               ; list directive to define processor
	include <p16f873a.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros


;------------------------------------------------------------------------
; gpsim command
.command macro x
  .direct "C", x
  endm

CMODE0                         EQU     H'0000'
CMODE1                         EQU     H'0001'
CMODE2                         EQU     H'0002'
CMODE3                         EQU     H'0003'
CMODE4                         EQU     H'0004'
CMODE5                         EQU     H'0005'
CMODE6                         EQU     H'0006'
CMODE7                         EQU     H'0007'


;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA_SHR
temp            RES     1

w_temp          RES     1
status_temp     RES     1

  GLOBAL done1


;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlw  high  start               ; load upper byte of 'start' label
        movwf  PCLATH                    ; initialize PCLATH
        goto   start                     ; go to beginning of program


;------------------------------------------------------------------------
;
;  Interrupt Vector
;
;------------------------------------------------------------------------
                                                                                
INT_VECTOR   CODE    0x004               ; interrupt vector location
                                                                                
        movwf   w_temp
        swapf   STATUS,W
        movwf   status_temp

       btfsc   PIR2,CMIF 
         btfss  PIE2,CMIE
	goto exit_int

	goto done1

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
;	test mode 6 - Four Inputs muxed to two comparators with Vref
;
  .assert "cmcon == 0x07, '*** FAILED 16f873A CMCON POR'"	; Rest value
	nop
	bcf	PIR2,CMIF
	BSF	STATUS,RP0
	movlw	0x06
	movwf	ADCON1

	movlw	0x0f		; RA3-RA0 are inputs
	movwf	TRISA
	movlw	0xAB		; enable Vref 11 low range
	movwf	CVRCON
        movlw	CMODE6		; comparater mux reference
        movwf	CMCON
  .assert "(cmcon & 0xc0) == 0x80, '*** FAILED 16f873A m6 C1OUT=0 C2OUT=1'"
	nop
	movlw	0x8B		; enable Vref 11 high range
	movwf	CVRCON
  .assert "(cmcon & 0xc0) == 0xc0, '*** FAILED 16f873A m6 C1OUT=1 C2OUT=1'" 
	nop
	bcf	OPTION_REG,NOT_RBPU	; set pull ups on B port
	bcf	OPTION_REG,T0CS		; Timer not from RA4
	CLRF	TRISB^0x80	;Port B is an output

	BCF 	STATUS,RP0
  .assert "(cmcon & 0xc0) == 0xc0, '*** FAILED 16f873A m6 C1OUT=1 C2OUT=1'"
	nop
  .assert "(pir2 & 0x40) == 0x40, '*** FAILED 16f873A CMIF=1'"
	nop
	bcf	PIR2,CMIF
  .assert "(pir2 & 0x40) == 0x00, '*** FAILED 16f873A CMIF=0'"
	nop
	bsf	PORTB,0			; drive comp 1 input high
  .assert "(cmcon & 0xc0) == 0x80, '*** FAILED 16f873A m6 C1OUT=0 C2OUT=1'"
        nop
  .assert "(pir2 & 0x40) == 0x40, '*** FAILED 16f873A CMIF=1'"
	nop
	bcf	PIR2,CMIF
	bsf	STATUS,RP0
	bsf	CMCON,C1INV		; invert output
	bsf	CMCON,C2INV
	bcf	STATUS,RP0
  .assert "(cmcon & 0xc0) == 0x40, '*** FAILED 16f873A m6 invert C1OUT=1 C2OUT=0'"
	nop
  .assert "(pir2 & 0x40) == 0x40, '*** FAILED 16f873A CMIF=1'"
	nop

	bcf	PIR2,CMIF
	bsf	PORTB,1			; drive comp 2 input high
	bcf	PORTB,0			; drive comp 1 input low
  .assert "(cmcon & 0xc0) == 0x80, '*** FAILED 16f873A m6 new inputs C1OUT=0 C2OUT=1'"
	nop
  .assert "(pir2 & 0x40) == 0x40, '*** FAILED 16f873A CMIF=1'"
	nop
	bcf	PIR2,CMIF

        movlw	0x06		; comparater mux reference normal output
	bsf	STATUS,RP0
        movwf	CMCON
        movf    CMCON,W                 ; This updates compare value
	bcf	STATUS,RP0
  .assert "(cmcon & 0xc0) == 0x40, '*** FAILED 16f873A m6 invert normal C1OUT=1 C2OUT=0'"
	nop
  .assert "(pir2 & 0x40) == 0x40, '*** FAILED 16f873A CMIF=1'"
	nop
	bcf	PIR2,CMIF
	bsf	STATUS,RP0
	bsf	PIE2,CMIE	; enable Comparator interupts
	bcf	STATUS,RP0
	bsf	INTCON,PEIE	; enable Periperal inturrupts
	bsf	INTCON,GIE	; and globale interupts
	bsf	PORTB,0		; drive comp1 input high
	nop
        nop
	goto FAILED		; no interrupt
done1:
;
;	mode 4 - Two Common reference Comparators
;
	movlw	0x08
	movwf	PORTB			; Both Comparator true
        movlw	CMODE4
	bsf	STATUS,RP0
        movwf	CMCON
	bcf	STATUS,RP0
  .assert "(cmcon & 0xc0) == 0xc0, '*** FAILED 16f873A m4 C1OUT=1 C2OUT=1'"
	nop
	movlw	0x03
	movwf	PORTB
  .assert "(cmcon & 0xc0) == 0x00, '*** FAILED 16f873A m4 C1OUT=0 C2OUT=0'"
	nop
	movlw	0x07
	movwf	PORTB
  .assert "(cmcon & 0xc0) == 0x00, '*** FAILED 16f873A m4 bit 2 C1OUT=0 C2OUT=0'"
	nop
	movlw	0x0c
	movwf	PORTB
  .assert "(cmcon & 0xc0) == 0xc0, '*** FAILED 16f873A m4 bit 2 C1OUT=1 C2OUT=1'"
	nop

;
;	mode 2 - Two independant Comparators
;
	movlw	0x05
	movwf	PORTB
        movlw	CMODE2
	bsf	STATUS,RP0
        movwf	CMCON
	bcf	STATUS,RP0
  .assert "(cmcon & 0xc0) == 0x80, '*** FAILED 16f873A m2 C1OUT=0 C2OUT=1'"
	nop
	movlw	0x0a
	movwf	PORTB
  .assert "(cmcon & 0xc0) == 0x40, '*** FAILED 16f873A m2 C1OUT=1 C2OUT=0'"
	nop
	movlw	0x0c
	movwf	PORTB
  .assert "(cmcon & 0xc0) == 0xc0, '*** FAILED 16f873A m2 C1OUT=1 C2OUT=1'"
	nop

;
;	mode 1 - One independant Comparator with output
;
	bsf	STATUS,RP0
	movlw	0x0f		; RA3-RA0 are inputs
	movwf	TRISA
	movlw	0xf0		; RB4, RB5 are inputs
	movwf	TRISB
        movlw	CMODE1
        movwf	CMCON
	bcf	STATUS,RP0
	movlw	0x01
	movwf	PORTB
  .assert "(cmcon & 0xc0) == 0x00, '*** FAILED 16f873A m1 C1OUT=0 C2OUT=0'"
	nop
  .assert "(porta & 0x10) == 0x00,  '*** FAILED 16f873A m1 porta4=0'"
	nop
  .assert "(portb & 0x10) == 0x00,  '*** FAILED 16f873A m1 portb4=0'"
	nop
	movlw	0x08
	movwf	PORTB
  .assert "(cmcon & 0xc0) == 0x40, '*** FAILED 16f873A m1 C1OUT=1 C2OUT=0'"
	nop
  .assert "(porta & 0x10) == 0x10,  '*** FAILED 16f873A m1 porta4=1'"
	nop
  .assert "(portb & 0x10) == 0x10,  '*** FAILED 16f873A m1 portb4=1'"
	nop
	movlw	0x0c
	movwf	PORTB
  .assert "(cmcon & 0xc0) == 0x40, '*** FAILED m1 16f873A AN2=1 C1OUT=1 C2OUT=0'"
	nop
  .assert "(porta & 0x10) == 0x10, '*** FAILED m1 16f873A AN2=1  porta4=1'"
	nop
  .assert "(portb & 0x10) == 0x10, '*** FAILED m1 16f873A AN2=1  portb4=1'"
	nop
	bsf	STATUS,RP0
        bsf	CMCON,C2INV		; Invert Comp 2 should have no effect
	bcf	STATUS,RP0
  .assert "(cmcon & 0xc0) == 0x40, '*** FAILED m1 16f873A C2INV=1 C1OUT=1'"
	nop
  .assert "(porta & 0x10) == 0x10, '*** FAILED m1 16f873A C2INV=1 porta4=1'"
	nop
  .assert "(portb & 0x10) == 0x10, '*** FAILED M1 16f873A C2INV=1 portb4=1'"
	nop
	bsf	STATUS,RP0
	bcf	CMCON,C2INV		; Invert Comp 1 only
	bsf	CMCON,C1INV
	bcf	STATUS,RP0
  .assert "(cmcon & 0xc0) == 0x00, '*** FAILED m1 16f873A C1INV=1 C1OUT=1'"
	nop
  .assert "(porta & 0x10) == 0x00, '*** FAILED m1 16f873A C1INV=1 porta4=0'"
	nop
  .assert "(portb & 0x10) == 0x00, '*** FAILED m1 16f873A C1INV=1 portb4=0'"
	nop

;
;	mode 5 - Two Common reference Comparators with outputs
;
	bsf	STATUS,RP0
	movlw	0x0f		; RA3-RA0 are inputs
	movwf	TRISA
	movlw	0x30		; RB4, RB5 are inputs
	movwf	TRISB
	bcf	STATUS,RP0

	movlw	0x08
	movwf	PORTB
        movlw	CMODE5		; Two common ref Comparators with output
	bsf	STATUS,RP0
        movwf	CMCON
	bcf	STATUS,RP0
  .assert "(cmcon & 0xc0) == 0xc0, '*** FAILED 16f873A m5 C1OUT=1 C2OUT=1'"
	nop
  .assert "(porta & 0x30) == 0x30, '*** FAILED 16f873A m5 porta4=1 porta5=1'"
	nop
  .assert "(portb & 0x30) == 0x30, '*** FAILED 16f873A m5 portb4=1 portb5=1'"
	nop
	movlw	0x03
	movwf	PORTB
  .assert "(cmcon & 0xc0) == 0x00, '*** FAILED 16f873A m5 C1OUT=0 C2OUT=0'"
	nop
  .assert "(porta & 0x30) == 0x00, '*** FAILED 16f873A m5 porta4=0 porta5=0'"
	nop
  .assert "(portb & 0x30) == 0x00, '*** FAILED 16f873A m5 portb4=0 portb5=0'"
	nop
        movlw	CMODE5|0x10		; invert COMP1 output
	bsf	STATUS,RP0
        movwf	CMCON
	bcf	STATUS,RP0
  .assert "(cmcon & 0xc0) == 0x40, '*** FAILED 16f873A m5 C1INV=1 C1OUT=1 C2OUT=0'"
	nop
  .assert "(porta & 0x30) == 0x10, '*** FAILED 16f873A m5 C1INV=1 porta4=1 porta5=0'"
	nop
  .assert "(portb & 0x30) == 0x10, '*** FAILED 16f873A m5 C1INV=1 portb4=1 portb5=0'"
	nop
        movlw	CMODE5|0x30		; invert COMP1  & COMP2 output
	bsf	STATUS,RP0
        movwf	CMCON
	bcf	STATUS,RP0
  .assert "(cmcon & 0xc0) == 0xc0, '*** FAILED 16f873A m5 C1OUT=1 C2OUT=1'"
	nop
  .assert "(porta & 0x30) == 0x30, '*** FAILED 16f873A m5 C1INV=1 C2INV=1 porta4=1 porta5=1'"
	nop
  .assert "(portb & 0x30) == 0x30, '*** FAILED 16f873A m5 C1INV=1 C2INV=1 portb4=1 portb5=1'"
	nop

;
;	mode 3 - Two independant Comparators with outputs
;
	movlw	0x05
	movwf	PORTB
        movlw	CMODE3
	bsf	STATUS,RP0
        movwf	CMCON
	bcf	STATUS,RP0
  .assert "(cmcon & 0xc0) == 0x80, '*** FAILED 16f873A m3 C1OUT=0 C2OUT=1'"
	nop
  .assert "(porta & 0x30) == 0x20, '*** FAILED 16f873A m3 porta4=0 porta5=1'"
	nop
  .assert "(portb & 0x30) == 0x20, '*** FAILED 16f873A m3 portb4=0 portb5=1'"
	nop
	movlw	0x0a
	movwf	PORTB
  .assert "(cmcon & 0xc0) == 0x40, '*** FAILED 16f873A m3 C1OUT=1 C2OUT=0'"
	nop
  .assert "(porta & 0x30) == 0x10, '*** FAILED 16f873A m3 porta4=1 porta5=0'"
	nop
  .assert "(portb & 0x30) == 0x10, '*** FAILED 16f873A m3 portb4=1 portb5=0'"
	nop
	movlw	0x0c
	movwf	PORTB
  .assert "(cmcon & 0xc0) == 0xc0, '*** FAILED 16f873A m3 C1OUT=1 C2OUT=1'"
	nop
  .assert "(porta & 0x30) == 0x30, '*** FAILED 16f873A m3 porta4=1 porta5=1'"
	nop
  .assert "(portb & 0x30) == 0x30, '*** FAILED 16f873A m3 portb4=1 portb5=1'"
	nop

  .assert  "'*** PASSED Comparator on 16f873A'"
	goto	$

FAILED:
  .assert  "'*** FAILED Comparator no interupt 16f873A'"
	goto	$



  end
