




	list    p=18f452                ; list directive to define processor
	include <p18f452.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros
        radix   dec                     ; Numbers are assumed to be decimal

        GLOBAL  DelayCycles
        GLOBAL  Delay256N
        GLOBAL  Delay4
        GLOBAL  Delay5
        GLOBAL  Delay6
        GLOBAL  Delay7
        GLOBAL  Delay8
        GLOBAL  Delay9

DELAY_CODE	CODE

;------------------------------------------------------------
; DelayCycles:
;
; Input:   W -- the desired delay
; Output:  Returns after W+7 cycles

DelayCycles:

dc1:	ADDLW	-3	;3-cycle delay loop
	BC	dc1

  ; W now contains either -3 (0xFD), -2 (0xFE) or -1 (0xFF).
  ; The -2 case needs to be delayed an extra cycle more than
  ; the -3 case, and the -1 case needs yet another cycle of delay.
  ;
  ; Examine the bottom two bits and W to determine the exact delay
  ;

	BTFSS   WREG,1
	 BRA	dc2	;W=0xFD - no extra delay needed
	RRCF	WREG,F
	BC	dc2
dc2:    RETURN

Delay256Loop
	RCALL	Delay4
Delay256N:
	RCALL	dc3

	DECFSZ	WREG,F
	 bra	Delay256Loop
Delay5	NOP
Delay4	RETURN

dc3:     RCALL  Delay64
         RCALL  Delay32
         RCALL  Delay16
         RCALL  Delay8
         NOP
Delay128 RCALL  Delay64
Delay64  RCALL  Delay32
Delay32  RCALL  Delay16
Delay16  RCALL  Delay8
Delay8   nop
Delay7   nop
Delay6   bra    Delay4
Delay9   bra    Delay7

  END

