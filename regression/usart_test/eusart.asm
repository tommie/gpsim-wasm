   ;;  EUSART test 
   ;;
   ;;  The purpose of this program is to verify that gpsim's
   ;; USART functions properly when configured as an EUSART. 
   ;; The USART module is used to loop
   ;; characters back to the receiver testing  RCIF interupts.
   ;;
   ;;
   ;;

	list	p=18f2221
	include <p18f2221.inc>
	include <coff.inc>

 CONFIG WDT=OFF
 CONFIG MCLRE=ON, LPT1OSC=OFF, PBADEN=DIG, CCP2MX=RC1


        errorlevel -302 
	radix dec

BAUDHI  equ     ((100000/4)/48)-1
BAUDLO  equ     129


;----------------------------------------------------------------------
; RAM Declarations


;
INT_VAR        UDATA   0x00
w_temp          RES     1
status_temp     RES     1
pclath_temp     RES     1



GPR_DAT        UDATA

#define	RX_BUF_SIZE	0x10

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

INT_VECTOR   CODE    0x018               ; interrupt vector location

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
   .assert "rcreg == txreg, '*** FAILED p18f2221 sent character looped back'"
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
   .sim "p18f2221.xpos = 48"
   .sim "p18f2221.ypos = 48"

   .sim "node PIC_tx"
   .sim "node PIC_rx"

   ;; Tie the USART module to the PIC
   .sim "attach PIC_tx portc6  U1.RXPIN"
   .sim "attach PIC_rx portc7 U1.TXPIN"

   ;; Set the USART module's Baud Rate

   .sim "U1.txbaud = 4800"
   .sim "U1.rxbaud = 4800"
   .sim "U1.loop = true"
   .sim "U1.xpos = 216"
   .sim "U1.ypos = 168"

	;; USART Initialization
	;;
	;; Turn on the high baud rate (BRGH), disable the transmitter,
	;; disable synchronous mode.
	;;
	
	clrf	STATUS

	bsf	PORTC,6         ;Make sure the TX line drives high when 
                                ;it is programmed as an output.

	bsf	TRISC,7		;RX is an input
;	bsf	TRISC,6		;TX EUSART sets pin direction

	;; CSRC - clock source is a don't care
	;; TX9  - 0 8-bit data
	;; TXEN - 0 don't enable the transmitter.
	;; SYNC - 0 Asynchronous
	;; BRGH - 1 Select high baud rate divisor
	;; TRMT - x read only
	;; TX9D - 0 not used
	
	movlw	(1<<BRGH)
	movwf	TXSTA

	movlw   BAUDLO  	;4800 baud at 10MHz clock.
	movwf   SPBRG


  .assert "(portc & 0x40) == 0x40, '*** FAILED: p18f2221 TX bit initilized as high'"
	clrf	tx_ptr
			
	;; Turn on the serial port
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
  .assert "pir1 == 0x00, '*** FAILED p18f2221 TXIF, RCIF not writable'"
	nop

	;; Enable the transmitter
	bsf	TXSTA,TXEN
  .assert "pir1 == 0x10, '*** FAILED p18f2221 TXIF should now be set'"

	bsf	TXSTA,SENDB
	clrf	TXREG
	btfsc	TXSTA,SENDB
	bra	$-2
	bsf	PIE1,RCIE	; Enable Rx interrupts

	;; Now Transmit some data and verify that it is transmitted correctly.

	call	TransmitNextByte
   .assert "U1.rx == 0x31, '*** FAILED p18f2221 sending 0x31'"
	nop
	call	TransmitNextByte
   .assert "U1.rx == 0x32, '*** FAILED p18f2221 sending 0x32'"
	nop
	call	TransmitNextByte
   .assert "U1.rx == 0x33, '*** FAILED p18f2221 sending 0x33'"
	nop
	call	TransmitNextByte
   .assert "U1.rx == 0x34, '*** FAILED p18f2221 sending 0x34'"
	nop

        ;; Switch to 16-bit BRG mode
        bsf     BAUDCON,BRG16
        movlw   low(BAUDHI)
        movwf   SPBRG
        movlw   high(BAUDHI)
        movwf   SPBRGH
        rcall   delay

	call	TransmitNextByte
   .assert "U1.rx == 0x35, '*** FAILED p18f2221 sending 0x35'"
	call	TransmitNextByte
   .assert "U1.rx == 0x36, '*** FAILED p18f2221 sending 0x36'"
	call	TransmitNextByte
   .assert "U1.rx == 0x37, '*** FAILED p18f2221 sending 0x37'"
	call	TransmitNextByte
   .assert "U1.rx == 0x38, '*** FAILED p18f2221 sending 0x38'"
	call	TransmitNextByte
   .assert "U1.rx == 0x39, '*** FAILED p18f2221 sending 0x39'"
	nop
;
; setup tmr0
;
        movlw  0xC5          ; Tmr0 internal clock prescaler 64
        movwf  T0CON

        btfss   PIR1,TXIF       ;Wait for TXREG ready for write
         goto   $-1
        clrf    TMR0L
        movlw   0x55
        movwf   TXREG

        btfss   TXSTA,TRMT ;Wait 'til through transmitting
         bra    $-2
;
;  At 9600 baud each bit takes 0.104 msec. TRMT will be low > 9 bits 
;  and < 10 bits or between 0.9375 and 1.041 msec.
;  with oscillator at 20MHz and TMR0 / 64 expect between 73 and 81
;  TMR0 cycles.

	movf	TMR0L,W

  .assert "tmr0 > 73 && tmr0 < 81, '*** FAILED p18f2221 baud rate'"
	nop
	clrf	rxFlag
        call rx_loop

        ; Disable interrupts because the following tests don't give good receive bytes
        bcf     PIE1,RCIE

        bsf     TXSTA,SENDB     ; request a break sequence
        btfss   PORTC,6         ; Shouldn't happen just yet
    .assert "'*** FAILED p18f2221 break sent too soon'"
        nop

        clrf    TMR0L
        movlw   0x55
        movwf   TXREG

        rcall   delay

        btfsc   PORTC,6         ; Should happen by now
    .assert "'*** FAILED p18f2221 to send break'"
        nop

        btfss   PORTC,6         ; Wait for stop bit
         bra    $-2
;
;  At 4800 baud each bit takes 0.208 msec. Output will be low for
;  start + 12 bit times or 2.70 msec. With 10Mhz TMR0 / 64 is 106 TMR0 counts.

	movf	TMR0L,W

  .assert "tmr0 > 101 && tmr0 <= 111, '*** FAILED sync pulse'"
	nop

done:
  .assert  "'*** PASSED E-Usart on 18F2221'"
	goto $



tx_message:
	incf	tx_ptr,w
	andlw	0x0f
	movwf	tx_ptr
        addlw   0x30
        return


delay:
	decfsz	temp2,f
	 bra    delay
	return



TransmitNextByte:	
	clrf	rxFlag
	call	tx_message
	btfss	PIR1,TXIF
	 bra	$-2
	movwf	TXREG
        clrwdt

rx_loop:

	btfss	rxFlag,0
	 bra	rx_loop


	return

	end
