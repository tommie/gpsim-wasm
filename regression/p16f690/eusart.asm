   ;;  EUSART test 
   ;;
   ;;  The purpose of this program is to verify that gpsim's
   ;; USART functions properly when configured as an EUSART. 
   ;; The USART module is used to loop
   ;; characters back to the receiver testing  RCIF interupts.
   ;;
   ;;
   ;;

	list	p=16f687
	include <p16f687.inc>
	include <coff.inc>

 __CONFIG _WDT_OFF  & _FOSC_HS & _IESO_ON


        errorlevel -302 
	radix dec
;	in following equation 100000 if Fosc/100, 48 is baud/100
BAUDHI  equ     ((100000/4)/48)-1
BAUDLO  equ     ((100000/16)/48)-1


;----------------------------------------------------------------------
; RAM Declarations


;
GPR_DATA                UDATA_SHR

#define	RX_BUF_SIZE	0x10
w_temp          RES     1
status_temp     RES     1
pclath_temp     RES     1

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

	btfsc	INTCON,PEIE
	 btfss	PIR1,RCIF
	  goto	int_done

;;;	Received a Character
   .assert "rcreg == txreg, '*** FAILED sent character looped back'"
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

   .sim ".frequency=10e6"
   .sim "break c 0x100000"
   .sim "module library libgpsim_modules"
   .sim "module load usart U1"
   .sim "U1.xpos = 84"
   .sim "U1.ypos = 276"

   .sim "node PIC_tx"
   .sim "node PIC_rx"

   ;; Tie the USART module to the PIC
   .sim "attach PIC_tx portb7  U1.RXPIN"
   .sim "attach PIC_rx portb5 U1.TXPIN"

   ;; Set the USART module's Baud Rate

   .sim "U1.txbaud = 4800"
   .sim "U1.rxbaud = 4800"

   .sim "U1.loop = true"

	;; USART Initialization
	;;
	;; Turn on the high baud rate (BRGH), disable the transmitter,
	;; disable synchronous mode.
	;;

	BANKSEL OSCCON
	.assert "p16f687.frequency == 4000000., 'FALIED 16f687 default frequency'"
	nop
	btfss	OSCCON,OSTS	; wait for hs to be active with 2 speed start
	goto	$-1
	.assert "cycles > 1014 && cycles <1028, 'FALIED 16f687 2 speed start'"
	nop
	.assert "p16f687.frequency == 10000000., 'FALIED 16f687  HS frequency'"
	nop

	
	clrf	STATUS

	bsf	PORTB,7         ;Make sure the TX line drives high when 
                                ;it is programmed as an output.
	BANKSEL ANSELH
	bcf	ANSELH,ANS11

	BANKSEL TRISB
	bsf	TRISB,5		;RX is an input
	bsf	TRISB,7		;TX EUSART sets pin direction

	;; CSRC - clock source is a don't care
	;; TX9  - 0 8-bit data
	;; TXEN - 0 don't enable the transmitter.
	;; SYNC - 0 Asynchronous
	;; BRGH - 1 Select high baud rate divisor
	;; TRMT - x read only
	;; TX9D - 0 not used
	
	BANKSEL TXSTA
	movlw	(1<<BRGH)
	movwf	TXSTA

	BANKSEL SPBRG
	movlw   BAUDLO  	;4800 baud at 10MHz clock.
	movwf   SPBRG


  .assert "(portb & 0x80) == 0x80, 'FAILED: TX bit initilized as high'"
	nop
	clrf	tx_ptr
			
	;; Turn on the serial port
	BANKSEL RCSTA
	movlw	(1<<SPEN) | (1<<CREN)
	movwf	RCSTA

	movf	RCREG,w          ;Clear RCIF
	bsf	INTCON,GIE
	bsf	INTCON,PEIE

	movf	RCREG,w          ;Clear RCIF
	movf	RCREG,w          ;Clear RCIF

	;; Test TXIF, RCIF bits of PIR1 are not writable

	clrf	PIR1
	bsf	PIR1,RCIF
	bsf	PIR1,TXIF
  .assert "pir1 == 0x00, '*** FAILED TXIF, RCIF not writable'"
	nop

	;; Enable the transmitter
	BANKSEL TXSTA
	bsf	TXSTA,TXEN
  .assert "pir1 == 0x10, '*** FAILED TXIF should now be set'"
	nop
	BANKSEL PIE1
	bsf	PIE1,RCIE	; Enable Rx interrupts

	;; Now Transmit some data and verify that it is transmitted correctly.

	call	TransmitNextByte
   .assert "U1.rx == 0x31, '*** FAILED sending 0x31'"
	nop
	call	TransmitNextByte
   .assert "U1.rx == 0x32, '*** FAILED sending 0x32'"
	nop
	call	TransmitNextByte
   .assert "U1.rx == 0x33, '*** FAILED sending 0x33'"
	nop
	call	TransmitNextByte
   .assert "U1.rx == 0x34, '*** FAILED sending 0x34'"
	nop

        ;; Switch to 16-bit BRG mode
	BANKSEL BAUDCTL
        bsf     BAUDCTL,BRG16
        movlw   low(BAUDHI)
        movwf   SPBRG
        movlw   high(BAUDHI)
        movwf   SPBRGH
        call   delay

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
	nop
