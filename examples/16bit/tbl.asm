	;; tbl.asm
	;;
	;; Test the tblrd and tblwt instructions

include "p18c242.inc"

  cblock  0
	version_hi,version_lo
	array_start
  endc


	org 0
start


	movlb	0		;Point to bank 0

;;; --------------------------------------------------------
;;; table read write example 1
;;;
;;; This example copies a string from program to data memory
;;;

;;; Initialize the table pointer to the start of the string
	movlw	LOW(str1)
	movwf	tabptrl,0
	movlw	HIGH(str1)
	movwf	tabptrh,0
	clrf	tabptru,0

;;; Initialize fsr0 to point to the start of the data array
	lfsr	0,array_start

copy_string:
	tblrd	*+		;Get one byte from program memory
	movff	tablat,indf0	;And copy it to data memory
	tstfsz	postinc0,0        ;If that byte was zero we're done
	 bra	copy_string     ;otherwise do more (note the post inc)

	

;;; --------------------------------------------------------
;;; table read/write example 2
;;;
;;; This example writes to program memory
;;;

;;; Initialize the table pointer to the
	movlw	LOW(version_string)
	movwf	tabptrl,0
	movlw	HIGH(version_string)
	movwf	tabptrh,0
	clrf	tabptru,0

write_version:
	tblrd	*+			;Read the version number
	movff	tablat,version_lo	;And copy it to data memory
	tblrd	*-			;high word 
	movff	tablat,version_hi

	infsnz	version_lo,f
	 incf	version_hi,f

	movff	version_lo,tablat
	tblwt	*+
	movff	version_hi,tablat
	tblwt	*-
	
	btfss	version_hi,1
	 bra	write_version
	

;;; --------------------------------------------------------
;;; table read/write example 3
;;;
;;; This example writes to program memory like the previous
;;; example. The internal table latch buffer is tested.
;;;

;;; Initialize the table pointer to the
	movlw	LOW(version_string)
	movwf	tabptrl,0
	movlw	HIGH(version_string)
	movwf	tabptrh,0
	clrf	tabptru,0


write_version2:
	tblrd	*			;Read the version number
	incf	tablat,f,0
	tblwt	*+
	
	tblrd	*			;high word
	skpz
	 incf	tablat,f,0
	tblwt	*-
	
	btfsc	version_hi,1
	 bra	write_version2
	

str1:
	db	"The 18Cxxx parts can read and write to tables",0

version_string:
	db	0,0
	end
