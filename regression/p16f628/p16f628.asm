
   ;;  16F628 tests
   ;;
   ;; This regression test exercises the 16f628.  
   ;; 
   ;;  Tests performed:
   ;;
   ;;  1) Verify Comparators can be disabled and that
   ;;     PORTA can work as a digital I/O port.


   list p=16f628a

include "p16f628a.inc"

CONFIG_WORD	EQU	_CP_OFF & _WDT_OFF & _INTRC_OSC_NOCLKOUT & _MCLRE_OFF

        __CONFIG  CONFIG_WORD

   cblock 0x20

	failures

   endc


	ORG	0

	CLRF	failures	;Assume success

	MOVLW	0xff
	BSF	STATUS,RP0

	CLRF	TRISA^0x80	;Port A is an output
	MOVWF	TRISB^0x80	;Port B is an input

	CLRF	VRCON^0x80	;Turn off the Voltage Reference module.

	BCF 	STATUS,RP0

   ;;
   ;; Turn off the comparator module
   ;;
 
	MOVLW	(1<<CM0) | (1<<CM1) | (1<<CM2)
	MOVWF	CMCON

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
	 GOTO	FAILED

   ; Test the upper three bits

	BSF	PORTA,5		;RA5 is an input only pin
	BTFSC	PORTB,5		;
	 GOTO	FAILED

	BSF	PORTA,6
	BTFSS	PORTB,6
	 GOTO	FAILED

	BSF	PORTA,7
	BTFSS	PORTB,7
	 GOTO	FAILED


   ; Now let's write from PORTB to PORTA.
   ; With the configuration bit setting we have, all of PORTA I/O lines
   ; should be able to serve as inputs

	BSF	STATUS,RP0

	COMF	TRISA^0x80,F	;Port A is now an input port
	COMF	TRISB^0x80,F	;Port B is now an output port

	BCF 	STATUS,RP0

	CLRW

MCLR_CFG_BIT	EQU	_MCLRE_ON ^ _MCLRE_OFF


b_to_a_loop:
	MOVWF	PORTB		;Port A and Port B are externally
	XORWF	PORTA,W		;connected. So we should see the

   if	CONFIG_WORD & MCLR_CFG_BIT
	ANDLW	~(1<<5)		;If MCLR is enabled, RA5 will read as a zero
   endif

	SKPZ			;same thing on each port.
	 GOTO	FAILED

	DECFSZ	PORTB,W
	 goto	b_to_a_loop


   if	!(CONFIG_WORD & MCLR_CFG_BIT)

	BSF	PORTB,5		;If MCLR is enabled, RA5 should read as a zero
	BTFSC	PORTA,5
	 GOTO	FAILED
	BCF	PORTB,5
	BTFSC	PORTA,5
	 GOTO	FAILED

   endif


	GOTO	done

FAILED:
	INCF	failures,F
done:
	GOTO	$


  end