;
; setup tmr0
;
	BANKSEL OPTION_REG   ;
	movlw   b'11000000' ;Mask TMR0 select and
	andwf   OPTION_REG,W ; prescaler bits
	iorlw   b'00000101' ;Set prescale to 1:64
	movwf   OPTION_REG   ;

	BANKSEL TMR0
        clrf    TMR0
        movlw   0x55
        movwf   TXREG

        btfss   PIR1,TXIF       ;Did the interrupt flag get set?
         goto   $-1

	BANKSEL TXSTA
        btfss   TXSTA,TRMT ;Wait 'til through transmitting
         goto    $-1
;
;  At 4800 baud each bit takes 0.208 msec. TRMT will be low > 9 bits 
;  and < 10 bits or between 1.87  and 2.08  msec.
;  with oscillator at 10MHz and TMR0 / 64 expect between 73 and 81
;  TMR0 cycles.

	BANKSEL TMR0
	movf	TMR0,W

  .assert "tmr0 > 73 && tmr0 <= 81, '*** FAILED baud rate'"
	nop
	clrf	rxFlag
        call rx_loop

        ; Disable interrupts because the following tests don't give good receive bytes
	BANKSEL PIE1
        bcf     PIE1,RCIE

	BANKSEL TXSTA
        bsf     TXSTA,SENB     ; request a break sequence
	BANKSEL PORTB
        btfss   PORTB,7         ; Shouldn't happen just yet
    .assert "'*** FAILED break sent too soon'"
        nop

        clrf    TMR0
        movlw   0x55
        movwf   TXREG

        call   delay

        btfsc   PORTB,7         ; Should happen by now
    .assert "'*** FAILED to send break'"
        nop

        btfss   PORTB,7         ; Wait for stop bit
         goto    $-1
;
;  At 4800 baud each bit takes 0.208 msec. Output will be low for
;  start + 12 bit times or 2.70 msec. With 10Mhz TMR0 / 64 is 106 TMR0 counts.

	movf	TMR0,W

  .assert "tmr0 > 101 && tmr0 < 111, '*** FAILED sync pulse'"
	nop

done:
  .assert  "'*** PASSED E-Usart on 16F687'"
	goto $



tx_message:
	incf	tx_ptr,w
	andlw	0x0f
	movwf	tx_ptr
        addlw   0x30
        return


delay:
	decfsz	temp2,f
	 goto    delay
	return



TransmitNextByte:	
	BANKSEL TXREG
	clrf	rxFlag
	call	tx_message
	movwf	TXREG

rx_loop:

	btfss	rxFlag,0
	 goto	rx_loop

;	clrf	temp2
;	call	delay		;; Delay between bytes.

	btfss	PIR1,TXIF
	 goto	$-1

	return

	end
