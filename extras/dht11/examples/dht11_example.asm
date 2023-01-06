   ;;  DHT11 temperature sensor read->uart
   ;;
   ;;  Reads from a DHT11 temperature sensor and outputs as ASCII
   ;; on a 9600 baud uart
   ;;
   ;; dave@treblig.org
   ;; based on the usart_test.asm code in gpsim

	list	p=16f627
	include <p16f627.inc>
	include <coff.inc>
        __CONFIG ( _WDT_OFF & _HS_OSC )


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

humid_int       RES     1
humid_dec       RES     1
tempe_int       RES     1
tempe_dec       RES     1
dht_sum         RES     1

tx_ptr		RES	1
rx_ptr		RES	1
rx_buffer	RES	RX_BUF_SIZE


; RB1 on the 627
#define rx_pin   1
; RB2 on the 627
#define tx_pin   2
; RB5 on the 627
#define dht_pin  5

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
	retfie   ; We're not using any


;; ----------------------------------------------------
;;
;;            start
;;

MAIN    CODE
start

   .sim ".frequency=4e6"

   .sim "module library libgpsim_modules"
   .sim "module load usart U1"

   .sim "node PIC_tx"
   .sim "node PIC_rx"
   .sim "p16f627.xpos = 192.0"
   .sim "p16f627.ypos = 156.0"

   ;; Tie the USART module to the PIC
   .sim "attach PIC_tx portb2 U1.RXPIN"
   .sim "attach PIC_rx portb1 U1.TXPIN"

   ;; Set the USART module's Baud Rate

   .sim "U1.txbaud = 9600"
   .sim "U1.rxbaud = 9600"
   .sim "U1.xpos = 192.0"
   .sim "U1.ypos = 48.0"

   .sim "module load pullup DHTPull"
   .sim "DHTPull.xpos = 228.0"
   .sim "DHTPull.ypos = 340.0"

   .sim "module library libgpsim_extras"
   .sim "module load dht11 D1"
   .sim "node dhtdata"
   .sim "attach dhtdata portb5 D1.data DHTPull.pin"
   .sim "D1.xpos = 228.0"
   .sim "D1.ypos = 312.0"

	clrf	STATUS

	bsf	PORTB,tx_pin    ;Make sure the TX line drives high when
                                ;it is programmed as an output.
	bcf	PORTB,dht_pin   ;The only time we drive DHT is to drive it low

	bsf	STATUS,RP0      ;TRIS* is bank 1

	bsf	TRISB,rx_pin	;RX is an input
	bcf	TRISB,tx_pin	;TX is an output
	bsf	TRISB,dht_pin	;Make DHT input for now - it'll get pulled up

	;; CSRC - clock source is a don't care
	;; TX9  - 0 8-bit data
	;; TXEN - 0 don't enable the transmitter.
	;; SYNC - 0 Asynchronous
	;; BRGH - 1 Select high baud rate divisor
	;; TRMT - x read only
	;; TX9D - 0 not used

	movlw	(1<<BRGH)
	movwf	TXSTA ^ 0x80

	movlw   25		;9600 baud @ 4MHz clock, BRGH set
	movwf   SPBRG ^0x80

	bcf	STATUS,RP0

	clrf	tx_ptr

	;; Turn on the serial port
	movlw	(1<<SPEN) | (1<<CREN)
	movwf	RCSTA

	movf	RCREG,w          ;Clear RCIF
	bcf	INTCON,GIE       ; All interrupts OFF
	bcf	INTCON,PEIE      ; Double sure - Peripheral interrupts off

	movf	RCREG,w          ;Clear RCIF
	movf	RCREG,w          ;Clear RCIF

	;; Enable the transmitter
	bsf	STATUS,RP0
	bsf	TXSTA^0x80,TXEN
	bsf	PIE1^0x80,RCIE
	bcf	STATUS,RP0

;; Main loop
loop:
	call    readdht

        movlw   42
        call    TransmitAndWait
        movlw   42
        call    TransmitAndWait
        movlw   42
        call    TransmitAndWait
        movlw   'H'
        call    TransmitAndWait
        movf    humid_int,0
        call    TransmitAsHex
        movlw   '.'
        call    TransmitAndWait
        movf    humid_dec,0
        call    TransmitAsHex
        movlw   'T'
        call    TransmitAndWait
        movf    tempe_int,0
        call    TransmitAsHex
        movlw   '.'
        call    TransmitAndWait
        movf    tempe_dec,0
        call    TransmitAsHex
        movlw   '+'
        call    TransmitAndWait
        movf    dht_sum,0
        call    TransmitAsHex
        movlw   10
        call    TransmitAndWait

        ;; Pause before next time
        clrf    temp1
        movlw   100  ;; about 120ms
        movwf   temp2
        call    delay

        goto    loop

