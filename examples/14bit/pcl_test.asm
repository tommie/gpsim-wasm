	list	p=16c84

	;; The purpose of this program is to test gpsim's ability to do some funky pcl stuff.

include "p16c84.inc"
		
  cblock  0x20

	temp1
	temp2
	temp3
	adr_cnt
	data_cnt

	w_temp
	status_temp

  endc
	org 0

	nop
	nop
	nop

	;; Branch to address 0000 by using a 'clrf pcl' instruction
	
pcl_test1:
	
	btfsc	temp2,0
	 goto	pcl_test2

	bsf	temp2,0
        clrf    pcl

	;;
	;; Test indirect branching
	;;
	;; The following test verifies indirect branching using a look up table
	;; An index counter (temp1) is used branch into a table that will return
	;; the same value of the index (i.e. array[i] is equal to i). The test
	;; then verifies that the index and the returned value are the same.
	
pcl_test2:
	
	clrf	temp1		;Index counter


tt1:
	movf	temp1,w
	call	table_test	;Get an element from the table

	xorwf	temp1,w
	skpz			;If the returned value and the index are different
	 goto	pcl_error	;then that's an error.

	incf	temp1,f		;Keep on testing
	btfss	temp1,4		;until temp1==0x10
	 goto	tt1

	goto	pcl_test3

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
	retlw	0x0a
	retlw	0x0b
	retlw	0x0c
	retlw	0x0d
	retlw	0x0e
	retlw	0x0f


pcl_test3:

	;; Direct branching by moving a constant into PCL
	;; This is identical to a goto statement
	
	movlw	ly
	movwf	pcl		; == goto ly
lx
	movlw	pcl_test4
	movwf	pcl
ly
	movlw	lx
	movwf	pcl		; == goto lx


pcl_test4:

	;; This next section acts like a goto $ instruction. The PCL is decremented
	;; by the instruction but incremented by the program counter. So the net
	;; affect is that it remains unchanged! Furthermore, any instruction that
	;; writes to the PCL incurs an extra execution cycle. So if you examine the
	;; trace, you'll see that the decfsz pcl,f does in fact take two cycles to
	;; execute.
	
	decfsz	pcl,f
	 goto	pcl_error
	goto	pcl_error



pcl_error
	goto	pcl_error
	

	end