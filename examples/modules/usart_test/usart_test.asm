   ;;  USART test 
   ;;
   ;;  The purpose of this program is to verify that gpsim's
   ;; USART module functions properly.
   ;;
   ;;
   ;;

	list	p=16f877
	include <p16f877.inc>
	include <coff.inc>
        __CONFIG _WDT_OFF


        errorlevel -302 
	radix dec
;----------------------------------------------------------------------
; RAM Declarations


;
INT_VAR        UDATA   0x20
w_temp          RES     1
status_temp     RES     1
pclath_temp     RES     1
fsr_temp	RES	1

INT_VAR1       UDATA   0xA0
w_temp1          RES     1	;Alias for w_temp at address 0x20


GPR_DAT        UDATA

#define	RX_BUF_SIZE	0x10

temp1		RES	1
temp2		RES	1
temp3		RES	1

tx_ptr		RES	1
rx_ptr		RES	1
rx_buffer	RES	RX_BUF_SIZE


;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlw  high  start               ; load upper byte of 'start' label
        movwf  PCLATH                    ; initialize PCLATH
        goto   start                     ; go to beginning of program



;------------------------------------------------------------------------
;
;  Interrupt Vector
;
;------------------------------------------------------------------------

INT_VECTOR   CODE    0x004               ; interrupt vector location

	movwf	w_temp
	swapf	STATUS,w
	clrf	STATUS
	movwf	status_temp
	movf	PCLATH,w
	movwf	pclath_temp
	clrf	PCLATH

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
	movf	RCREG,w
	movwf	INDF
	movf	fsr_temp,w
	movwf	FSR
	
int_done:
	clrf	STATUS
	movf	pclath_temp,w
	movwf	PCLATH
	swapf	status_temp,w
	movwf	STATUS
	swapf	w_temp,f
	swapf	w_temp,w
	retfie


;; ----------------------------------------------------
;;
;;            start
;;

MAIN    CODE
start	

   .sim ".frequency=20e6"
   .sim "module library libgpsim_modules"
   .sim "module load usart U1"
 ;  .sim "U1.xpos = 250.0"
;   .sim "U1.ypos = 80.0"

   .sim "node PIC_tx"
   .sim "node PIC_rx"

   ;; Tie the USART module to the PIC
   .sim "attach PIC_tx portc6 U1.RXPIN"
   .sim "attach PIC_rx portc7 U1.TXPIN"

   ;; Set the USART module's Baud Rate

   .sim "U1.txbaud = 9600"
   .sim "U1.rxbaud = 9600"
   .sim "U1.xpos = 216.0"
   .sim "U1.ypos = 48.0"

	;; USART Initialization
	;;
	;; Turn on the high baud rate (BRGH), disable the transmitter,
	;; disable synchronous mode.
	;;
	
	clrf	STATUS

	bsf	PORTC,6         ;Make sure the TX line drives high when 
                                ;it is programmed as an output.

	bsf	STATUS,RP0


	bsf	TRISC,7		;RX is an input
	bcf	TRISC,6		;TX is an output

	;; CSRC - clock source is a don't care
	;; TX9  - 0 8-bit data
	;; TXEN - 0 don't enable the transmitter.
	;; SYNC - 0 Asynchronous
	;; BRGH - 1 Select high baud rate divisor
	;; TRMT - x read only
	;; TX9D - 0 not used
	
	movlw	(1<<BRGH)
	movwf	TXSTA ^ 0x80

	movlw   129		;9600 baud.
	movwf   SPBRG ^0x80

	;;clrf	SPBRG		;Highest possible baud rate.

	bcf	STATUS,RP0

	clrf	tx_ptr
			
	;; Turn on the serial port
	movlw	(1<<SPEN) | (1<<CREN)
	movwf	RCSTA

	movf	RCREG,w          ;Clear RCIF
	bsf	INTCON,GIE
	bsf	INTCON,PEIE

	;; Delay for a moment to allow the I/O lines to settle
;	clrf	temp2
;	call	delay
	
	movf	RCREG,w          ;Clear RCIF
	movf	RCREG,w          ;Clear RCIF

	;; Enable the transmitter
	bsf	STATUS,RP0
	bsf	TXSTA^0x80,TXEN
	bsf	PIE1^0x80,RCIE
	bcf	STATUS,RP0

	;; Now Transmit some data and verify that it is transmitted correctly.

	call	TransmitNextByte
   .assert "U1.rx == 0x31, \"*** FAILED sending 0x31\""

	call	TransmitNextByte
   .assert "U1.rx == 0x32, \"*** FAILED sending 0x32\""
	call	TransmitNextByte
   .assert "U1.rx == 0x33, \"*** FAILED sending 0x33\""
	call	TransmitNextByte
   .assert "U1.rx == 0x34, \"*** FAILED sending 0x34\""
	call	TransmitNextByte
   .assert "U1.rx == 0x35, \"*** FAILED sending 0x35\""
	call	TransmitNextByte
   .assert "U1.rx == 0x36, \"*** FAILED sending 0x36\""
	call	TransmitNextByte
   .assert "U1.rx == 0x37, \"*** FAILED sending 0x37\""
	call	TransmitNextByte
   .assert "U1.rx == 0x38, \"*** FAILED sending 0x38\""
	call	TransmitNextByte
   .assert "U1.rx == 0x39, \"*** FAILED sending 0x39\""
	call	TransmitNextByte
   .assert "U1.rx == 0x41, \"*** FAILED sending 0x41\""
	call	TransmitNextByte
   .assert "U1.rx == 0x42, \"*** FAILED sending 0x42\""
	call	TransmitNextByte
   .assert "U1.rx == 0x43, \"*** FAILED sending 0x43\""
	call	TransmitNextByte
   .assert "U1.rx == 0x44, \"*** FAILED sending 0x44\""

	nop

done:
  .assert  "\"*** PASSED Usart Module test\""
	goto	done


TransmitNextByte:	
	call	tx_message
	btfss	PIR1,TXIF
	 goto	$-1
	movwf	TXREG

	clrf	temp2
	call	delay		;; Delay between bytes.

	return

tx_message
	incf	tx_ptr,w
	andlw	0x0f
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
	btfss	TXSTA^0x80,TRMT	;Wait 'til through transmitting
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


delay	
	decfsz	temp1,f
	 goto 	$+2
	decfsz	temp2,f
	 goto   delay
	return

	end
