
        radix dec
        ;; The purpose of this program is to test gpsim's register stimuli.
        ;;

	list    p=16f84                 ; list directive to define processor
	include <p16f84.inc>            ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA
failures    RES     1
shift_in    RES     1  ; A shifting bit pattern stimulus is connected to this.
flag_reg    RES     1  ; Another register stimulus is tied to this.
temp1       RES     1

  GLOBAL  shift_in
  GLOBAL  flag_reg


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
        clrf    shift_in
        clrf    flag_reg
        clrf    temp1

start_loop:

   ; First, wait for shift_in to become '1'

        decfsz  shift_in,W
         goto   start_loop

        movf    flag_reg,W
        movwf   temp1

   ; The register stimulus will shift the register 'shift_in' 1 bit position left every
   ; 100 cycles. The final value will stick at 0x80. Meanwhile, the flag_reg will 
   ; will become 0xff after about 1000 cycles.

shift_loop

        movf    shift_in,W      ; Compare shift_in to it's last known value.
        xorwf   temp1,W

        skpnz                   ; If it has changed, then update it and check it.
         goto   L1

        xorwf   temp1,F         ; temp1 = temp1^(shift_in^temp1) = shift_in

        decf    temp1,W         ; temp1 should be 2^n
        andwf   temp1,W

        skpz
         goto   failed          ; failed

L1:   
        incfsz  flag_reg,W      ; does flag == 0xff?
         goto   shift_loop
        

   ; loop is finished. temp1 should be 0x80.

        movlw   0x80
        xorwf   temp1,W
        skpz
         goto	failed

done:
  .assert  "'*** PASSED Register Stimulation test'"
        goto    done

failed:	
  .assert  "'*** FAILED Register Stimulation test'"
	goto	failed
        end
