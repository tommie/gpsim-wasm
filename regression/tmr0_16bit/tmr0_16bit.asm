        ;; tmr0_16bit.asm
        ;;
        ;; The purpose of this program is to test how well gpsim can simulate
        ;; TMR0 for a 16bit-core pic (like the 18fxxx family)
        ;; Here are the tests performed:
	;;
	;; -- TMR0L and TMR0H can be read and written
	;; -- Writing to TMR0L 

	list    p=18f452                ; list directive to define processor
	include <p18f452.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros
        radix   dec                     ; Numbers are assumed to be decimal

	include "delay.inc"	; Defines the "DELAY" macro


;----------------------------------------------------------------------

; Printf Command
.command macro x
  .direct "C", x
  endm

;----------------------------------------------------------------------
GPR_DATA                UDATA
temp            RES     1
temp1           RES     1
temp2           RES     1
failures        RES     1

TMR0_RollOver	RES	1
tmr0Lo		RES	1
tmr0Hi		RES	1
psa		RES	1
b16bitMode	RES	1
countLo		RES	1
countHi		RES	1

  GLOBAL tmr0Lo, tmr0Hi
  GLOBAL psa,b16bitMode
  GLOBAL done
  GLOBAL TMR0_RollOver
  GLOBAL countLo, countHi


;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
STARTUP    CODE	0

	bra	Start

;------------------------------------------------------------------------
;
;  Interrupt Vector
;
;------------------------------------------------------------------------

INT_VECTOR   CODE    0x008               ; interrupt vector location


check_TMR0_interrupt:

	BTFSC	INTCON,T0IF		;If the roll over flag is not set
	 BTFSS	INTCON,T0IE		;or the interrupt is not enabled
	  RETFIE 1			; Then leave

    ;; TMR0 has rolled over. Clear the pending interrupt and notify
    ;; the foreground code by incrementing the "roll over" counter
    ;; Notice that the increment operation is atomic - this means
    ;; we won't have to worry about the foreground code and interrupt
    ;; code clashing over accesses to this variable.
	
        BCF     INTCON,T0IF,0           ; Clear the pending interrupt
        INCF    TMR0_RollOver,F         ; Set a flag to indicate rollover

ExitInterrupt:
	RETFIE	1


MAIN	CODE

Start:	
  ; At reset, all bits of t0Con should be set.
  .assert "t0con == 0xff"
	nop

  ;------------------------------------------------------------
  ; The high byte of TMR0 is exposed to the firmware as a shadow register. When
  ; firmware writes to this register, the value is latched. When the firmware
  ; writes the to the low byte, then the latched high byte will get copied
  ; along with the low byte to TMR0. This allows all 16-bits to be written in a
  ; single cycle. Similarly, when TMR0L is read, the high byte of the timer
  ; is copied to the shadowed register. 

TMR0H_ShadowRegisterTest:
  .command  "echo *** Testing TMR0H"

	MOVLW	42
	MOVWF	TMR0H
  .assert "tmr0h == 42"
	nop

	MOVWF	TMR0L
  .assert "tmr0l == 42"
	nop

	MOVF	TMR0L,W
  .assert "tmr0l == 42"
	nop

	MOVF	TMR0H,W
  .assert "tmr0h == 42"
	nop

    ; Clear the shadowed TMR0H register and verify that it cleared.
	CLRF	TMR0H
  .assert "tmr0h == 0"
	nop

    ; Now, when we read the low byte of Timer 0, the high byte gets
    ; refreshed. So let's check that TMR0H changes due to a TMR0L read:
	MOVF	TMR0L,W
  .assert "tmr0l == 42"
	nop
	MOVF	TMR0H,W
  .assert "tmr0h == 42"
	NOP

TMR0_8BitModeTest:

 .command  "echo *** Testing 8-bit mode"

  ; Stop the timer and clear its value
	CLRF	T0CON
	CLRF	TMR0L
	CLRF	TMR0H
  .assert "(tmr0l == 0) && (tmr0h == 0)"
	nop

  ; The timer is off, so any value written to it should be read back
  ; unchanged.
	MOVLW	42
	MOVWF	TMR0L
  .assert "tmr0l == 42"
	NOP

	MOVWF	TMR0H
  .assert "tmr0h == 42"
	NOP
	MOVWF	tmr0Hi
  ;------------------------------------------------------------
  ; 8-bit mode tests
  ;
  ; First, TMR0 is tested in 8-bit mode and with interrupts disabled.
  ; All 8 combinations of PSA are tested.

	CLRF	psa	; Shadow's the PSA bits of T0CON

