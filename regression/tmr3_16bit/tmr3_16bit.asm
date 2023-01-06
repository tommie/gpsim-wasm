        ;; tmr3_16bit.asm
        ;;
        ;; The purpose of this program is to test how well gpsim can simulate
        ;; TMR1 for a 16bit-core pic (like the 18fxxx family) although
	;; most of the functionality is generic to all processors where
	;; t1mer1 supports an external clock source.
	;;
        ;; Here are the tests performed:
	;;
	;; -- TMR3 count seconds when driven by simulated external crystal
	;; -- TMR3 driven Fosc/4 with prescale of 8
	;; -- TMR3 count seconds when driven by external stimuli
	;; -- TMR3L and TMR3H can be read and written

	list    p=18f452                ; list directive to define processor
	include <p18f452.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros
        radix   dec                     ; Numbers are assumed to be decimal



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
TMR3_RollOver	RES	1
countLo		RES	1
countHi		RES	1
TMR3H_Sampled	RES	1
TMR3H_Last	RES	1
SecondsLo	RES	1
SecondsHi	RES	1

  GLOBAL countLo, countHi
  GLOBAL SecondsLo, SecondsHi

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


    ;;==================
    ;; TMR3 Interrupt
    ;;==================

	BTFSC	INTCON,T0IF		;If the roll over flag is not set
	 BTFSS	INTCON,T0IE		;or the interrupt is not enabled
	  bra   checkTMR0IntEnd         ;

    ;; TMR0 has rolled over. Clear the pending interrupt and notify
    ;; the foreground code by incrementing the "roll over" counter
    ;; Notice that the increment operation is atomic - this means
    ;; we won't have to worry about the foreground code and interrupt
    ;; code clashing over accesses to this variable.
	
        BCF     INTCON,T0IF,0           ; Clear the pending interrupt
        INCF    TMR0_RollOver,F         ; Set a flag to indicate rollover

checkTMR0IntEnd:

    ;;==================
    ;; TMR3 Interrupt
    ;;==================
	BTFSC	PIR2,TMR3IF
	 BTFSS	PIE2,TMR3IE
	  BRA	checkTMR3IntEnd

	BCF	PIR2,TMR3IF		;Clear the interrupt
	INCF	TMR3_RollOver,F		;Set a flag
	BSF     TMR3H, 7     		; Preload for 1 sec overflow

   ; Note, since we have a 32768 Hz crystal tied to TMR3 and we have the timer
   ; configured as a 16-bit timer TMR3H loaded with 0x80, then this interrupt 
   ; will happen once every second.

	INFSNZ	SecondsLo,F		;Update the system time.
	 INCF	SecondsHi,F


	CLRF	TMR3H_Sampled
	CLRF	TMR3H_Last
checkTMR3IntEnd:

ExitInterrupt:
	RETFIE	1


MAIN	CODE

