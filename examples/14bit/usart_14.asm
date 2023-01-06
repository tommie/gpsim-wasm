	;; The purpose of this program is to test gpsim's ability to simulate
	;; USART in the 14bit core.


	list	p=16c74
  __config _WDT_OFF


include <p16c74.inc>
include <coff.inc>

.command macro x
  .direct "C", x
  endm


#define	RX_BUF_SIZE	0x10

  cblock  0x20

	temp1
	temp2

	w_temp
	status_temp
	fsr_temp

	tx_ptr
	rx_ptr
	rx_buffer : RX_BUF_SIZE


  endc
	org 0

	goto	start

	org	4
	;; Interrupt
	;; 
	clrf	STATUS
	movwf	w_temp
	swapf	STATUS,w
	movwf	status_temp

	bcf	STATUS,RP0

	btfsc	INTCON,PEIE
	 btfss	PIR1,RCIF
	  goto	int_done

;;;
	movf	FSR,w
	movwf	fsr_temp
	incf	rx_ptr,w
	andlw	0x0f
	movwf	rx_ptr
	addlw	rx_buffer
	movwf	FSR
    .assert "rcreg != 0x44, 'PASSED'"
	nop
	movf	RCREG,w
	movwf	INDF
	movf	fsr_temp,w
	movwf	FSR
	
int_done:	
	swapf	status_temp,w
	movwf	STATUS
	swapf	w_temp,f
	swapf	w_temp,w
	retfie

	;; ----------------------------------------------------
	;;
	;;            start
	;; 
start	
		
	
	;; USART Initialization
	;;
	;; Turn on the high baud rate (BRGH), disable the transmitter,
	;; disable synchronous mode.
	;;
	
	clrf	STATUS

	bsf	STATUS,RP0
	
	movlw	(1<<BRGH)

	movwf	TXSTA
	movlw   0x81             ;9600 baud.
        movwf   SPBRG ^0x80


				; Make rc6 and rc7 inputs. These are the
	bsf	TRISC,6
	bsf	TRISC,7
	
	bcf	STATUS,RP0

	movlw	0xff
	movwf	tx_ptr
			
	;; Turn on the serial port
	movlw	(1<<SPEN) | (1<<CREN)
	movwf	RCSTA

	movf	RCREG,w
	bsf	INTCON,GIE
	bsf	INTCON,PEIE
	
	;; Enable the transmitter
	bsf	STATUS,RP0
	bsf	TXSTA,TXEN
	bsf	PIE1,RCIE
	bcf	STATUS,RP0

tx_loop:	
	call	tx_message
	BANKSEL TXREG
	movwf	TXREG


	bcf	PIR1,TXIF

	btfss	PIR1,TXIF
	 goto	$-1


	goto	tx_loop

tx_message
	incf	tx_ptr,w
;	andlw	0x0f
	movwf	tx_ptr
	addlw	TX_TABLE
	skpnc
	 incf	PCLATH,f
	movwf	PCL
TX_TABLE
	dt	"0123456789ABCDEF",0

	
	movlw	0xaa
	movwf	TXREG

	btfss	PIR1,TXIF	;Did the interrupt flag get set?
	 goto	$-1

	bsf	STATUS,RP0
	btfss	TXSTA,TRMT	;Wait 'til through transmitting
	 goto	$-1
	bcf	STATUS,RP0

;;; 	bcf	TXSTA,TXEN
	bcf	PIR1,TXIF
	bcf	PIR1,RCIF


rx_loop:

	bsf	RCSTA,CREN
	btfss	PIR1,RCIF
	 goto	$-1

	bcf	PIR1,RCIF

	goto	rx_loop

	goto $
	
	end
