	;; it.asm
	;;
	;; The purpose of this program is to test how well gpsim can simulate
        ;; instructions on a mid-range (14bit core) Pic.


	list    p=16f628                ; list directive to define processor
	include <p16f628.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA
temp            RES     1
temp1           RES     1
temp2           RES     1


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
	clrf	temp1		;Assume clrf works...
				;
	;;
	;; map tmr0 to the internal clock 
	;; 
	clrw
	option			;initialize tmr0

BankLoop:	
	;; Perform some basic tests on some important instructions

	setc			;set the Carry flag
	skpc			;and verify that it happened
	 goto	failed

	clrc			;Now try clearing the carry
	skpnc
	 goto	failed

	setz			;set the Zero flag
	skpz			;and verify that it happened
	 goto	failed

	clrz			;Now try clearing it
	skpnz
	 goto	failed
	
	setdc			;set the Digit Carry flag
	skpdc			;and verify that it happened
	 goto	failed

	clrdc			;Now try clearing it
	skpndc
	 goto	failed

	movlw	1
	movwf	temp
	movf	temp,f
	skpnz
	 goto   failed

	movlw	0
	movwf	temp
	movf	temp,f
	skpz
	 goto   failed
	
	clrf	temp		;Clear a register
	skpz			;and verify that the Z bit in the
	 goto	failed		;status register was set.

	movf	temp,w		;Read the register that was just
	skpz			;cleared, and again verify that Z
	 goto	failed		;was set

	incf	temp,w		;Now this should cause the Z bit to
	skpnz			;get cleared.
	 goto	failed

	movlw	0xff
	movwf	temp
	incf	temp,f

	skpz
	 goto	failed

	;;
	;; incfsz
	;;

	movlw	0xff
	movwf	temp
	movf	temp,f		;Should clear Z
	incfsz	temp,w
	 goto	failed

	skpnz			;incfsz shouldn't affect Z
	 goto	failed

	incfsz	temp,f		;temp should now be zero
	 goto	failed

	incfsz	temp,w		;the following inst. shouldn't be skipped
	 skpnz			;Z should be clear from above.
	  goto	failed
		
	
	;; 	addlw	0xaa

	;; 
	;; addwf test:
	;;
	;; The purpose of this test is to a) verify that the addwf instruction
	;; adds correctly and b) that the appropriate status register bits are
	;; set correctly.

	clrw
	clrf	temp
	
	addwf	temp,w		; 0+0
	skpnc
	 goto	failed
	skpz
	 goto	failed
	skpndc
	 goto	failed

	movlw	1		; 0+1
	addwf	temp,w
	
	skpnc
	 goto	failed
	skpnz
	 goto	failed
	skpndc
	 goto	failed

	movlw	8		; 8+8
	movwf	temp
	addwf	temp,w
	
	skpnc
	 goto	failed
	skpnz
	 goto	failed
	skpdc
	 goto	failed

	movlw	0x88		; 0x88+0x88
	movwf	temp
	addwf	temp,w
	
	skpc
	 goto	failed
	skpnz
	 goto	failed
	skpdc
	 goto	failed

	movlw	0x80		; 0x80+0x80
	movwf	temp
	addwf	temp,w
	
	skpc
	 goto	failed
	skpz
	 goto	failed
	skpndc
	 goto	failed

	movlw	1
	movwf	temp
	movlw	0xff
	addwf	temp,w

	skpc
	 goto	failed
	skpz
	 goto	failed
	skpdc
	 goto	failed


	clrc
	clrdc
		
	;;
	;; andwf test:
	;;
	clrf	temp
	movlw	1
	andwf	temp,f

	skpz
	 goto	failed

	skpc
	 skpndc
	  goto  failed

	andwf	temp,w

	skpz
	 goto	failed

	skpc
	 skpndc
	  goto  failed

	movlw	1
	movwf	temp
	andwf	temp,f

	skpnz
	 goto	failed

	skpc
	 skpndc
	  goto  failed

	movlw	0xff
	andwf	temp,f		;1 & 0xff should = 1

	skpnz
	 goto	failed

	skpc
	 skpndc
	  goto  failed

	movlw	0xff		;Now add 0xff to see if temp
	addwf	temp,f		;really is 1. (should cause a carry)

	skpnz
	 skpc
	  goto	failed

	;;
	;; iorwf
	;;

	movlw	0
	movwf	temp
	iorwf	temp,f
	skpz
	 goto	failed

	movlw	1
	iorwf	temp,f
	skpnz
	 goto	failed

	movlw	0xff
	addwf	temp,f
	skpnz
	 skpc
	  goto	failed


	;;
	;; rlf
	;;

	clrdc
	clrf	temp
	clrc
	rlf	temp,w
	
	skpdc
	 skpnc
	  goto	failed

	skpz
	 goto	failed

	setc
	rlf	temp,f

	skpdc
	 skpnc
	  goto	failed

	skpz
	 goto	failed

	movlw	0xff
	addwf	temp,f
	skpnz
	 skpc
	  goto	failed

	rlf	temp,f
	skpnc
	 goto	failed
	
	movlw	0x80
	movwf	temp
	rlf	temp,w

	skpc
	 goto	failed
	movwf	temp
	addwf	temp,w
	skpz
	 goto	failed

	;;
	;; rrf
	;;

	clrdc
	clrf	temp
	clrc
	rrf	temp,w
	
	skpdc
	 skpnc
	  goto	failed

	skpz
	 goto	failed

	setc
	rrf	temp,f

	skpdc
	 skpnc
	  goto	failed

	skpz
	 goto	failed

	movlw	0x80
	addwf	temp,f
	skpnz
	 skpc
	  goto	failed

	rrf	temp,f
	skpnc
	 goto	failed
	
	movlw	1
	movwf	temp
	rrf	temp,w

	skpc
	 goto	failed
	movwf	temp
	addwf	temp,w
	skpz
	 goto	failed


	;; 
	;; subwf test:
	;;

	clrw
	clrf	temp
	
	subwf	temp,w		; 0-0
	skpc
	 goto	failed
	skpz
	 goto	failed
	skpdc
	 goto	failed

	movlw	1		; 0-1
	subwf	temp,w
	
	skpnc
	 goto	failed
	skpnz
	 goto	failed
	skpndc
	 goto	failed

	movlw	0x10		; 0x10-0x10
	movwf	temp
	subwf	temp,w
	
	skpc
	 goto	failed
	skpz
	 goto	failed
	skpdc
	 goto	failed

	movlw	0x88		; 0x88-0x88
	movwf	temp
	subwf	temp,w
	
	skpc
	 goto	failed
	skpz
	 goto	failed
	skpdc
	 goto	failed

	clrf	temp
	movlw	0x80		; 0 - 0x80
	subwf	temp,f
	
	skpnc
	 goto	failed
	skpnz
	 goto	failed
	skpdc
	 goto	failed

	movlw	0		; 0x80 - 0
	subwf	temp,w

	skpc
	 goto	failed
	skpnz
	 goto	failed
	skpdc
	 goto	failed

	movlw	1		; 0x80 - 1
	subwf	temp,w

	skpc
	 goto	failed
	skpnz
	 goto	failed
	skpndc
	 goto	failed
	

	clrc
	clrdc

	;; 
	;; xorwf
	;;

	clrf	temp
	movlw	0xff
	xorwf	temp,f

	skpc
	 skpndc
	  goto	failed

	skpnz
	 goto	failed

	xorwf	temp,f

	skpz
	 goto	failed

	xorwf	temp,f
	incfsz	temp,f
	 goto	failed

	;;
	;; swapf
	;;

	movlw	0x0f
	movwf	temp
	swapf	temp,f
	xorwf	temp,f
	incfsz	temp,f
	 goto	failed

	decf	temp1,f
	incfsz	temp1,w
	 goto	done

	;; repeat the test with in the next bank
	
	bsf	STATUS,RP0
	goto	BankLoop
	