Start:	

  ;------------------------------------------------------------------------
  ;
  ; Embedded simulation scripts
  ;
  ; Set the clock frequency to 10MHz

  .sim "frequency 10e6"


  ; define a stimulus to generate a 32kHz clock. This will be tied
  ; to TMR3's input and is used to simulate an external drive.
  ;
  ; 40MHz 
  ;   Instruction Rate = (4 clks/instruction) / (10e6 clks/second)
  ;                    = 400nS per instruction
  ;  32.768kHz ==> 30.517uS period
  ;  Instruction Cycles/32.768kHz cycle = 30.517uS / .4uS 
  ;                                     = 76.2925 instructions
  ;

  .sim "stimulus asynchronous_stimulus "
  .sim "initial_state 0"
  .sim "start_cycle 0x2008130"
  .sim "period 305"
  .sim "{ 38,1 ,  76,0 , 114,1 , 152,0 , 190,1 , 228,0 , 266,1}"
  .sim "name Clk32kHz"
  .sim "end"

  ; Create a node

  .sim "node clk_node"

  ; attach the clock

  .sim "attach clk_node  Clk32kHz portc0"



  ; At reset, all bits of t1Con should be clear.
  .assert "t1con == 0x00"

	MOVLW	(1<<RC0)|(1<<RC1)  ;The lower two bits of PORTC are for
	IORWF	TRISC,F		   ;the 32kHz oscillator and should be inputs

	; Load TMR3H, TMR3L with 0x8000 for one second interupts
	CLRF	TMR3L
	MOVLW	0x80
	MOVWF	TMR3H

	BCF	PIR1,TMR3IF	;Clear any TMR3 pending interrupt
	BSF	PIE2,TMR3IE	;Enable TMR3 interrupts
	BSF	INTCON,PEIE	;Enable Peripheral interrupts
	BSF	INTCON,GIE	;Enable Global interrupts

  ; TMR3 not running yet, TMR3H and TMR3L should be unchanged
	MOVF	TMR3L,W		; test read
   .assert "W==0, '*** FAILED 16bit-core TMR3 test TMR3L read'"
	nop
	MOVF	TMR3H,W
   .assert "W==0x80, '*** FAILED 16bit-core TMR3 test TMR3H read'"
	nop

  ; Simulate crystal oscillator for TMR1 external source
  ;
	MOVLW	( (1<<T1OSCEN) | (1<<TMR1CS) )
	MOVWF	T1CON
        MOVLW   (1<<TMR3CS) | (1<<TMR3ON)
        MOVWF	T3CON

	; Delay 16,793,733 about 6.7 seconds with TMR3 counting seconds
	clrf	countLo
	MOVLW	0x10
	MOVWF	countHi
L1:	clrwdt
	rcall	d1
	decfsz	countLo,F
	 bra	L1
	decfsz	countHi,F
	 bra	L1

   .assert "(SecondsLo==6), '*** FAILED 16bit-core TMR3 test bad count with crystal'"
	nop
test2:
	nop

  ; TMR3 on Fosc/4 with prescale of 8
  ;
	; Load TMR3H, TMR3L with 0x8000 for one second interupts
;	BCF	T1CON,TMR3ON ; Microchip recommend stopping couter for writes
	CLRF	TMR3L
	MOVLW	0x80
	MOVWF	TMR3H
	CLRF	SecondsLo

	CLRF	T1CON
        MOVLW	(1<<T3CKPS1) | (1<<T3CKPS0) | (1<<TMR3ON)
	MOVWF	T3CON

	; Delay 16,793,733 cycles period 32768 * 8 gives 64 counts
	clrf	countLo
	MOVLW	0x10
	MOVWF	countHi
L2:	clrwdt
	rcall	d1
	decfsz	countLo,F
	 bra	L2
	decfsz	countHi,F
	 bra	L2

   .assert "(SecondsLo==64), '*** FAILED 16bit-core TMR3 test bad count with crystal'"
	nop

test3:
	nop
	CLRF	SecondsLo
  ; External Stimulus for TMR3
	MOVLW	((1<<TMR1CS) | (1<<TMR3ON))
	MOVWF	T3CON
	nop

	; Delay 33587476 cycles, 13.4 seconds with TMR3 counting seconds
	MOVLW	0x20
	MOVWF	countHi
L3:	clrwdt
	rcall	d1
	decfsz	countLo,F
	 bra	L3
	decfsz	countHi,F
	 bra	L3

   .assert "SecondsLo == 13, '*** FAILED 16bit-core TMR3 test bad count with stimuli'"
	nop
done:
  .assert  "'*** PASSED 16bit-core TMR3 test'"
        bra     $

failed:
        movlw   1
        movwf   failures
  .assert  "'*** FAILED 16bit-core TMR3 test'"
        bra     done

d1	RCALL	d2
d2	RCALL	d3
d3	RCALL	d4
d4	RCALL	d5
d5	RCALL	d6
d6	RCALL	d7
d7	RCALL	d8
d8	RCALL	d9
d9	RCALL	d10
d10	RCALL	d11
d11	RETURN

 end
