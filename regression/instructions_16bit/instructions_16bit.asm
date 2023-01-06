        ;; it.asm
        ;;
        ;; The purpose of this program is to test how well gpsim can simulate
        ;; a 16bit-core pic (like the 18cxxx family not the 17c family.
        ;; Nothing useful is performed - this program is only used to
        ;; debug gpsim.

	list    p=18f452                ; list directive to define processor
	include <p18f452.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA
temp            RES     1
temp1           RES     1
temp2           RES     1
failures        RES     1


  GLOBAL done, temp

ifndef __IDLOCS_START
; The following warning can be ignored the line is used where gpasm < 1.4.0
; Warning[205] Found directive in column 1: "IDLOCS"

IDLOCS CODE
  db      "ID"
else
 IDLOCS "ID"
endif

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

        setc                    ;set the Carry flag
        skpc                    ;and verify that it happened
         bra    failed1
        bnc     failed1

        clrc                    ;Now try clearing the carry
        skpnc
         bra    failed1
        bc      failed1

        setz                    ;set the Zero flag
        skpz                    ;and verify that it happened
         bra    failed1
        bnz     failed1

        clrz                    ;Now try clearing it
        skpnz
         bra    failed1
        bz      failed1
        
        setdc                   ;set the Digit Carry flag
        skpdc                   ;and verify that it happened
         bra    failed

        clrdc                   ;Now try clearing it
        skpndc
         bra    failed

        setov                   ;set the Over Flow flag
        skpov                   ;and verify that it happened
         bra    failed
        bnov    failed1

        clrov                   ;Now try clearing it
        skpnov
         bra    failed
        bov     failed1

        setn                    ;set the negative flag
        skpn                    ;and verify that it happened
         bra    failed
        bnn     failed1

        clrn                    ;Now try clearing it
        skpnn
         bra    failed
        bn      failed1

        movlw   1
        movwf   temp
        movf    temp,f
        bz      failed1

        movlw   0
        movwf   temp
        movf    temp,f
        bnz     failed1
        
        clrf    temp            ;Clear a register
        bnz     failed1         ;and verify that the Z bit is set

        movf    temp,w          ;Read the register that was just
        bnz     failed1         ;and verify that the Z bit is set

        incf    temp,w          ;Now this should cause the Z bit to
        bz      failed1         ;get cleared.

        movlw   0xff
        movwf   temp
        incf    temp,F

        bnz      failed1

        ;;
        ;; incfsz
        ;;

        movlw   0xff
        movwf   temp
        movf    temp,f          ;Should clear Z
        incfsz  temp,w
         bra    failed

        bz      failed1         ;incfsz shouldn't affect Z

        incfsz  temp,f          ;temp should now be zero
         bra    failed

        incfsz  temp,w          ;the following inst. shouldn't be skipped
         skpnz                  ;Z should be clear from above.
          bra   failed
                

        ;; 
        ;; addwf test:
        ;;
        ;; The purpose of this test is to a) verify that the addwf instruction
        ;; adds correctly and b) that the appropriate status register bits are
        ;; set correctly.

        clrw
        clrf    temp
        
        addwf   temp,w          ; 0+0
        bc      failed1
        bnz     failed1
        skpndc
         bra    failed

        movlw   1               ; 0+1
        addwf   temp,w

        bc      failed1
        bz      failed1
        bn      failed1
        bov     failed1
        skpndc
         bra    failed

        movlw   8               ; 8+8 , test dc
        movwf   temp
        addwf   temp,w
        
        bc      failed1
        bz      failed1
        skpdc
         bra    failed
        bn      failed1
        bov     failed1

        movlw   0x88            ; 0x88+0x88
        movwf   temp
        addwf   temp,w

        bnc     failed1
        bz      failed1        
        skpdc
         bra    failed
        bn      failed1
        bnov    failed1

        movlw   0x80            ; 0x80+0x80
        movwf   temp
        addwf   temp,w

        bnc     failed1
        bnz      failed1
        skpndc
         bra    failed
        bn      failed1
        bnov    failed1

        clrw
        addwf   temp,w          ; 0x80+0
        
        skpc
         skpnz
          bra   failed
        skpndc
         bra    failed
        skpnn
         skpov
          bra   failed


        clrf    temp
        addwf   temp,w          ; 0+0x80
        
        skpc
         skpnz
          bra   failed
        skpndc
         bra    failed
        skpnn
         skpnov
          bra   failed

        movlw   1
        movwf   temp
        movlw   0xff
        addwf   temp,w

        skpc
         bra    failed
        skpz
         bra    failed
        skpdc
         bra    failed


        clrc
        clrdc
                
        ;;
        ;; andwf test:
        ;;
        clrf    temp
        movlw   1
        andwf   temp,f

        skpz
         bra    failed

        skpc
         skpndc
          bra  failed

        andwf   temp,w

        skpz
         bra    failed

        skpc
         skpndc
          bra  failed

        movlw   1
        movwf   temp
        andwf   temp,f

        skpnz
         bra    failed

        skpc
         skpndc
          bra  failed

        movlw   0xff
        andwf   temp,f          ;1 & 0xff should = 1

        skpnz
         bra    failed

        skpc
         skpndc
          bra  failed

        movlw   0xff            ;Now add 0xff to see if temp
        addwf   temp,f          ;really is 1. (should cause a carry)

        skpnz
         skpc
          bra   failed

        ;;
        ;; decf
        ;;

        clrf    temp
        decf    temp,F	        ;0==>0xff
        skpc
         skpnz
          bra   failed

        decf    temp,F          ;0xff==>0xfe
        skpnc
         skpnz
          bra   failed

        incf    temp,F
        skpc
         skpnz
          bra   failed

        incf    temp,F
        skpnc
         skpz
          bra   failed

        ;;
        ;; iorwf
        ;;

        movlw   0
        movwf   temp
        iorwf   temp,f
        skpz
         bra    failed

        movlw   1
        iorwf   temp,f
        skpnz
         bra    failed

        movlw   0xff
        addwf   temp,f
        skpnz
         skpc
          bra   failed


        ;;
        ;; rlcf
        ;;

        clrdc
        clrf    temp
        clrc
        rlcf    temp,w
        
        skpdc
         skpnc
          bra   failed

        skpz
         bra    failed

        setc
        rlcf    temp,f

        skpdc
         skpnc
          bra   failed

        skpnz
         bra    failed

        movlw   0xff
        addwf   temp,f
        skpnz
         skpc
          bra   failed

        rlcf    temp,f
        skpnc
         bra    failed
        
        movlw   0x80
        movwf   temp
        rlcf    temp,w

        skpc
         bra    failed
        movwf   temp
        addwf   temp,w
        skpz
         bra    failed

        ;;
        ;; rrcf
        ;;

        clrdc
        clrf    temp
        clrc
        rrcf    temp,w
        
        skpdc
         skpnc
          bra   failed

        skpz
         bra    failed

        setc
        rrcf    temp,f

        skpdc
         skpnc
          bra   failed

        skpnz
         bra    failed

        movlw   0x80
        addwf   temp,f
        skpnz
         skpc
          bra   failed

        rrcf    temp,f
        skpnc
         bra    failed
        
        movlw   1
        movwf   temp
        rrcf    temp,w

        skpc
         bra    failed
        movwf   temp
        addwf   temp,w
        skpz
         bra    failed


        ;; 
        ;; subwf test:
        ;;

        clrf    WREG
        clrf    temp
        
        subwf   temp,w          ; 0-0
        skpc
         bra    failed
        skpz
         bra    failed
        skpdc
         bra    failed

        movlw   1               ; 0-1
        subwf   temp,w
        
        skpnc
         bra    failed
        skpnz
         bra    failed
        skpndc
         bra    failed

        movlw   0x10            ; 0x10-0x10
        movwf   temp
        subwf   temp,w
        
        skpc
         bra    failed
        skpz
         bra    failed
        skpdc
         bra    failed

        movlw   0x88            ; 0x88-0x88
        movwf   temp
        subwf   temp,w
        
        skpc
         bra    failed
        skpz
         bra    failed
        skpdc
         bra    failed

        clrf    temp
        movlw   0x80            ; 0 - 0x80
        subwf   temp,f
        
        skpnc
         bra    failed
        skpnz
         bra    failed
        skpdc
         bra    failed

        movlw   0               ; 0x80 - 0
        subwf   temp,w

        skpc
         bra    failed
        skpnz
         bra    failed
        skpdc
         bra    failed

        movlw   1               ; 0x80 - 1
        subwf   temp,w

        skpc
         bra    failed
        skpnz
         bra    failed
        skpndc
         bra    failed
        

        movlw   0x05
        movwf   temp
                                                                                
        ;;  positive - positive => positive  no overflow
        movlw   0x03
        subwf   temp,W
        bov     failed2
                                                                                
        ;; positive - positive => negative, no overflow
                                                                                
        movlw   0x07
        subwf   temp,W
        bov     failed2
                                                                                
        ;; positive - negative => positive, no overflow.
        movlw   0xff
        subwf   temp,W
        bov     failed2

       ;; positive - negative => negative, overflow.
        movlw   0x81
        subwf   temp,W
        bnov    failed2

        movlw   -0x05
        movwf   temp

        ;;  negative - positive => positive,  overflow
        movlw   0x7e
        subwf   temp,W
        bnov    failed2

        ;; negative - positive => negative, no overflow

        movlw   0x02
        subwf   temp,W
        bov     failed2

        ;; negative - negative => positive, no overflow.
        movlw   -0x07
        subwf   temp,W
        bov     failed2

        ;; negative - negative => negative, overflow.
        movlw   -0x02
        subwf   temp,W
        bov     failed2
	
	;; test subwfc
	movlw	0xf0
	subwfb  temp,W ;; temp=fb - W=f0 - B = W=a
   .assert "W == 0xa, '*** FAILED subwfb fb - f0 - 1 = a'"
	nop
	subwfb  temp,F ;; temp=fb - W=a -0 =  temp=f1
   .assert "temp == 0xf1, '*** FAILED subwfb fb - a - 0 = f1'"
	nop

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
          bra   failed
        bz      failed

        xorwf   temp,f

        bnz     failed

        xorwf   temp,f
        incfsz  temp,f
         bra    failed

        ;;
        ;; swapf
        ;;

        movlw   0x0f
        movwf   temp
        swapf   temp,f
        xorwf   temp,f
        incfsz  temp,f
         bra    failed

        ;;
        ;; daw
        ;;

        ; trivial case : 13+25=38, DAW makes no adjustment
        movlw   0x13
        addlw   0x25
        daw
        bc      failed
        xorlw   0x38
        bnz     failed
        ; simple case : 28+39=67, DAW adjusts lower nibble only
        movlw   0x28
        addlw   0x39
        daw
        bc      failed
        xorlw   0x67
        bnz     failed
        ; carry case : 84+39=123, DAW adjusts both nibbles and sets carry
        movlw   0x84
        addlw   0x39
        daw
        bnc     failed
        xorlw   0x23
        bnz     failed
        ; corner case : 96+84=180, DAW adjusts both nibbles but must leave existing carry set
        movlw   0x96
        addlw   0x84
        daw
        bnc     failed      ; this used to fail
        xorlw   0x80
        bnz     failed
 if 0
        ; subtract case : 47-19=28, DAW adjusts lower nibble only
        ; Not sure the real PIC works here - the data sheet says "addition"
        movlw   0x19
        sublw   0x47
        daw
        bnc     failed
        xorlw   0x28
        bnz     failed
 endif

        ;;

        clrf    temp
        negf    temp
        bnz     failed
        incf    temp,f
        negf    temp
        bz      failed
        incfsz  temp,f
         bra    failed

        goto    BranchTest
