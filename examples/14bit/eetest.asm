	list	p=16c84
  __config _wdt_off


include "p16c84.inc"
	
  cblock	0x0c
	adr_cnt
	data_cnt
	w_temp
	status_temp
  endc


	org	0

	goto    start

	org	4
	;; Interrupt
	;; 
	movwf	w_temp
	swapf	status,w
	movwf	status_temp

	bsf	status,rp0

	btfss	(eecon1 & 0x7f),eeif
	 goto	check

;;; eeprom has interrupted
	bcf	(eecon1 & 0x7f),eeif

check:
	swapf	status_temp,w
	movwf	status
	swapf	w_temp,f
	swapf	w_temp,w
	retfie

start:	
	clrf	adr_cnt
	clrf	data_cnt
	incf    data_cnt,f
	bsf	intcon,eeie

l1:	

	bcf	status,rp0
	movf	adr_cnt,w
	movwf	eeadr
	movf	data_cnt,W
	movwf	eedata

	bsf	status,rp0
	bcf	intcon,gie	;Disable interrupts while enabling write

	bsf	(eecon1 & 0x7f),wren	;Enable eeprom writes

	movlw	0x55		;Magic sequence to enable eeprom write
	movwf	(eecon2 & 0x7f)
	movlw	0xaa
	movwf	(eecon2 & 0x7f)

	bsf	(eecon1 & 0x7f),wr	;Begin eeprom write

	bsf	intcon,gie	;Re-enable interrupts
	
	btfsc	(eecon1 & 0x7f),wr
	 goto	$-1

	bcf	status,rp0
	
	incf	adr_cnt,w
	andlw	0x3f
	movwf   adr_cnt

	skpz
	 goto	l1

	incfsz	data_cnt,F
	goto	l1

	goto	l1		;A place to set a break point (for timing how long
				;it takes to fill the eeprom 256 times

	org	0x2100
	de	"Linux is cool!",0
	de	0xaa,0x55,0xf0,0x0f
	de	'g','p','s','i','m'
	end
