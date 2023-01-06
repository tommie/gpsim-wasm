   ;;  USART TXISR test 
   ;;
   ;;  The purpose of this program is to verify that gpsim's
   ;; USART module generates TX interrupts properly.
   ;;
   ;;
   ;;

	list	p=16f871
	include <p16f871.inc>
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
;fsr_temp	RES	1

INT_VAR1       UDATA   0xA0
w_temp1          RES     1	;Alias for w_temp at address 0x20


GPR_DAT        UDATA    0x30

temp1		RES	1
temp2		RES	1

tx_ptr		RES	1

rxLastByte	RES	1
rxFlag		RES	1

  GLOBAL tx_ptr

;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlw  high  start               ; load upper byte of 'start' label
        movwf  PCLATH                    ; initialize PCLATH
  .assert "tx_ptr==0, '*** FAILED unexpected reset'"
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

	btfss	PIR1,TXIF
	goto	int_done

	call	tx_message
	movwf	TXREG

	btfss	tx_ptr,5
        goto	int_done

	bsf	STATUS,RP0
	bcf	PIE1^0x80,TXIE	; Enable Tx interrupts
	bcf	STATUS,RP0

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

   .sim "tx_ptr=0"

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
	bcf	TRISC,6		;RC6 must be an input for TX to drive pin

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

;	clrf	tx_ptr
			
	;; Turn on the serial port
	movlw	(1<<SPEN) | (1<<CREN)
	movwf	RCSTA

	movf	RCREG,w          ;Clear RCIF
	bsf	INTCON,GIE
	bsf	INTCON,PEIE

	;; Delay for a moment to allow the I/O lines to settle
	clrf	temp2
	call	delay
	
	movf	RCREG,w          ;Clear RCIF
	movf	RCREG,w          ;Clear RCIF

	movf	tx_ptr,w
	skpz

   .assert "'*** FAILED - premature interrupts'"
	nop

	;; Enable the transmitter
	bsf	STATUS,RP0
	bsf	TXSTA^0x80,TXEN
	bsf	PIE1^0x80,TXIE	; Enable Tx interrupts
	bcf	STATUS,RP0

	movf	tx_ptr,w
        sublw	2
        skpz

   .assert "'*** FAILED to generate interrupts'"

	nop

	;; Delay for a moment to allow the transmission to finish
	clrf	temp2
	call	delay
	call	delay
	call	delay

	btfsc	tx_ptr,5
	btfsc	tx_ptr,4

   .assert "'*** FAILED to stop interrupts'"

	nop

done:
  .assert  "'*** PASSED Usart interrupt test'"

	goto	done


tx_message
	incf	tx_ptr,w
	movwf	tx_ptr
	andlw	0x0f
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
