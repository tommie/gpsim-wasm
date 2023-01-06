        ;; pm.asm
        ;;

        list    p=16c84
        
include "p16c84.inc"

  cblock  0x0c

        lo,hi,_upper,kz
  endc

ioport  equ     portb
iobit   equ     0

        
        org 0

start:  
        clrf  lo
        clrf  hi
        clrf  _upper     ;Used as a known zero in
                        ;the loop
        call    cnt16bit


        goto    start

measure:
;-----------------------------------------------
;pulse width measurements with 3 Tcyc resolution
;
;The purpose of this routine is to accurately measure
;the width of a pulse. The resolution is 3 instruction
;cycles and the dynamic range is 19 bits or 3*2^19 cycles.
;(That's 1,572,864 cycles which is approximately pi/10 
;seconds on a 20Mhz pic.)
;
;

        btfsc   ioport,iobit
         goto   $-1

loop
   btfsc ioport,iobit   ;If the pulse is over
    goto high0          ;then adjust the count
                        ;
   movlw 1              ;Otherwise, intertwine the counter
                        ;incrementing with the pulse checking
   btfsc ioport,iobit   ;
    goto high1          ;
                        ;
   addwf lo,f           ;Increment the lo byte
                        ;
   btfsc ioport,iobit   ;
    goto high2          ;
                        ;
   rlf   _upper,w        ;Pick up the carry (if lo byte
                        ;overflowed)
   btfsc ioport,iobit   ;
    goto high3          ;
                        ;
   addwf hi,f           ;Increment the high byte
                        ;
   btfsc ioport,iobit   ;
    goto high4          ;
                        ;
   skpc                 ;If the high byte rolls over
    btfsc ioport,iobit  ;or the pulse is over
     goto high5_or_done ;then get out of the loop
                        ;
   clrwdt               ;Could be a nop or some inst.
                        ;
   btfsc ioport,iobit   ;
    goto high6          ;
                        ;
   nop                  ;
                        ;
   btfss ioport,iobit   ;Note that we check the opp. level
    goto loop           ;

  ;fall through... Add 7 to the total count

   incf  _upper,f

high6:
   incf  _upper,f

high5_or_done:
   skpnc            ;If c=1 then we have an
    goto overflow   ;overflow

   incf  _upper,f

high4:
   incf  _upper,f

high3:
   incf  _upper,f

high2:
   decf  lo,f       ;Get rid of the extra
                    ;increment of the lo byte
   incf  _upper,f

high1:
   incf  _upper,f

high0:


   rlf   _upper,f

   rlf   lo,f
   rlf   hi,f
   rlf   _upper,f

   rlf   lo,f
   rlf   hi,f
   rlf   _upper,f

   rlf   lo,f
   rlf   hi,f
   rlf   _upper,f

   swapf _upper,w
   andlw 7
   iorwf lo,f

   movlw 7
   andwf _upper,f

   retlw 0

overflow
   ;If we get here, then there was an overflow.
   ;it turns out that all three bytes of the
   ;counter are zero. Decrementing all three
   ;will set them to 0xff.

   decf  _upper,f
   decf  hi,f
   decf  lo,f

   retlw 0xff


CONST		EQU	0
CNT_PER_LOOP	EQU	7
	
cnt16bit:
        clrf    kz
        clrf    lo
        clrf    hi


        movlw   CNT_PER_LOOP	;preset counts

	btfsc   ioport,iobit
	 goto	$-1

loop1   btfsc   ioport,iobit
         goto   quit1           ;1

        addwf   lo,f            ;inc lo cnt
        btfsc   ioport,iobit
         goto   quit2           ;2

        rlf     kz,w            ;get carry
        btfsc   ioport,iobit
         goto   quit3           ;3-2

        addwf   hi,f            ;hi incremented depends on carry
        btfsc   ioport,iobit
         goto   quit4           ;4-2

        skpc                    ;check if already finished
        btfsc   ioport,iobit
         goto   quit5           ;5-2

        movlw   CNT_PER_LOOP	;add uncounted tests
        btfsc   ioport,iobit
         goto   quit6           ;6-2

        btfss   ioport,iobit
         goto   loop1
                                ;7-2

quit7   incf    kz,f            ;add uncounted tests
quit6   incf    kz,f


quit5   skpnc
         goto   overflow1

        movlw   -CNT_PER_LOOP + 4  ;
	addwf	kz,w
        addwf   lo,f
        skpc
         decf   hi,f
	clrf	kz
	return

quit4   incf    kz,f
        subwf   hi,f    ;counter-act the addwf

quit3   incf    kz,f
quit2
   ;subtract the 7 added in above - the incf's
   ;will keep track of how much needs to be added
   ;back. ignore the C, since the loop didn't get
   ;a chance to handle it.

        movlw   -CNT_PER_LOOP + 1  ;+1 in lieu of incf kz,f
        addwf   lo,f

quit1   incf    kz,w

        addlw	CONST   	;take into account
                                ;initial latency
        addwf   lo,f
        skpnc
         incf   hi,f

        clrf    kz

        return

overflow1
        movlw   0xff
        movwf   lo
        movwf   hi
        clrf    kz
        return


  end
