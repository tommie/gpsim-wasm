        radix dec
        ;; The purpose of this program is to test gpsim macros
        ;;

	list    p=16f84                 ; list directive to define processor
	include <p16f84.inc>            ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros


GPR_DATA        UDATA
failures        RES     1
mac_count       RES     1
mac_flags       RES     1

 GLOBAL  mac_count, mac_flags, failures
 GLOBAL  mac_loop

;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlw  high  start               ; load upper byte of 'start' label
        movwf  PCLATH                    ; initialize PCLATH
        goto   start                     ; go to beginning of program
        
;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start
        clrf    failures
        clrf    mac_count           ; a counter
	clrf	mac_flags

mac_loop:
	incf	mac_count,F
	btfss	mac_flags,4
	 goto	mac_loop


        movf	failures,W
	skpz
failed:	
  .assert  "'*** FAILED Macro test'"
	 goto	$
done:
  .assert  "'*** PASSED Macro test'"
        goto    done

        end
