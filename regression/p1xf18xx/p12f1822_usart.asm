   ;;  USART test 
   ;;
   ;;  The purpose of this program is to verify that gpsim's
   ;; USART functions properly. The USART module is used to loop
   ;; characters back to the receiver testing  RCIF interupts.
   ;;
   ;;
   ;;

	list	p=12f1822
	include <p12f1822.inc>
	include <coff.inc>
       __CONFIG _CONFIG1, _CP_OFF & _WDTE_ON &  _FOSC_INTOSC & _PWRTE_ON &  _BOREN_OFF & _MCLRE_OFF & _CLKOUTEN_OFF



        errorlevel -302 
	radix dec
;----------------------------------------------------------------------
; RAM Declarations


;
INT_VAR        UDATA   0x70

temp1		RES	1
temp2		RES	1
temp3		RES	1

tx_ptr		RES	1

rxLastByte	RES	1
rxFlag		RES	1

 global  rxLastByte
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

	clrf    BSR             ; set bank 0

	btfsc	INTCON,PEIE
	 btfss	PIR1,RCIF
	  goto	int_done

;;;	Received a Character
   .assert "rcreg == txreg, 'sent character looped back'"
	nop
	BANKSEL	RCREG
	movf	RCREG,W
	movwf	rxLastByte
	bsf	rxFlag,0
	
int_done:
	retfie


;; ----------------------------------------------------
;;
;;            start
;;

MAIN    CODE
start	

   .sim ".frequency=16e6"
   .sim "break c 0x100000"
   .sim "module library libgpsim_modules"
   .sim "module load usart U1"
   .sim "U1.xpos = 250.0"
   .sim "U1.ypos = 80.0"

   .sim "module load usart U2"
   .sim "U2.xpos = 80.0"
   .sim "U2.ypos = 200.0"

   .sim "node PIC_tx"
   .sim "node PIC_rx"

   .sim "node PIC_tx2"
   .sim "node PIC_rx2"

   ;; Tie the USART module to the PIC
   .sim "attach PIC_tx porta0 U1.RXPIN"
   .sim "attach PIC_rx porta1 U1.TXPIN"

   ;; Tie the USART module to the PIC
   .sim "attach PIC_tx2 porta4 U2.RXPIN"
   .sim "attach PIC_rx2 porta5 U2.TXPIN"

   ;; Set the USART module's Baud Rate

   .sim "U1.txbaud = 9600"
   .sim "U1.rxbaud = 9600"
   .sim "U1.loop = true"

   .sim "U2.txbaud = 9600"
   .sim "U2.rxbaud = 9600"
   .sim "U2.loop = true"

        ;set clock to 16 Mhz
        BANKSEL OSCCON
        bsf     OSCCON,6

	BANKSEL	APFCON
	movlw	0x84
	;movwf	APFCON

	;; USART Initialization
	;;
	;; Turn on the high baud rate (BRGH), disable the transmitter,
	;; disable synchronous mode.
	;;
	
	clrf	STATUS
        BANKSEL ANSELA
        clrf    ANSELA  ; set port to digital

	BANKSEL	PORTA
	bsf	PORTA,0         ;Make sure the TX line drives high when 
                                ;it is programmed as an output.


	BANKSEL	TRISA
	bsf	TRISA,1		;RX is an input
	bcf	TRISA,0		;TX is an output
	bsf	TRISA,5		;Alternate RX is an input
	bcf	TRISA,4		;Alternate TX is an output


	BANKSEL	SPBRGL
	movlw   25		;9600 baud.
	movwf   SPBRGL

	clrf	BSR
  .assert "(porta & 0x01) == 0x01, 'FAILED: 12F1822 USART TX bit initilized as high'"

	clrf	tx_ptr
			
	;; Turn on the serial port
	BANKSEL	RCSTA
	movlw	(1<<SPEN) | (1<<CREN)
	movwf	RCSTA

	movf	RCREG,w          ;Clear RCIF
	bsf	INTCON,GIE
	bsf	INTCON,PEIE

	movf	RCREG,w          ;Clear RCIF
	movf	RCREG,w          ;Clear RCIF

	;; Test TXIF, RCIF bits of PIR1 are not writable

	BANKSEL	PIR1
	clrf	PIR1
	bsf	PIR1,RCIF
	bsf	PIR1,TXIF
  .assert "pir1 == 0x00, '*** FAILED 12F1822 USART TXIF, RCIF not writable'"
	nop

	;; Enable the transmitter
	BANKSEL	TXSTA
	bsf	TXSTA,TXEN
  .assert "pir1 == 0x10, '*** FAILED 12F1822 USART TXIF should now be set'"
	nop
	BANKSEL	PIE1
	bsf	PIE1,RCIE	; Enable Rx interrupts
	clrf	BSR

	;; Now Transmit some data and verify that it is transmitted correctly.

	call	TransmitNextByte
   .assert "rxLastByte == 0x31, '*** FAILED 12F1822 USART -  sending 0x31'"
	nop

	BANKSEL	APFCON
	movlw	0x84
	movwf	APFCON

	call	TransmitNextByte
   .assert "rxLastByte == 0x32, '*** FAILED 12F1822 USART -  sending 0x32'"
	nop

	BANKSEL	APFCON
	clrf	APFCON

	call	TransmitNextByte
   .assert "rxLastByte == 0x33, '*** FAILED 12F1822 USART -  sending 0x33'"
	nop

	BANKSEL	APFCON
	movlw	0x84
	movwf	APFCON

	call	TransmitNextByte
   .assert "rxLastByte == 0x34, '*** FAILED 12F1822 USART -  sending 0x34'"
	nop

	BANKSEL	APFCON
	clrf	APFCON

	call	TransmitNextByte
   .assert "rxLastByte == 0x35, '*** FAILED 12F1822 USART -  sending 0x35'"
	nop

	BANKSEL	APFCON
	movlw	0x84
	movwf	APFCON

	call	TransmitNextByte
   .assert "rxLastByte == 0x36, '*** FAILED 12F1822 USART -  sending 0x36'"
	nop

	BANKSEL	APFCON
	clrf	APFCON

	call	TransmitNextByte
   .assert "rxLastByte == 0x37, '*** FAILED 12F1822 USART -  sending 0x37'"
	nop

	BANKSEL	APFCON
	movlw	0x84
	movwf	APFCON

	call	TransmitNextByte
   .assert "rxLastByte == 0x38, '*** FAILED 12F1822 USART -  sending 0x38'"
	nop

	BANKSEL	APFCON
	clrf	APFCON

	call	TransmitNextByte
   .assert "rxLastByte == 0x39, '*** FAILED 12F1822 USART -  sending 0x39'"
	nop

	BANKSEL	APFCON
	movlw	0x84
	movwf	APFCON

	call	TransmitNextByte
   .assert "rxLastByte == 0x41, '*** FAILED 12F1822 USART -  sending 0x41'"
	nop

	BANKSEL	APFCON
	clrf	APFCON

	call	TransmitNextByte
   .assert "rxLastByte == 0x42, '*** FAILED 12F1822 USART -  sending 0x42'"
	nop

	BANKSEL	APFCON
	movlw	0x84
	movwf	APFCON

	call	TransmitNextByte
   .assert "rxLastByte == 0x43, '*** FAILED 12F1822 USART -  sending 0x43'"
	nop

	BANKSEL	APFCON
	clrf	APFCON

	call	TransmitNextByte
   .assert "rxLastByte == 0x44, '*** FAILED 12F1822 USART -  sending 0x44'"
	nop

