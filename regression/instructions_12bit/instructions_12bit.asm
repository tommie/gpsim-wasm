
        ;; it.asm
        ;;
        ;; The purpose of this program is to test how well gpsim can simulate
        ;; a pic. Nothing useful is performed - this program is only used to
        ;; debug gpsim.
        ;; The way it is structured, a test is performed and then various
        ;; status bits are examined. If things 'are as expected' then the
        ;; next test will be performed, otherwise the program will go into
        ;; an infinite loop (goto $).
        ;; If the 'success:' label is reached, then all of the tests were
        ;; successful.

        
	list    p=12c508                ; list directive to define processor
	include <p12c508.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

  __config _WDT_ON & _MCLRE_OFF

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


        ;; Assume that 'goto' works and begin the thorough instruction
        ;; tests below (the pcl related tests must be in bank 0 because
        ;; the 5x hardware cannot make calls to bank 1 [bit 8 of the PC
        ;; always gets cleared]).

        goto    start

        
        ;;
        ;; Test indirect branching
        ;;
test_pcl:
        
        clrf    temp
        clrf    temp1


tt1:
        movf    temp,w
        call    table_test

        xorwf   temp,w
        skpz
         bsf    temp1,0

        incf    temp,f
        btfss   temp,4
         goto   tt1

        goto    test_pcl2
table_test:
        addwf   PCL,F
table:
        retlw   0
        retlw   1
        retlw   2
        retlw   3
        retlw   4
        retlw   5
        retlw   6
        retlw   7
        retlw   8
        retlw   9
        retlw   10
        retlw   11
        retlw   12
        retlw   13
        retlw   14
        retlw   15

test_pcl2:

        ;; If this is the first time through, then clear the registers
        btfsc   temp2,0
         goto   test_pcl3
        movlw   0x10
        movwf   FSR
