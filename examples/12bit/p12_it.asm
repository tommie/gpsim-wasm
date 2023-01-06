	;; p12_it.asm
	;;
	;; The purpose of this program is to test how well gpsim can simulate
	;; the instruction set of the 12bit core pics. Nothing useful is performed
	;;  - this program is only used to debug gpsim.
	
	list	p=12c508
;; include "p16c84.inc"
#define	indf	0
#define	pcl	2
#define status	3
#define fsr	4




#define f	1
#define w	0
	
  cblock  0x0c

	temp,temp1,temp2
  endc
	
	org 0

	goto	test_pcl
;;	addlw	0xaa

	addwf	temp,w
	andlw	0xaa
	andwf	temp,w
	bcf	temp,5
	bsf	temp,5
	btfsc	temp,5
	btfss	temp,5
	call	routine
	clrf	temp
	clrw
	clrwdt
	comf	temp,w
	decf	temp,w
	decfsz	temp,w

	goto	l1
	nop
	nop
	nop
l1:	
	incf	temp,w
	incfsz	temp,w
	iorlw	0xaa
	iorwf	temp,w
	movlw	0xaa
	movf	temp,w
	movwf	temp
	nop
	option


	goto	r1
		
;	retfie
	retlw   0xaa
routine
	retlw	0
r1:

	rlf	temp,w
	rrf	temp,w
	sleep
;	sublw	0xaa
	subwf	temp,w
	swapf	temp,w
	tris	5
	xorlw	0xaa
	xorwf	temp,w
	

	;;
	;; Test indirect branching
	;;
test_pcl:
	
	clrf	temp
	clrf	temp1

tt1:
	movf	temp,w
	call	table_test

	xorwf	temp,w
	skpz
	 bsf	temp1,0

	incf	temp,f
	btfss	temp,4
	 goto	tt1

	goto	test_pcl2
table_test:
	addwf	pcl,f
table:
	retlw	0
	retlw	1
	retlw	2
	retlw	3
	retlw	4
	retlw	5
	retlw	6
	retlw	7
	retlw	8
	retlw	9
	retlw	10
	retlw	11
	retlw	12
	retlw	13
	retlw	14
	retlw	15

test_pcl2:
	
	btfsc	temp2,0
	 goto	test_pcl3
        movlw   0x20
        movwf   fsr
next    clrf    indf
        incf    fsr,1
        btfss   fsr,4
        goto    next

	bsf	temp2,0
        clrf    pcl

test_pcl3:

	movlw	ly
	movwf	pcl		; == goto ly
lx
	goto	test_pcl4
ly
	movlw	lx
	movwf	pcl

test_pcl4:
	
	goto	$
	end