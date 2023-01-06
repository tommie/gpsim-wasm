
   ;;  16F84 tests
   ;;
   ;; This regression test exercises the 16f84.  
   ;; 
   ;;  Tests performed:
   ;;
   ;;  - Verify that the PORTB internal pull-ups work
   ;;     


	list    p=16f84                 ; list directive to define processor
	include <p16f84.inc>            ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

CONFIG_WORD	EQU	_CP_OFF & _WDT_OFF
        __CONFIG  CONFIG_WORD

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA
failures        RES     1


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


   ;    Node Test
   ;  This script will tie Port A and Port B together

   ;"# First, create 8 nodes:
   .sim "node  loop_back0"
   .sim "node  loop_back1"
   .sim "node  loop_back2"
   .sim "node  loop_back3"

   .sim "node  loop_back4"
   .sim "node  loop_back5"
   .sim "node  loop_back6"
   .sim "node  loop_back7"

   ;# Now tie the ports together:

   .sim "attach loop_back0 portb0 porta0"
   .sim "attach loop_back1 portb1 porta1"
   .sim "attach loop_back2 portb2 porta2"
   .sim "attach loop_back3 portb3 porta3"

   .sim "attach loop_back4 portb4 porta4"

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start

	CLRF	failures	;Assume success

	MOVLW	0xff
	BSF	STATUS,RP0

	CLRF	TRISA^0x80	;Port A is an output
	MOVWF	TRISB^0x80	;Port B is an input
	BSF	OPTION_REG ^ 0x80, NOT_RBPU	;Disable the pullups

	BCF 	STATUS,RP0


	MOVLW	0x0F

a_to_b_loop:
	MOVWF	PORTA		;Port A and Port B are externally
	XORWF	PORTB,W		;connected. So we should see the
	SKPZ			;same thing on each port.
	 GOTO	FAILED

	DECFSZ	PORTB,W
	 goto	a_to_b_loop

	BSF	PORTA,4		;Port A bit 4 is an open collector.
	BTFSC	PORTB,4
  .assert  "'*** FAILED 16f84 test -RA4 stuck high'"
	 GOTO	FAILED


   ; Now let's write from PORTB to PORTA.
   ; With the configuration bit setting we have, all of PORTA I/O lines
   ; should be able to serve as inputs

	BSF	STATUS,RP0

	COMF	TRISA^0x80,F	;Port A is now an input port
	COMF	TRISB^0x80,F	;Port B is now an output port

	BCF 	STATUS,RP0

	CLRW


b_to_a_loop:
	MOVWF	PORTB		;Port A and Port B are externally
	XORWF	PORTA,W		;connected. So we should see the
	ANDLW	0x1f
	SKPZ			;same thing on each port.
	 GOTO	FAILED

	DECFSZ	PORTB,W
	 goto	b_to_a_loop

   ;
   ; Now test PORTB's internal pullups
   ;

	CLRF	PORTB

	MOVLW	0xff
	BSF	STATUS,RP0

	MOVWF	TRISA^0x80	;Port A is an input
	MOVWF	TRISB^0x80	;Port B is an input

	BSF	OPTION_REG ^ 0x80, NOT_RBPU	;Disable the pullups

	BCF 	STATUS,RP0

	MOVF	PORTB,W
	ANDLW	0x0F
	SKPZ
	 GOTO	FAILED

	BSF	STATUS,RP0
	BCF	OPTION_REG ^ 0x80, NOT_RBPU	;Enable the pullups
	BCF 	STATUS,RP0

	MOVF	PORTB,W		;All lines should be high
	XORLW	0xFF
	SKPZ
	 GOTO	FAILED

	GOTO	done

FAILED:
  .assert  "'*** FAILED 16f84 test'"
	INCF	failures,F
done:
  .assert  "'*** PASSED 16f84 test'"
	GOTO	$


  end