next    clrf    INDF
        incf    FSR,f
        btfss   FSR,5
         goto    next

        ;;
        ;; Set this flag to indicate we've gotten here once already
        ;; (note: we're assuming that temp2 was cleared at the beginning
        ;; of the simulation. By default, gpsim clears ram at 
        ;; initialization. If this is not true, then you might have
        ;; to manually clear temp2.
        
        bsf     temp2,0
        clrf    PCL

test_pcl3:

        movlw   ly
        movwf   PCL             ; == goto ly
lx
        goto    test_pcl4
ly
        movlw   lx
        movwf   PCL

test_pcl4:

        movlw   0
        tris    GPIO

        NOP
        NOP
        NOP
        NOP

        ;;
        ;; If we get here, then all of the tests were passed
        ;;
success:
done:	        
  .assert  "'*** PASSED 12bit core instruction test'"
        goto    $       

        NOP
        NOP
        NOP
        NOP

failed: 
        movlw   1
        movwf   failures
  .assert  "'*** FAILED: 12bit core instruction test'"
	goto	done
start:  

        ;;
        ;; if bit 0 of temp2 is set, then we've already passed
        ;; through here once. The reason we got here a second
        ;; time is because of the 'clrf pcl' test (which is basically
        ;; a 'goto 0' instruction).

        btfsc   temp2,0
         goto   test_pcl

        clrf    temp1           ;Assume clrf works...
        clrf    failures
                                ;
        ;;
        ;; map tmr0 to the internal clock 
        ;; 
        clrw
        option                  ;initialize tmr0

start1: 
        ;; Perform some basic tests on some important instructions

        setc                    ;set the Carry flag
        skpc                    ;and verify that it happened
         goto   failed

        clrc                    ;Now try clearing the carry
        skpnc
         goto   failed

        setz                    ;set the Zero flag
        skpz                    ;and verify that it happened
         goto   failed

        clrz                    ;Now try clearing it
        skpnz
         goto   failed
        
        setdc                   ;set the Digit Carry flag
        skpdc                   ;and verify that it happened
         goto   failed

        clrdc                   ;Now try clearing it
        skpndc
         goto   failed

        movlw   1
        movwf   temp
        movf    temp,f
        skpnz
         goto   failed

        movlw   0
        movwf   temp
        movf    temp,f
        skpz
         goto   failed
        
        clrf    temp            ;Clear a register
        skpz                    ;and verify that the Z bit in the
         goto   failed          ;status register was set.

        movf    temp,w          ;Read the register that was just
        skpz                    ;cleared, and again verify that Z
         goto   failed          ;was set

        incf    temp,w          ;Now this should cause the Z bit to
        skpnz                   ;get cleared.
         goto   failed

        movlw   0xff
        movwf   temp
        incf    temp,f

        skpz
         goto   failed

        ;;
        ;; incfsz
        ;;

        movlw   0xff
        movwf   temp
        movf    temp,f          ;Should clear Z
        incfsz  temp,w
         goto   failed

        skpnz                   ;incfsz shouldn't affect Z
         goto   failed

        incfsz  temp,f          ;temp should now be zero
         goto   failed

        incfsz  temp,w          ;the following inst. shouldn't be skipped
         skpnz                  ;Z should be clear from above.
          goto  failed
                
        
        ;;      addlw   0xaa

        ;; 
        ;; addwf test:
        ;;
        ;; The purpose of this test is to a) verify that the addwf instruction
        ;; adds correctly and b) that the appropriate status register bits are
        ;; set correctly.

        clrw
        clrf    temp
        
        addwf   temp,w          ; 0+0
        skpnc
         goto   failed
        skpz
         goto   failed
        skpndc
         goto   failed

        movlw   1               ; 0+1
        addwf   temp,w
        
        skpnc
         goto   failed
        skpnz
         goto   failed
        skpndc
         goto   failed

        movlw   8               ; 8+8
        movwf   temp
        addwf   temp,w
        
        skpnc
         goto   failed
        skpnz
         goto   failed
        skpdc
         goto   failed

        movlw   0x88            ; 0x88+0x88
        movwf   temp
        addwf   temp,w
        
        skpc
         goto   failed
        skpnz
         goto   failed
        skpdc
         goto   failed

        movlw   0x80            ; 0x80+0x80
        movwf   temp
        addwf   temp,w
        
        skpc
         goto   failed
        skpz
         goto   failed
        skpndc
         goto   failed

        movlw   1
        movwf   temp
        movlw   0xff
        addwf   temp,w

        skpc
         goto   failed
        skpz
         goto   failed
        skpdc
         goto   failed


        clrc
        clrdc
                
        ;;
        ;; andwf test:
        ;;
        clrf    temp
        movlw   1
        andwf   temp,f

        skpz
         goto   failed

        skpc
         skpndc
          goto  failed

        andwf   temp,w

        skpz
         goto   failed

        skpc
         skpndc
          goto  failed

        movlw   1
        movwf   temp
        andwf   temp,f

        skpnz
         goto   failed

        skpc
         skpndc
          goto  failed

        movlw   0xff
        andwf   temp,f          ;1 & 0xff should = 1

        skpnz
         goto   failed

        skpc
         skpndc
          goto  failed

        movlw   0xff            ;Now add 0xff to see if temp
        addwf   temp,f          ;really is 1. (should cause a carry)

        skpnz
         skpc
          goto  failed

        ;;
        ;; iorwf
        ;;

        movlw   0
        movwf   temp
        iorwf   temp,f
        skpz
         goto   failed

        movlw   1
        iorwf   temp,f
        skpnz
         goto   failed

        movlw   0xff
        addwf   temp,f
        skpnz
         skpc
          goto  failed


        ;;
        ;; rlf
        ;;

        clrdc
        clrf    temp
        clrc
        rlf     temp,w
        
        skpdc
         skpnc
          goto  failed

        skpz
         goto   failed

        setc
        rlf     temp,f

        skpdc
         skpnc
          goto  failed

        skpz
         goto   failed

        movlw   0xff
        addwf   temp,f
        skpnz
         skpc
          goto  failed

        rlf     temp,f
        skpnc
         goto   failed
        
        movlw   0x80
        movwf   temp
        rlf     temp,w

        skpc
         goto   failed
        movwf   temp
        addwf   temp,w
        skpz
         goto   failed

        ;;
        ;; rrf
        ;;

        clrdc
        clrf    temp
        clrc
        rrf     temp,w
        
        skpdc
         skpnc
          goto  failed

        skpz
         goto   failed

        setc
        rrf     temp,f

        skpdc
         skpnc
          goto  failed

        skpz
         goto   failed

        movlw   0x80
        addwf   temp,f
        skpnz
         skpc
          goto  failed

        rrf     temp,f
        skpnc
         goto   failed
        
        movlw   1
        movwf   temp
        rrf     temp,w

        skpc
         goto   failed
        movwf   temp
        addwf   temp,w
        skpz
         goto   failed


        ;; 
        ;; subwf test:
        ;;

        clrw
        clrf    temp
        
        subwf   temp,w          ; 0-0
        skpc
         goto   failed
        skpz
         goto   failed
        skpdc
         goto   failed

        movlw   1               ; 0-1
        subwf   temp,w
        
        skpnc
         goto   failed
        skpnz
         goto   failed
        skpndc
         goto   failed

        movlw   0x10            ; 0x10-0x10
        movwf   temp
        subwf   temp,w
        
        skpc
         goto   failed
        skpz
         goto   failed
        skpdc
         goto   failed

        movlw   0x88            ; 0x88-0x88
        movwf   temp
        subwf   temp,w
        
        skpc
         goto   failed
        skpz
         goto   failed
        skpdc
         goto   failed

        clrf    temp
        movlw   0x80            ; 0 - 0x80
        subwf   temp,f
        
        skpnc
         goto   failed
        skpnz
         goto   failed
        skpdc
         goto   failed

        movlw   0               ; 0x80 - 0
        subwf   temp,w

        skpc
         goto   failed
        skpnz
         goto   failed
        skpdc
         goto   failed

        movlw   1               ; 0x80 - 1
        subwf   temp,w

        skpc
         goto   failed
        skpnz
         goto   failed
        skpndc
         goto   failed
        

        clrc
        clrdc

        ;; 
        ;; xorwf
        ;;

        clrf    temp
        movlw   0xff
        xorwf   temp,f

        skpc
         skpndc
          goto  failed

        skpnz
         goto   failed

        xorwf   temp,f

        skpz
         goto   failed

        xorwf   temp,f
        incfsz  temp,f
         goto   failed

        ;;
        ;; swapf
        ;;

        movlw   0x0f
        movwf   temp
        swapf   temp,f
        xorwf   temp,f
        incfsz  temp,f
         goto   failed

        decf    temp1,f
        incfsz  temp1,w
         goto   d1

        ;; repeat the test with in the next bank of registers
        
        bsf     FSR,5
        goto    start1
        
d1:
        bcf     FSR,5
        
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP

                
        
        goto    test_pcl
        end