done:
  .assert  "'*** PASSED MidRange core instruction test'"
	bcf	STATUS,RP0
	
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	
	goto	$
	
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
		
	andlw	0xaa
	andwf	temp,w
	bcf	temp,5
	bsf	temp,5
	btfsc	temp,5
	btfss	temp,5
	call	routine
	clrf	temp
	clrw
	clrwdt
	comf	temp,w
	decf	temp,w
	decfsz	temp,w

	goto	l1
	nop
	nop
	nop
l1:	
	incf	temp,w
	incfsz	temp,w
	iorlw	0xaa
	iorwf	temp,w
	movlw	0xaa
	movf	temp,w
	movwf	temp
	nop
	option


	goto	r1
		
	retfie
	retlw   0xaa
routine
	return
r1:

	rlf	temp,w
	rrf	temp,w
	sleep
	sublw	0xaa
	subwf	temp,w
	swapf	temp,w
	tris	5
	xorlw	0xaa
	xorwf	temp,w
	

	;;
	;; Test indirect branching
	;;
test_pcl:
	
	clrf	temp
	clrf	temp1

tt1:
	movf	temp,w
	call	table_test

	xorwf	temp,w
	skpz
	 bsf	temp1,0

	incf	temp,f
	btfss	temp,4
	 goto	tt1

	goto	test_pcl2
table_test:
	addwf	PCL,f
table:
	retlw	0
	retlw	1
	retlw	2
	retlw	3
	retlw	4
	retlw	5
	retlw	6
	retlw	7
	retlw	8
	retlw	9
	retlw	10
	retlw	11
	retlw	12
	retlw	13
	retlw	14
	retlw	15

test_pcl2:
	
	btfsc	temp2,0
	 goto	test_pcl3
        movlw   0x20
        movwf   FSR
next    clrf    INDF
        incf    FSR,1
        btfss   FSR,4
        goto    next

	bsf	temp2,0
        clrf    PCL

test_pcl3:

	movlw	LOW(ly)
	movwf	PCL		; == goto ly
lx
	goto	test_pcl4
ly
	movlw	LOW(lx)
	movwf	PCL

test_pcl4:
	
	goto	$

failed:
  .assert  "'*** FAILED MidRange core instruction test'"
	goto	done

	end