;
; setup tmr0
;
	BANKSEL	OPTION_REG
        movlw  0x05          ; Tmr0 internal clock prescaler 64
        movwf  OPTION_REG
	clrf	BSR

        clrf    TMR0
	BANKSEL	TXREG
        movlw   0x55
        movwf   TXREG

	BANKSEL	PIR1
        btfss   PIR1,TXIF       ;Did the interrupt flag get set?
         goto   $-1

	BANKSEL	TXSTA
        btfss   TXSTA,TRMT ;Wait 'til through transmitting
         goto   $-1
	clrf	BSR
;
;  At 9600 baud each bit takes 0.104 msec. TRMT will be low > 9 bits 
;  and < 10 bits or between 0.9375 and 1.041 msec.
;  with oscillator at 16MHz and TMR0 / 64 expect between 58 and 65
;  TMR0 cycles.

	movf	TMR0,W

  .assert "tmr0 > 58 && tmr0 < 65, '*** FAILED 12F1822 USART baud rate'"
	nop
	clrf	rxFlag
        call rx_loop

done:
  .assert  "'*** PASSED 12F1822 USART'"
	goto $


TransmitNextByte:	
	clrf	rxFlag
	call	tx_message
	BANKSEL TXREG
	movwf	TXREG

	BANKSEL	PIR1

rx_loop:

	btfss	rxFlag,0
	 goto	rx_loop

;	clrf	temp2
;	call	delay		;; Delay between bytes.

	btfss	PIR1,TXIF
	 goto	$-1

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



delay	
	decfsz	temp1,f
	 goto 	$+2
	decfsz	temp2,f
	 goto   delay
	return

	end