failed2:	; interim label for tests above
        bra     failed
BranchTest:
        call    CallTest
        bra     MultiWordInstructions
CallTest
        return

MultiWordInstructions:
	movlw	0xa5
	movwf	temp1
	movff	WREG,temp2
	xorwf	temp2,W
	bnz	failed

        ;; LFSR
        LFSR	0,temp1
	MOVF	INDF0,W
	xorwf	INDF0,W
	bnz	failed

TableRead:
   ; get a pointer to the start of the table area
	clrf	TBLPTRU
	movlw	LOW(TableDATA)
	movwf	TBLPTRL
	movlw	HIGH(TableDATA)
	movwf	TBLPTRH

   ; read the table and test the autoinc and autodec.

	tblrd	*+
	movlw	0x11
	rcall	TestTablat
	tblrd   *
	movlw	0x22
	rcall	TestTablat
	tblrd	*+
	movlw	0x22
	rcall	TestTablat
	tblrd	*-
	movlw	0x33
	rcall	TestTablat
	tblrd   *
	movlw	0x22
	rcall	TestTablat
	tblrd	+*
	movlw	0x33
	rcall	TestTablat

	clrf	TBLPTRH
	setf	TBLPTRL
	tblrd	*+
	movlw	1
	cpfseq	TBLPTRH
  .assert  "'FAILED tblrd increment over boundary'"
	 bra 	failed

	tblrd	*-
	movlw	0
	cpfseq	TBLPTRH
  .assert  "'FAILED tblrd decrement over boundary'"
	 bra 	failed

	clrf	TBLPTRL
	tblrd	*-
	movlw	0xFF
	cpfseq	TBLPTRH
  .assert  "'FAILED tblrd decrement over boundary'"
	 bra 	failed

	tblrd	+*
	movlw	0
	cpfseq	TBLPTRH
  .assert  "'FAILED tblrd increment over boundary'"
	 bra 	failed

	; Now a quick check of an ID location
        movlw	0x20
        movwf	TBLPTRU
        clrf	TBLPTRH
        clrf	TBLPTRL
        tblrd	*+
	movlw	0x49
	rcall	TestTablat
	tblrd   *
	movlw	0x44
	rcall	TestTablat
	bra	TableReadEnd
	