L1_tmr0_8BitModeTest:   ; Beginning of the loop

    ; Start off with T0CON and TMR0L in a known state.

  .command  "psa"
	CLRF	T0CON
	CLRF	TMR0L
	CLRF	tmr0Lo

    ; Assume that the PSA is not assigned to TMR0:

	MOVLW	(1<<TMR0ON) | (1<<T08BIT) | (1<<PSA)

    ; if psa is non-zero then the prescaler *is* assigned to tmr0
	MOVF	psa,F
	BZ	L_psaInitComplete

    ; psa is one greater than the actually Prescale bits
	DECF	psa,W
	ANDLW	7
	IORLW	(1<<TMR0ON) | (1<<T08BIT)
		
L_psaInitComplete:
	MOVWF	T0CON
    ;
    ; tmr0 is running now. It should increment once every PSA cycles.
    ;
    ; First read the whole 16-bit counter note

	MOVF	TMR0L,W
	MOVF	TMR0H,W
  .assert "W == tmr0Hi"
	NOP

    ; Now we'll check TMR0L after various delays. The assertion works
    ; by reading tmr0l and comparing it to the number of cycles that
    ; have gone by since the timer started. The shift right by 'psa' allows
    ; the same assertion to be used as we loop through all combinations
    ; of psa.

    ; Single cycle checks.

  .assert "tmr0l == ((tmr0Lo+4)>>psa)"
	NOP
  .assert "tmr0l == ((tmr0Lo+5)>>psa)"
	NOP
  .assert "tmr0l == ((tmr0Lo+6)>>psa)"
	NOP
  .assert "tmr0l == ((tmr0Lo+7)>>psa)"
	NOP
  .assert "tmr0l == ((tmr0Lo+8)>>psa)"
	NOP
  .assert "tmr0l == ((tmr0Lo+9)>>psa)"
	NOP
  .assert "tmr0l == ((tmr0Lo+10)>>psa)"

  ; 8 23-cycle delays.

	MOVLW	23 - 10
	RCALL	DelayCycles
  .assert "tmr0l == ((tmr0Lo+33)>>psa )"

	MOVLW	23 - 10
	RCALL	DelayCycles
  .assert "tmr0l == ((tmr0Lo+56)>>psa )"

	MOVLW	23 - 10
	RCALL	DelayCycles
  .assert "tmr0l == ((tmr0Lo+79)>>psa )"

	MOVLW	23 - 10
	RCALL	DelayCycles
  .assert "tmr0l == ((tmr0Lo+102)>>psa )"

	MOVLW	23 - 10
	RCALL	DelayCycles
  .assert "tmr0l == ((tmr0Lo+125)>>psa )"

	MOVLW	23 - 10
	RCALL	DelayCycles
  .assert "tmr0l == ((tmr0Lo+148)>>psa )"

	MOVLW	23 - 10
	RCALL	DelayCycles
  .assert "tmr0l == ((tmr0Lo+171)>>psa )"

	MOVLW	23 - 10
	RCALL	DelayCycles
  .assert "tmr0l == ((tmr0Lo+194)>>psa )"

  ; 255 and 256 cycle boundary condition

	MOVLW	61 - 10
	RCALL	DelayCycles
  .assert "tmr0l == ((tmr0Lo+255)>>psa )"
	NOP

  .assert "tmr0l == (((tmr0Lo+256)>>psa)&0xff)"
	nop

  ; now for some bigger delays.

	DELAY  250
  .assert "tmr0l == (((tmr0Lo+256+251)>>psa)&0xff)"
	nop

	DELAY  250
  .assert "tmr0l == (((tmr0Lo+256+2*251)>>psa)&0xff)"
	nop

	DELAY  250
  .assert "tmr0l == (((tmr0Lo+256+3*251)>>psa)&0xff)"
	nop

	DELAY  250
  .assert "tmr0l == (((tmr0Lo+256+4*251)>>psa)&0xff)"
	nop

	DELAY  250
  .assert "tmr0l == (((tmr0Lo+256+5*251)>>psa)&0xff)"
	nop

	DELAY  250
  .assert "tmr0l == (((tmr0Lo+256+6*251)>>psa)&0xff)"
	nop

	DELAY  250
  .assert "tmr0l == (((tmr0Lo+256+7*251)>>psa)&0xff)"
	nop

	DELAY  250
  .assert "tmr0l == (((tmr0Lo+256+8*251)>>psa)&0xff)"
	nop

	DELAY  8190
  .assert "tmr0l == (((tmr0Lo+256+8*251+8191)>>psa)&0xff)"
	nop

	INCF	psa,F
	btfss	psa,3
	 bra	L1_tmr0_8BitModeTest

    ; Through looping through all combinations of PSA.
    ; Now make sure that the high byte of TMR0 hasn't changed.
	MOVF	TMR0H,W
  .assert "W == tmr0Hi"
	NOP


