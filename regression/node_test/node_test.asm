
   ;;  Node test 
   ;;
   ;;  The purpose of this program is to verify that nodes
   ;; can interconnect I/O pins.


	list    p=16f877                ; list directive to define processor
	include <p16f877.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros


;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA
temp            RES     1

  GLOBAL done


;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlw  high  start               ; load upper byte of 'start' label
        movwf  PCLATH                    ; initialize PCLATH
        goto   start                     ; go to beginning of program


INT_VECTOR   CODE    0x004               ; interrupt vector location
	nop
;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start

	MOVLW	0xff
	BSF	STATUS,RP0

	CLRF	TRISB^0x80	;Port B is an output
	MOVWF	TRISC^0x80	;Port C is an input

	BCF 	STATUS,RP0

b_to_c_loop:
	MOVWF	PORTB		;Port B and Port C are externally
	XORWF	PORTC,W		;connected. So we should see the
	SKPZ			;same thing on each port.
	 GOTO	FAILED

	DECFSZ	PORTC,W
	 goto	b_to_c_loop

done:

  .assert  "'*** PASSED Node Test on 16f877'"
	goto	$

FAILED:
  .assert  "'*** FAILED Node Test on 16f877'"
	goto	$



  end
