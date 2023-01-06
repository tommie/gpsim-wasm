        ;; it.asm
        ;;
        ;; The purpose of this program is to test how well gpsim can simulate
        ;; the 18f extended instructions.


	list    p=18f4321                ; list directive to define processor
	include <p18f4321.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

.command macro x
  .direct "C", x
  endm

	radix dec

 	CONFIG   XINST=ON

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA
temp            RES     1
temp1           RES     1
temp2           RES     1
failures        RES     1


dataStack	RES	32
dataStackEnd    RES     1

  GLOBAL done
  GLOBAL temp1
  GLOBAL dataStack
  GLOBAL dataStackEnd

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE

    .sim "symbol oldcycle = 0"

	bra	start
  ;------------------------------------------------------------
  ; function vectors
  ; The function vectors are branch instructions to functions
  ; that reside at addresses for which PC>256. They're used to
  ; make indirect calls with the extended CALLW instruction.

vf1	bra	f1
vf2	bra	f2
vf3	bra	f3
vf4	bra	f4
vf5	bra	f5

start:  

        clrf    temp1           ;Assume clrf works...
                                ;
	lfsr    0,0
	lfsr    1,0
	lfsr    2,0x1ff
	clrf	BSR

	addfsr	0,15
   .assert "(fsr0l==15) && (fsr0h==0)"
	nop
	addfsr	1,15
   .assert "(fsr1l==15) && (fsr1h==0)"
	nop
	addfsr	2,15	; 0x1ff + 15 = 0x20e
   .assert "(fsr2l==0x0e) && (fsr2h==2)"
	nop


	subfsr	0,10
   .assert "(fsr0l==5) && (fsr0h==0)"
	nop
	subfsr	1,10
   .assert "(fsr1l==5) && (fsr1h==0)"
	nop
	subfsr	2,15	; 0x20e - 15 = 0x1ff
   .assert "(fsr2l==0xff) && (fsr2h==1)"
	nop

	lfsr    2,0x80
	movlw	0x10
	movwf	temp1
	addwf   [1],W
   .assert "W == 0x20"
	nop
	addwfc [1],W
	bsf	[1],0
   .assert "temp1 == 0x11"
	nop
	clrf	[1]
   .assert "temp1==0"
	nop
	setf	[1]
   .assert "temp1 == 0xff"
	nop

  	call 	test_addulnk
    .assert "(fsr2l==0x81) && (fsr2h==0)"
	nop
	movlw	0xf0
	andwf	[0],W
    .assert "W==0xf0"
	nop
	bcf	[0],0
    .assert "temp1==0xfe"
	nop
	btfsc	[0],0
	goto	failed
	btfss	[0],1
	goto	failed
	btg	[0],0
    .assert "temp1==0xff"
	nop
	comf	[0],F
    .assert "temp1==0x00"
	nop
	movlw	0
	cpfseq  [0]
	goto	failed
	nop
	decf	[0],F
    .assert "temp1==0xff"
	nop
	cpfsgt	[0]
	goto	failed
	cpfslt	[0]
	goto	$+8
	goto	failed
	decfsz	[0],W
	goto	$+8
	goto	failed
	dcfsnz	[0],W
	goto	failed
	incfsz	[0],F
	goto	failed
	infsnz	[0],F
	goto	failed
	iorwf	[0],W
    .assert "W==0xff"
	nop
	movf	[0],W
    .assert "W==0x01"
	nop
	movlw	2
	movwf	[0]
	mulwf	[0]
    .assert "(prodl==4) && (prodh==0)"
	nop
	negf	[0]
    .assert "temp1==0xfe"
	nop
	rlcf	[0],W
    .assert "W==0xfc"
	nop
	rlncf	[0],W
    .assert "W==0xfd"
	nop
	rrcf	[0],W
    .assert "W==0xff"
	nop
	rrncf	[0],W
    .assert "W==0x7f"
	nop
	subfwb	[0],W
    .assert "W==0x80"
	nop
	subwfb	[0],W
    .assert "W==0x7d"
	nop
	swapf	[0],F
    .assert "temp1==0xef"
	nop
	tstfsz	[0]
	goto	$+8
	goto	failed
	xorwf	[0],W
    .assert "W==0x92"
	nop
	
	

   ;; Also test that addfsr followed immediately by and INDF access works
        clrf    dataStackEnd
        movlw   0x55
        lfsr    1,dataStack
        addfsr  1,32
        movwf   POSTDEC1
   .assert "dataStackEnd == 0x55"
        nop

    ;;  Test postinc and addfsr interaction
    lfsr    1,0x80
    movlw   0xBB
    movwf   POSTINC1
    addfsr  1,1
    .assert "(fsr1h == 0x00) && (fsr1l == 0x82)"

    ;;  Test postinc and subfsr interaction
    lfsr    1,0x80
    movlw   0xBB
    movwf   POSTINC1
    subfsr  1,1
    .assert "(fsr1h == 0x00) && (fsr1l == 0x80)"


   ;; CALLW test
   ;; This test makes 5 indirect calls using the callw instruction.
   ;; There is a function vector table in low memory that redirects
   ;; calls to high memory. The reason for this table is to allow the
   ;; destination address for the CALLW instruction to fit entirely
   ;; in the WREG

	clrf	PCLATU
	clrf	PCLATH
	movlw	5
	movwf	temp1		;temp1 is used as the index into 
	nop			;function vector table.

