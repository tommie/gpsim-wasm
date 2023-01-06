
   ;;  16F84 test program to generate squares using the 
   ;;  'sum of odd integers' algorithm.


   ;; The main purpose of this program is to illustrate scripting.


   list p=16f84

include "p16f84.inc"

CONFIG_WORD	EQU	_CP_OFF & _WDT_OFF

        __CONFIG  CONFIG_WORD


   ;; Ram 
   cblock 0x20

	count
        odd_int
        x_lo, x_hi
        reg1
        reg2
   endc


	ORG	0


    ;; Simple program to square a number:


    ;; This loop is designed to be easily controlled by the script.
    ;; The script will set a break point at the 'start:' label. Each
    ;; time the breakpoint is encounterd, the previous call to 'square'
    ;; is checked and the input for the next time is initialized.

start:
        movf    count,W
        call    square


        goto    start



        

;;------------------------------------------------------------------------
;;
;; Square a number.
;;
;; INPUT:  W - the number to be squared
;; OUTPUT: x_hi:lo
square:
        clrf    x_lo
        clrf    x_hi

        movwf   count

    ; if the input is 0, the 0*0 = 0
        movf    count,F
        skpnz
         return

        movlw   -1
        movwf   odd_int

square_loop:

    ; Generate the next odd integer:

        movlw   2
        addwf   odd_int,f

    ; Add it to the square so far:

        movf    odd_int,w
        addwf   x_lo,f
        skpnc
         incf   x_hi,f

    ; Are we through?

        decfsz  count,f
         goto   square_loop

        return        

done:
	GOTO	$


  end