;------------------------------------------------------------
; Interrupt tests

	NOP
	NOP
	NOP


	CLRF	T0CON		;Stop TMR0
	CLRF	INTCON		;Clear any pending interrupts
	BSF	INTCON,T0IE	;Enable TMR0 overflow interrupts
	BSF	INTCON,GIE	;Enable global interrupts

	CLRF	psa		;Loop counter and prescaler assignment.

L1_InterruptTest:

    ; Stop and clear TMR0:
	CLRF	T0CON
	CLRF	TMR0H
	CLRF	TMR0L
	CLRF	TMR0_RollOver   ;Interrupt flag

    ;Software counter to count cycles

	SETF	countLo		;Start off with counter==-1
	SETF	countHi

    ; Assume that the PSA is not assigned to TMR0:

	MOVLW	(1<<TMR0ON) | (1<<T08BIT) | (1<<PSA)

    ; if psa is non-zero then the prescaler *is* assigned to tmr0
	MOVF	psa,F
	BZ	L_psaInitComplete2

    ; psa is one greater than the actually Prescale bits
	DECF	psa,W
	ANDLW	7
	IORLW	(1<<TMR0ON) | (1<<T08BIT)
L_psaInitComplete2:
	MOVWF	T0CON

    ; Now enter into a loop and wait for the interrupt.
    ; Each loop iteration takes exactly 8 cycles.

TMR0_WaitForInt:
	NOP
	NOP
	clrwdt
	INFSNZ	countLo,F
	 INCF	countHi,F
        BTFSS   TMR0_RollOver,0
	 bra	TMR0_WaitForInt

  .assert " (((countHi<<8)+countLo)>>(5+psa)) == 1"
	nop
	INCF	psa,F
	btfss	psa,3
	 bra	L1_InterruptTest

;
;	test that t0 continues to run after sleep woken up by t1
	nop
	bsf	T0CON,PSA  	; bypass precounter
	bsf	T1CON,T1SYNC	;sync off
	bsf	T1CON,TMR1ON	; start timer 1
	bsf	PIE1,TMR1IE	; turn on interrupt
	sleep
	movf	TMR0L,W
	bcf	T1CON,TMR1ON	; stop timer 1
	nop
	nop
  .assert "tmr0l != W, '*** FAILED 16bit-core TMR0 test- TMR0 stopped after sleep'"
	nop

;
; TMR0 should not interrupt during sleep as it is turned off
; The WDT cancels the sleep
;
	clrf	TMR0_RollOver
	sleep
	nop
  .assert "TMR0_RollOver == 0, '*** FAILED 16bit-core TMR0 test- TMR0 interrupt in sleep'"
	nop

;------------------------------------------------------------
; 16bit-mode tests

	CLRF	psa

L1_tmr0_16BitModeTest:

	CLRF	T0CON
	CLRF	TMR0H
	CLRF	TMR0L

    ; Assume that the PSA is not assigned to TMR0:

	MOVLW	(1<<TMR0ON) | (1<<PSA)

    ; if psa is non-zero then the prescaler *is* assigned to tmr0
	MOVF	psa,F
	BZ	L_psa16bitInitComplete

    ; psa is one greater than the actually Prescale bits
	DECF	psa,W
	ANDLW	7
	IORLW	(1<<TMR0ON)
		
L_psa16bitInitComplete:
  .command  "psa"
	MOVWF	T0CON


	MOVF	TMR0L,W
  .assert "( ((tmr0h<<8)+W) == (1>>psa))"

	MOVF	TMR0L,W
  .assert "( ((tmr0h<<8)+W) == (2>>psa))"

	MOVF	TMR0L,W
  .assert "( ((tmr0h<<8)+W) == (3>>psa))"

	MOVF	TMR0L,W
  .assert "( ((tmr0h<<8)+W) == (4>>psa))"

	MOVF	TMR0L,W
  .assert "( ((tmr0h<<8)+W) == (5>>psa))"


	MOVF	TMR0L,W
	MOVWF	tmr0Lo

  .command  "tmr0Lo"
  .command  "tmr0h"

	DELAY	1024-2
	MOVF	TMR0L,W
	MOVWF	tmr0Lo

  .command  "tmr0Lo"
  .command  "tmr0h"

	DELAY	8192-2
	MOVF	TMR0L,W
	MOVWF	tmr0Lo

  .command  "tmr0Lo"
  .command  "tmr0h"

	INCF	psa,F
	btfss	psa,3
	 bra	L1_tmr0_16BitModeTest

	nop
done:
  .assert  "'*** PASSED 16bit-core TMR0 test'"
        bra     $

failed:
        movlw   1
        movwf   failures
  .assert  "'*** FAILED 16bit-core TMR0 test'"
        bra     done

 end
