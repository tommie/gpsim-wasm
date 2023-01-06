        ;; it.asm
        ;;
        ;; The purpose of this program is to test how well gpsim can simulate
        ;; a 16bit-core pic (like the 18cxxx family not the 17c family.
        ;; Nothing useful is performed - this program is only used to
        ;; debug gpsim.

	list    p=18f4321                ; list directive to define processor
	include <p18f4321.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA
temp            RES     1
temp1           RES     1
temp2           RES     1
failures        RES     1


  GLOBAL done

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE

        clrf    temp1           ;Assume clrf works...
                                ;
        bra     start

failed1:        ; a relatively local label
        bra     failed

start:  
        ;; Perform some basic tests on some important instructions
	movlw 2
	call  func_lookup_table
  .assert "W == 1, \"*** FAILED Lookup table\""
	nop
done:
  .assert  "'*** PASSED 16bit-core instruction test'"
        bra     $

failed:
        movlw   1
        movwf   failures
  .assert  "'*** FAILED 16bit-core instruction test'"
        bra     done


    org 0xf00
func_lookup_table
    ; Restrict/Mask jump length
    andwf 0x03
    ; Update PCLATH/PCLATU !!!!!! (Without touching WREG)
    btfss PCL, 0
    nop
    ; Perform 
    addwf PCL, F
    retlw 0
    retlw 1
    retlw 2
    retlw 3

    goto failed


        end