; Transmit a byte in W as two hex digits
TransmitAsHex:
        movwf   temp2
        swapf   temp2,0  ; Get bottom nybble
        andlw   15
        movwf   temp1
        call    TransmitNybble
        movf    temp2,0
        andlw   15
        movwf   temp1
        call    TransmitNybble
        return

; Transmit a nybble in temp1
TransmitNybble
        movlw   10
        subwf   temp1,0  ; C is 0 for -ve, i.e. less than 10
        btfss   STATUS,C
         goto   tnyb_cont
        ; this section skipped for 0..9
        addlw   7        ; i.e. 65 ascii 'A' - 58 the next part is about to do

tnyb_cont
        addlw   58       ; i.e. 48 ascii '0' + 10 we just subtracted

        ; !! Drop through to transmitandwait

TransmitAndWait:
	btfss	PIR1,TXIF
	 goto	$-1
        movwf   TXREG
        return

; I think this is 5cycles for every temp1. ~1280 for a full temp1
delay
	decfsz	temp1,f
	 goto 	$+2       ; i.e. to the 2nd goto; just adds 2 cycles
	decfsz	temp2,f
	 goto   delay
	return

; wait for DHT pin to be 0, times out after 255 cycles, count in temp1
waitDHT0
        clrf    temp1
DHT0loop
        btfss   PORTB,dht_pin   ; 1 cycle+1 if skip?
         goto   DHT0loop_end
        incfsz  temp1,1         ; 1 cycle
         goto   DHT0loop        ; 2 cycles

;  Exit - either because we hit a 0 or because we hit max count
DHT0loop_end
        return

; wait for DHT pin to be 1, times out after 255 cycles, count in temp1
waitDHT1
        clrf    temp1
DHT1loop
        btfsc   PORTB,dht_pin   ; 1 cycle+1 if skip?
         goto   DHT1loop_end
        incfsz  temp1,1         ; 1 cycle
         goto   DHT1loop        ; 2 cycles

;  Exit - either because we hit a 1 or because we hit max count
DHT1loop_end
        return

; Should be called with the input low
; seeing 3 for 0 bit, 13 for 1 bit (on sim), so threshold at 8?
; Result in C, temp1 is lost
DHTbit
        call    waitDHT1   ; Skip the fixed length 0 part
        call    waitDHT0   ; It's the length of this that determines if it's a 1
        movlw   8          ; threshold
        subwf   temp1,0    ; result in C (positive gives C=1 which should be our 1 bit)

        return

; Read a byte from the DHT, result in temp2, temp1 is lost
DHTbyte
        call    DHTbit
        RLF     temp2,1
        call    DHTbit
        RLF     temp2,1
        call    DHTbit
        RLF     temp2,1
        call    DHTbit
        RLF     temp2,1
        call    DHTbit
        RLF     temp2,1
        call    DHTbit
        RLF     temp2,1
        call    DHTbit
        RLF     temp2,1
        call    DHTbit
        RLF     temp2,1
        movf    temp2,0
        return

readdht
	bsf	STATUS,RP0      ;TRIS* is bank 1

        ; set up delay time
        clrf    temp1
        movlw   15      ; about 19ms
        movwf   temp2

	bcf	TRISB,dht_pin	;Make DHT output - gets driven low
        ; We need > 18ms here with it low
        call    delay

	bsf	TRISB,dht_pin	;Make DHT input - pulled up

	bcf	STATUS,RP0     ; back to bank 0- it has timers and port reg

        ; line is currently high, DHT will shortly realise and pull it low for
        ; ~80us, then high for 80us and then send data
        call    waitDHT0
        call    waitDHT1
        call    waitDHT0

        ; and now we should be at the start of the 1st bit of the 1st byte
        call    DHTbyte
        movwf   humid_int
        call    DHTbyte
        movwf   humid_dec
        call    DHTbyte
        movwf   tempe_int
        call    DHTbyte
        movwf   tempe_dec
        call    DHTbyte
        movwf   dht_sum
        return

	end