TableDATA:
	db	0x11,0x22,0x33,0x44

TestTablat:	
	cpfseq	TABLAT
  .assert  "'FAILED tblrd indexing / value'"
	 bra	failed
	return
TableReadEnd:	

   ;
   ; Now test 16-bit core SFRs (which should be done in an SFR test but there isn't one)
   ;

        movlw   0xFF
        movwf   PCLATH
        
   .assert "pclath == 0xFF, '*** FAILED 16-bit PCLATH masking'"
        nop
        clrf    PCLATH
   .assert "pclath == 0, '*** FAILED 16-bit PCLATH clear'"

	call   pcl_check

	movlb   5
   .assert "bsr == 5, '*** FAILED movlb'"
	nop


done:
  .assert  "'*** PASSED 16bit-core instruction test'"
        bra     $

failed:
        movlw   1
        movwf   failures
  .assert  "'*** FAILED 16bit-core instruction test'"
        bra     done


  if 0  

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
        addwf   PCL,f
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
        
        btfsc   temp2,0
         goto   test_pcl3
        movlw   0x20
        movwf   FSR0L
        clrf    FSR0H
        
next    clrf    INDF0
        incf    FSR0L,1
        btfss   FSR0L,4
        goto    next

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
        
        goto    $

  endif



;--------------------------------------------------------------------------
; PCL testing

        org     0x04FE
pcl_check:
    movf   PCL,w       ; Read of PCL sets PCLATH _after_ incrementing PC
    movwf  temp1
    movf   PCLATH,w
    movwf  temp2
  .assert "temp1 == 0, '*** FAILED read PCL after increment'"
  .assert "temp2 == 5, '*** FAILED read PCL should set PCLATH after PC increments'"

    clrf   PCLATH
    addwf  PCL,w       ; Also a read, so sets PCLATH again
    movf   PCLATH,w
    movwf  temp2
  .assert "temp2 == 5, '*** FAILED addwf PCL,w should set PCLATH'"

    movlw  high(pcl_check_2)
    movwf  PCLATH
    movlw  branch_dist
    addwf  PCL,f       ; This is a read-modify-write so DOES NOT set PCLATH
_pc1Exit:

    org 0x0480
pcl_check_2:
    movf   PCLATH,w
    movwf  temp2
  .assert "temp2 == 4, '*** FAILED addwf PCL,f should not change PCLATH'"
    return

    org 0x0580
pcl_check_3:
  .assert  "'*** FAILED PCLATH needs to be set before PCL write'"
    movf   PCLATH,w
    movwf  temp2
    return

branch_dist equ pcl_check_2-_pc1Exit


        end