callw_loop:
	rlncf	temp1,W		;program memory indexes through even addresses
	addlw	vf1-2		;offset to the start of the function vectors.
	callw			;Make the indirect call
	decfsz  temp1,F		;next vector
	 bra	callw_loop


    ;; Test interaction of postdec and movsf
        lfsr	2,dataStack+1
        movlw   0xAA
        movwf   POSTDEC2
        movsf   [1],temp1

    .assert "temp1 == 0xAA"
        nop

    ;;
	lfsr	2, dataStack+10
	clrf	dataStack
	pushl	0x42
   .assert "(fsr2l == ((&dataStack)+10-1))"

	nop

	movsf	[1],WREG
   .assert "W == 0x42"

        nop

    ;;   Test for the mixed-mode-repeat condition c.f. bug #3309120
        pushl   0x01
        pushl   0x02
        pushl   0x03
        movlw   3
        movff   PLUSW2,POSTDEC2
        movff   PLUSW2,POSTDEC2
        pushl   0x04
        pushl   0x05
        
   .assert "(fsr2l == ((&dataStack)+10-8))"
        nop

	movsf	[1],WREG
   .assert "W == 0x05"
	nop

	movss	[1],[3]

   .assert "(*(fsr2l+1))==(*(fsr2l+3))"
	nop

;	read_devid to test tblrd instruction
        movlw   0x3f
        movwf   TBLPTRU
        movlw   0xFF
        movwf   TBLPTRH
        movlw   0xFE              ;
        movwf   TBLPTRL
        movlw   (1<<CFGS)
;        movwf   EECON1

    .command "oldcycle = cycles"
	nop
        tblrd*+             ; read into TABLAT and increment
        tblrd*+             ; read into TABLAT and increment
    .assert "(cycles - oldcycle) == 5, '*** FAILED 16bit-core tblrd cycles'"
	nop
    .assert "tablat == 0x21, '*** FAILED 16bit-core device id'"
	nop
        bra     done

;------------------------------------------------------------
; CALLW functions - here are 5 functions that are called indirectly
; by the callw instruction. All they do is return. However, the assertion
; verifies that the index (temp1) used to calculate the address of the
; indirect call is correct.
f1:
   .assert "temp1==1"
	return
f2:
   .assert "temp1==2"
	return
f3:
   .assert "temp1==3"
	return
f4:
   .assert "temp1==4"
	return
f5:
   .assert "temp1==5"
	return


failed1:        ; a relatively local label
        bra     failed

        ;; Perform some basic tests on some important instructions
done:
  .assert  "'*** PASSED 16bit-core extended instruction test'"
        bra     $

failed:
        movlw   1
        movwf   failures
  .assert  "'*** FAILED 16bit-core extended instruction test'"
        bra     done

test_addulnk:
	addulnk 1	; should do return


        end
