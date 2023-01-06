   ;;  USART test 
   ;;
   ;;  The purpose of this program is to verify that gpsim's
   ;; USART functions properly. The USART module is used to loop
   ;; characters back to the receiver testing  RCIF interupts.
   ;;
   ;;
   ;;

	list	p=16f88
	include <p16f88.inc>
	include <coff.inc>
        __CONFIG _WDT_OFF


.command macro x
  .direct "C", x
  endm

        errorlevel -302 
	radix dec
;----------------------------------------------------------------------
; RAM Declarations


;
INT_VAR        UDATA   0x20
w_temp          RES     1
status_temp     RES     1
pclath_temp     RES     1
;fsr_temp	RES	1

temp1		RES	1
temp2		RES	1
temp3		RES	1

tx_ptr		RES	1

rxLastByte	RES	1
rxFlag		RES	1

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

;;;	Received a Character
   .assert "rcreg == txreg, 'sent character looped back'"
	nop
	movf	RCREG,W
	movwf	rxLastByte
	bsf	rxFlag,0
	
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
   .sim "break c 70000"
   .sim "module library libgpsim_modules"
   .sim "module load usart U1"
   .sim ".xpos=72"
   .sim ".ypos=156"
 ;  .sim "U1.xpos = 250.0"
;   .sim "U1.ypos = 80.0"

   .sim "node PIC_tx"
   .sim "node PIC_rx"

   ;; Tie the USART module to the PIC
   .sim "attach PIC_tx portb5 U1.RXPIN"
   .sim "attach PIC_rx portb2 U1.TXPIN"

   ;; Set the USART module's Baud Rate

   .sim "U1.txbaud = 28800"
   .sim "U1.rxbaud = 28800"
   .sim "U1.loop = true"
   .sim "scope.ch0='portb5'"

	;; USART Initialization
	;;
	;; Turn on the high baud rate (BRGH), disable the transmitter,
	;; disable synchronous mode.
	;;
	
	clrf	STATUS

	bsf	PORTB,5         ;Make sure the TX line drives high when 
                                ;it is programmed as an output.

	bsf	STATUS,RP0	; select bank 0
;	bcf	OPTION_REG,NOT_RBPU	; turn on B pullups



	bsf	TRISB,2		;RX is an input
	bsf	TRISB,5		;TX is an input

	;; CSRC - clock source is a don't care
	;; TX9  - 0 8-bit data
	;; TXEN - 0 don't enable the transmitter.
	;; SYNC - 0 Asynchronous
	;; BRGH - 1 Select high baud rate divisor
	;; TRMT - x read only
	;; TX9D - 0 not used
	
	movlw	(1<<BRGH)
	movwf	TXSTA ^ 0x80

	movlw   42		;28800 baud.
	movwf   SPBRG ^0x80

	bcf	STATUS,RP0
  .assert "(portb & 0x04) == 0x04, 'FAILED: TX bit initilized as high'"

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

	;; Test TXIF, RCIF bits of PIR1 are not writable

	clrf	PIR1
	bsf	PIR1,RCIF
	bsf	PIR1,TXIF
  .assert "pir1 == 0x00, '*** FAILED TXIF, RCIF not writable'"
	nop

	;; Enable the transmitter
	bsf	STATUS,RP0
	bsf	TXSTA^0x80,TXEN

  .assert "pir1 == 0x10, '*** FAILED TXIF should now be set'"
	nop

	;; Now Transmit some data and verify that it is transmitted correctly.
   .command "U1.tx = \"Hi!\r\n\""
	nop
	BANKSEL PIR1
	bcf	PIR1,RCIF
	btfss   PIR1,RCIF
	 goto	$-1
    .assert "rcreg == 0x48, '*** FAILED p16f88 USART string tx H'"
	nop
	movf	RCREG,W

	bcf	PIR1,RCIF
	btfss   PIR1,RCIF
	 goto	$-1
    .assert "rcreg == 0x69, '*** FAILED p16f88 USART string tx i'"
	nop
	movf	RCREG,W
	bcf	PIR1,RCIF
	btfss   PIR1,RCIF
	 goto	$-1
    .assert "rcreg == 0x21, '*** FAILED p16f88 USART string tx !'"
	nop
	movf	RCREG,W
	bcf	PIR1,RCIF
	btfss   PIR1,RCIF
	 goto	$-1
    .assert "rcreg == 0x0d, '*** FAILED p16f88 USART string tx \\r'"
	nop
	movf	RCREG,W
	bcf	PIR1,RCIF
	btfss   PIR1,RCIF
	 goto	$-1
    .assert "rcreg == 0x0a, '*** FAILED p16f88 USART string tx \\n'"
	nop
	movf	RCREG,W

	.command "rcreg"
	nop
	BANKSEL PIE1
	bsf	PIE1,RCIE	; Enable Rx interrupts
	bcf	STATUS,RP0
	call	TransmitNextByte
   .assert "U1.rx == 0x31, '*** FAILED sending 0x31'"
	nop
	call	TransmitNextByte
   .assert "U1.rx == 0x32, '*** FAILED sending 0x32'"
	nop
	call	TransmitNextByte
   .assert "U1.rx == 0x33, '*** FAILED sending 0x33'"
	call	TransmitNextByte
   .assert "U1.rx == 0x34, '*** FAILED sending 0x34'"
	call	TransmitNextByte
   .assert "U1.rx == 0x35, '*** FAILED sending 0x35'"
	call	TransmitNextByte
   .assert "U1.rx == 0x36, '*** FAILED sending 0x36'"
	call	TransmitNextByte
   .assert "U1.rx == 0x37, '*** FAILED sending 0x37'"
	call	TransmitNextByte
   .assert "U1.rx == 0x38, '*** FAILED sending 0x38'"
	call	TransmitNextByte
   .assert "U1.rx == 0x39, '*** FAILED sending 0x39'"
	call	TransmitNextByte
   .assert "U1.rx == 0x41, '*** FAILED sending 0x41'"
	call	TransmitNextByte
   .assert "U1.rx == 0x42, '*** FAILED sending 0x42'"
	call	TransmitNextByte
   .assert "U1.rx == 0x43, '*** FAILED sending 0x43'"
	call	TransmitNextByte
   .assert "U1.rx == 0x44, '*** FAILED sending 0x44'"
	nop

;
; setup tmr0
;
        bsf    STATUS, RP0   ;  Bank1
        movlw  0x05          ; Tmr0 internal clock prescaler 64
        movwf  OPTION_REG
        bcf    STATUS, RP0   ;  Bank0

        movlw   0x55
        btfss   PIR1,TXIF       ;check for TXREG ready
         goto   $-1
        clrf    TMR0
        movwf   TXREG

        bsf     STATUS,RP0
        btfss   TXSTA^0x80,TRMT ;Wait 'til through transmitting
         goto   $-1
        bcf     STATUS,RP0
;
;  At 28800 baud each bit takes 34.7 usec. So 9 bits is 0.3125 msec
;  10 bits 0.347 msec.  FOSC/4 = 5MHZ and TMR0 / 64 so TMR0 between
;  24 (0x18) and 27 (0x1b),

	movf	TMR0,W

  .assert "tmr0 > 24 && tmr0 < 27, '*** FAILED baud rate'"
	nop
	clrf	rxFlag
        call rx_loop

done:
  .assert  "'*** PASSED Usart p16f88'"
	goto $


TransmitNextByte:	
	clrf	rxFlag
	call	tx_message
	btfss	PIR1,TXIF
	 goto	$-1
	movwf	TXREG

rx_loop:

	btfss	rxFlag,0
	 goto	rx_loop

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
