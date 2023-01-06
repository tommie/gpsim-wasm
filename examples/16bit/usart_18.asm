	;; The purpose of this program is to test gpsim's ability to simulate
	;; USART in the 18cxxx core.

	
include "p18c242.inc"
		
  cblock  0

	temp1
	temp2

	w_temp
	status_temp

  endc
	org 0

	goto	start

	org	4

start


	clrf	txsta,0

	;; Wait for the tsr (transmit shift register) to be empty.
	btfss	txsta,trmt,0
	 bra	$-1
	
	bsf	txsta,brgh,0
	bsf	rcsta,spen,0
	
	clrf	spbrg,0

	bsf	txsta,txen,0

	movlw	0xaa
	movwf	txreg,0

	btfss	pir1,txif,0	;Did the interrupt flag get set?
	 bra	$-1

	btfss	txsta,trmt,0	;Wait 'til through transmitting
	 bra	$-1

	bcf	txsta,txen,0
	bcf	pir1,txif,0
	bcf	pir1,rcif,0

rx_loop:

	bsf	rcsta,cren,0
	btfss	pir1,rcif,0
	 bra	$-1

	bcf	pir1,rcif,0

	bra	rx_loop
	
	bra $
	
	end



