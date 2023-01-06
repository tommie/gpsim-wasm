

        EXTERN  DelayCycles
        EXTERN  Delay256N
        EXTERN  Delay4
        EXTERN  Delay5
        EXTERN  Delay6
        EXTERN  Delay7
        EXTERN  Delay8
        EXTERN  Delay9


;------------------------------------------------------------------------
; DELAY macro
;
; The purpose of this macro is to delay exactly "delay" number of instruction
; cycles, where:	
;
;
;        0 < delay < 65547
;
DELAY	macro	delay
  
   if delay == 1
	NOP
   else
    if delay == 2
        BRA	$+2
    else
     if delay == 3
	NOP
        BRA	$+2
     else
      if delay<10
	RCALL	Delay#v(delay)
      else
       if delay < 265
	 MOVLW	delay-10
	 RCALL	DelayCycles
       else
        if delay < 274
          RCALL Delay9
	  MOVLW delay-10-9
          RCALL DelayCycles
        else

	  MOVLW (delay-11) & 0xff
	  RCALL DelayCycles
	  MOVLW (delay-11)>>8
	  RCALL Delay256N
        endif
       endif
      endif
     endif
    endif
   endif

  endm

