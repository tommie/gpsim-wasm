
   ;;  Node test 
   ;;
   ;;  The purpose of this program is to verify that nodes
   ;; can interconnect I/O pins.


	list    p=16f690                ; list directive to define processor
	include <p16f690.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

        __CONFIG _CP_OFF & _WDT_ON &  _INTRC_OSC_NOCLKOUT & _PWRTE_ON &  _MCLRE_OFF


;------------------------------------------------------------------------
; gpsim command
.command macro x
  .direct "C", x
  endm

#define SDO_PORT PORTC,7
#define SDI_PORT PORTB,4
#define SCK_PORT PORTB,6
#define SS_PORT PORTC,6
#define SS_DRIVE PORTA,2

#define SDO_TRIS TRISC,7
#define SDI_TRIS TRISB,4
#define SCK_TRIS TRISB,6
#define SS_TRIS TRISC,6
#define SS_DRIVE_TRI TRISA,2

#define DRV_CLOCK PORTA,1
#define DRV_CLOCK_TRIS TRISA,1
        radix dec

BAUDHI  equ     ((80000/4)/48)-1
;;BAUDLO  equ     129
BAUDLO  equ     103

        radix hex

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA_SHR
temp		RES	1

w_temp		RES	1
status_temp	RES	1
cmif_cnt	RES	1
tmr0_cnt	RES	1
tmr1_cnt	RES	1
eerom_cnt	RES	1
adr_cnt		RES	1
data_cnt	RES	1
adc_cnt		RES	1
rxFlag		RES	1
rxLastByte	RES	1
tx_ptr		RES	1
temp2		RES	1
loopcnt		RES	1




;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlw  high  start               ; load upper byte of 'start' label
        movwf  PCLATH                    ; initialize PCLATH
        goto   start                     ; go to beginning of program

  .sim "module library libgpsim_modules"
   ; Use a pullup resistor as a voltage source
  .sim "module load pullup R1"
  .sim "R1.resistance = 10000.0"
  .sim "R1.voltage = 2.0"
  .sim "R1.xpos = 264"
  .sim "R1.ypos = 36"

   ; Use a pullup resistor as a voltage reference
  .sim "module load pullup R2"
  .sim "R2.resistance = 10000.0"
  .sim "R2.voltage = 2.5"
  .sim "R2.xpos = 252"
  .sim "R2.ypos = 96"

   .sim "module load not not"
   .sim "not.xpos=108."
   .sim "not.ypos=360."

   .sim "module load pu pu2"
   .sim "pu2.xpos = 96"
   .sim "pu2.ypos = 420"


  .sim "node n1"
  .sim "attach n1 porta2 porta3 portc6"
  .sim "node n2"
  .sim "attach n2 porta0 porta5 R1.pin"
  .sim "node n3"
  .sim "attach n3 porta1 porta4 portb6 R2.pin"
  .sim "node n4"

;;   .sim ".frequency=10e6"
   .sim "break c 0x200000"
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


   
   .sim "node sdo"
   .sim "attach sdo not.in0 portc7 pu2.pin"

   .sim "node sdi"
   .sim "attach sdi portb4 not.out"


;------------------------------------------------------------------------
;
;  Interrupt Vector
;
;------------------------------------------------------------------------
                                                                                
INT_VECTOR   CODE    0x004               ; interrupt vector location
                                                                                
        movwf   w_temp
        swapf   STATUS,W
	clrf	STATUS
        movwf   status_temp

        btfsc   PIR2,C1IF	
	   goto	cmif_int

        btfsc   PIR1,RCIF	
	   goto	rcif_int

	btfsc	PIR1,TMR1IF
	   goto tmr1_int

	btfsc	PIR2,EEIF
	    goto ee_int

  	btfsc	PIR1,ADIF
	    goto adc_int

  	btfsc	INTCON,T0IF
	    goto tmr0_int

        btfsc   PIR1,TXIF	
	   goto	txif_int

	.assert "'***FAILED p16f690 unexpected interrupt'"
	nop

; Interrupt from Comparator
cmif_int
	incf	cmif_cnt,F
	bcf	PIR2,C1IF
	goto	exit_int

; Interrupt from USART receive
rcif_int
   .assert "rcreg == txreg, '*** FAILED USART sent character looped back'"
        nop
        movf    RCREG,W
        movwf   rxLastByte
        bsf     rxFlag,0
	goto	exit_int

; Interrupt from USART transmitter
txif_int
	goto exit_int

; Interrupt from TMR0
tmr0_int
	incf	tmr0_cnt,F
	bcf	INTCON,T0IF
	goto	exit_int

; Interrupt from TMR1
tmr1_int
	incf	tmr1_cnt,F
	bcf	PIR1,TMR1IF
	goto	exit_int

; Interrupt from eerom
ee_int
	incf	eerom_cnt,F
	bcf	PIR2,EEIF
	goto	exit_int

; Interrupt from adc
adc_int
	incf	adc_cnt,F
	bcf	PIR1,ADIF
	goto	exit_int
	
exit_int:
                                                                                
        swapf   status_temp,w
        movwf   STATUS
        swapf   w_temp,f
        swapf   w_temp,w
        retfie
                                                                                

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start
;	BANKSEL VRCON
;	bsf	VRCON,VP6EN
;	bsf	VRCON,VRR
;	bcf	VRCON,VRR
;	bsf	VRCON,C1VREN
	;
	.assert "p16f690.frequency == 4000000., 'FALIED 16f690 default frequency'"
	nop
	BANKSEL OSCCON
	clrf	OSCCON		; set 31 kHz
	.assert "(osccon & 6) == 2, 'FALIED 16f690 osccon just after h to l change'"
	nop
	movlw	0x70
	movwf	OSCCON		; set 8 MHz
	.assert "(osccon & 6) == 0, 'FALIED 16f690 osccon just after l to h change'"
	nop
	nop
	nop
	.assert "(osccon & 6) == 4, 'FALIED 16f690 osccon 1us after l to h change'"
	nop
	.assert "p16f690.frequency == 8000000., 'FALIED 16f690 max internal oscilator frequency'"
	nop

	; test pins in analog mode return 0 on register read
	BANKSEL TRISA
	movlw	0xff
	movwf	TRISA
  .assert "trisa == 0x3f, '***FAILED 16f690 trisa 1 writable'"
	nop
	clrf	TRISA
  .assert "trisa == 0x08, '***FAILED 16f690 trisa 0 writable'"
	nop
;	bcf	OPTION_REG,NOT_RABPU ; enable pullups on portA
	BANKSEL PORTA
	movlw	0xff
	movwf	PORTA
   .assert "(porta & 0x17) == 0, '**FAILED 16f690  analog bits read 0'"
	nop
	clrf	PORTA
	movwf	PORTA
	movf	PORTA,W
	BANKSEL ANSEL
	clrf	ANSEL		; turn off analog port selects
	clrf	ANSELH
;
; test PORTA works as expected
;
	BANKSEL PORTA
	clrf	PORTA
	BANKSEL	TRISA
	movlw	0x38
	movwf	TRISA		;PORTA 0,1,2 output 3,4,5 input
	bcf	STATUS,RP0
  .assert "porta == 0x00, '**FAILED 16f690 PORTA = 0x00'"
	nop
	movlw	0x07
	movwf	PORTA		; drive 0,1,2  bits high
  .assert "porta == 0x3f, '**FAILED 16f690 PORTA = 0x3f'"
	nop
	movlw	0x0b
	movwf	PORTA		; drive 0,1,3  bits high
	bsf     STATUS,RP0
	movlw	0x0b
	movwf	TRISA  	; PORTA 2, 4, 5 output 0,1,3 input (3 input only)
        bcf     STATUS,RP0
  .assert "porta == 0x00, '**FAILED 16f690 PORTA = 0x00'"
	nop
	movlw	0x34
	movwf	PORTA		; drive output 2, 4, 5 bits high
  .assert "porta == 0x3f, '**FAILED 16f690 PORTA = 0x33'"
	nop

	call test_compare
	call test_tmr1
	call test_eerom
	call test_adc
	call test_eusart
	call test_ssp

  .assert  "'*** PASSED 16f690 Functionality'"
	goto	$

test_compare:
	BANKSEL TRISA
	movlw	0x33
	movwf	TRISA
	BANKSEL CM1CON0
	movlw	0xff
	movwf   CM2CON1		; test writable bits
	movlw	0x03
	movwf	ANSEL		;Inputs analog
  .assert "cm2con1 == 0x03, '*** FAILED 16f690 cm2con1 writable bits'"
	nop
        clrf	CM2CON1
	bsf	CM1CON0,C1POL	; toggle ouput polarity, not ON
	bsf	CM1CON0,C1OE	; C1OUT t0 RA2
  .assert "cm1con0 == 0x70, '*** FAILED 16f690 cm1con0 off toggle'"
	nop
  .assert "cm2con1 == 0x80, '*** FAILED 16f690 cm2con1 mirror C1OUT'"
	nop

	movlw 	(1<<C1ON)	; Enable Comparator use C1IN+, C12IN0
	movwf	CM1CON0
  .assert "cm1con0 == 0x80, '*** FAILED 16f690 cm1con0 ON=1'"
	nop
	bsf	CM1CON0,C1POL	; toggle ouput polarity
  .assert "cm1con0 == 0xd0, '*** FAILED 16f690 cm1con0 ON=1 POL=1'"
	nop
	bsf	CM2CON0,C1POL	; toggle ouput polarity, not ON
  .assert "cm2con0 == 0x50, '*** FAILED 16f690 cm2con0 ON=0 POL=1'"
	nop
  .assert "cm2con1 == 0xc0, '*** FAILED 16f690 cm2con1 mirror C2OUT'"
	nop
	bsf	CM1CON0,C1OE	; C1OUT t0 RA2
   .assert "(porta & 0x04) == 0x04, '*** FAILED 16f690 compare RA2 not high'"
	nop 
	bcf	CM1CON0,C1POL	; toggle ouput polarity
   .assert "(porta & 0x04) == 0x00, '*** FAILED 16f690 compare RA2 not low'"
	nop
	; Test change in voltage detected
   .command "R1.voltage = 3.0"
	nop
   .assert "(porta & 0x04) == 0x04, '*** FAILED 16f690 compare RA2 not high'"
	nop
	BANKSEL VRCON		
	movlw	0x0f
	movwf	VRCON		; Max voltage
	bsf	VRCON,VP6EN	; turn on 0.6 Volt reference
	BANKSEL CM1CON0
	bsf	CM1CON0,C1R	; C1IN+ is voltage reference
   .assert "(porta & 0x04) == 0x00, '*** FAILED 16f690 compare RA2 not low vref=0.6'"
	nop
	BANKSEL VRCON
	bsf	VRCON,C1VREN	; C1IN+ is CVREF
	
   .assert "(porta & 0x04) == 0x04, '*** FAILED 16f690 compare RA2 not high vref=CVREF'"
	nop
	BANKSEL CM1CON0
	nop
	movf	CM1CON0,W	; FIXME should not be needed
	nop
	clrf	CM1CON0		; turn off C1
	clrf	CM2CON0		; turn off C2
	bsf	CM2CON1,T1GSS	; gate is pin
	return

test_tmr1:
        ;; Here are the tests performed:
        ;;
        ;; -- TMR1L and TMR1H can be read and written
        ;; -- TMR1 driven Fosc/4 with prescale of 8 and generates an interrupt

	BANKSEL ANSEL
	clrf	ANSEL		; turn off analog port selects
	clrf	ANSELH
        ; Load TMR1H, TMR1L with 0x8000 
	BANKSEL TMR1L
        CLRF    TMR1L
        MOVLW   0x80
        MOVWF   TMR1H

        BCF     PIR1,TMR1IF     ;Clear any TMR1 pending interrupt
	BANKSEL PIE1
        BSF     PIE1,TMR1IE     ;Enable TMR1 interrupts
	BANKSEL TMR1L
        BSF     INTCON,PEIE     ;Enable Peripheral interrupts
        BSF     INTCON,GIE      ;Enable Global interrupts

  ; TMR1 not running yet, TMR1H and TMR1L should be unchanged
        MOVF    TMR1L,W         ; test read
   .assert "W==0, '*** FAILED 16f690 TMR1 test TMR1L read'"
        nop
        MOVF    TMR1H,W
   .assert "W==0x80, '*** FAILED 16f690 TMR1 test TMR1H read'"
        nop
;  at 4Mhz prescale of 8 Fosc/4 should interrupt in 0.1 seconds
	movlw	0xcf
	movwf	TMR1H
	movlw	0x2c
	movwf	TMR1L
	MOVLW   (1<<T1CKPS1) | (1<<T1CKPS0) | (1<<TMR1ON)
        MOVWF   T1CON
	CLRWDT
tmr1_loop:
	movf 	tmr1_cnt,W
	btfsc	STATUS,Z
	goto	tmr1_loop
;
;	Test gate enable
;
	BANKSEL TRISA
	movlw	0x38
	movwf	TRISA		;PORTA 0,1,2 output 3,4,5 input
	bcf	OPTION_REG,T0CS ; run TMR0 off Fosc/4
	BANKSEL PORTA
	movlw	(1<<GIE) | (1<<T0IE) | (1<<PEIE)
	movwf	INTCON		; enable T0 interupt
	bsf	PORTA,1		; Pin 4 high
	bsf	T1CON,TMR1GE	; Enable Gate Control
	call	test_tmr1_off
	bcf	PORTA,1		; Pin 4 low tmr1, tmr1 running
	call	test_tmr1_on
	bsf	PORTA,1		; Pin 4 high, stop TMR1
	call	test_tmr1_off
	bcf	T1CON,TMR1GE	; Gate Control off, start TMR1
	call	test_tmr1_on

        sleep

	return

;
; test that tmr1 is stopped
;
test_tmr1_off:
	clrwdt
	movlw	0xc0
	movwf	TMR0
	movf	TMR1L,W		; Hold TMR1l 
	clrf	tmr0_cnt
tmr1_loop2:
	btfss	tmr0_cnt,0
	goto	tmr1_loop2

   .assert "tmr1l == W, '*** FAILED 16f690 TMR1 test - TMR1 stop'"
	nop
	return

;
; test that tmr1 is running
;
test_tmr1_on:
	clrwdt
	movlw	0xc0
	movwf	TMR0
	movf	TMR1L,W
	clrf	tmr0_cnt
tmr1_loop3:
	btfss	tmr0_cnt,0
	goto	tmr1_loop3

   .assert "tmr1l != W, '*** FAILED 16f690 TMR1 test - TMR1 running'"
	nop
	return

test_eerom:
  ;
  ;	test can write and read to all 128 eeprom locations
  ;	using intterupts
        clrf    adr_cnt
        clrf    data_cnt
;  setup interrupts
        bsf     INTCON,PEIE
        bsf     INTCON,GIE
	BANKSEL PIE2
	bsf	PIE2,EEIE
	BANKSEL	PIR2
;
;	write to EEPROM starting at EEPROM address 0
;	value of address as data using interrupts to
;	determine write complete. 
;	read and verify data

l1:     
	bcf	PIR2,EEIF
        movf    adr_cnt,W
	clrf	eerom_cnt
	BANKSEL	EEADR
        movwf   EEADR 
        movf    data_cnt,W
        movwf   EEDATA

	BANKSEL EECON1
        bcf     INTCON,GIE      ;Disable interrupts while enabling write
        bcf 	EECON1, EEPGD 	;Point to DATA memory
        bsf     EECON1,WREN    	;Enable eeprom writes

        movlw   0x55            ;Magic sequence to enable eeprom write
        movwf   EECON2
        movlw   0xaa
        movwf   EECON2

        bsf     EECON1,WR      ;Begin eeprom write

        bsf     INTCON,GIE      ;Re-enable interrupts
        
	BANKSEL	PIR1
        clrf    STATUS          ; Bank 0
        movf   eerom_cnt,W
	skpnz
        goto   $-2
;
;	read what we just wrote
;
	
        movf    adr_cnt,W

	BANKSEL	EEADR
	movwf   EEADR
	BANKSEL EECON1
	bsf	EECON1,RD	; start read operation
	BANKSEL EEDATA
	movf	EEDATA,W	; Read data
	BANKSEL	PIR1

	xorwf	data_cnt,W	; did we read what we wrote ?
	skpz
	goto eefail

        incf    adr_cnt,W
        andlw   0x7f
        movwf   adr_cnt
	movwf	data_cnt

        skpz
         goto   l1

	return

eefail:
  .assert "'***FAILED 16f690 eerom write/read error'"
	nop

test_tmr0:
	return
	
test_adc:

  ; ADCS = 110 Fosc/64
  ; ANS = 1 AN0 is only analog port
   .command "R1.voltage = 2.0"
   .command "R2.voltage = 4.0"
	BANKSEL ANSEL
	movlw	(1 << ANS0)
	movwf	ANSEL
	BANKSEL ADCON1
	movlw	(1 << ADCS2) | (1 << ADCS1) 
	movwf	ADCON1
	movlw	0x3f
	movwf	TRISA		; All pins input
	bsf     PIE1,ADIE
	bcf	WPUA,5		; turn off input pullup on porta5
	bcf	WPUA,4		; turn off input pullup on porta4
	BANKSEL ADCON0
	bsf	ADCON0,ADON	; enable ADC

        bsf     INTCON,GIE      ;Global interrupts
        bsf     INTCON,PEIE     ;Peripheral interrupts
        call    Convert
   .assert "W == 0x66, '*** FAILED 16f690 ADC test - AN0 = 2 Vref = Vddi ADFM=0'"
	nop

	BANKSEL ANSEL
	bsf	ANSEL,ANS1	; AN1 is analog
	BANKSEL ADCON0
	bsf	ADCON0,VCFG	; AN1 as hi Vref 
        call    Convert
   .assert "W == 0x80, '*** FAILED 16f690 ADC test - AN0 = 2 Vref = AN1 = 4 ADFM=0'"
	nop
	bsf	ADCON0,ADFM	; AN1 as hi Vref , ADFM=1
        call    Convert
   .assert "W == 0x02, '*** FAILED 16f690 ADC test - AN0 = 2 Vref = AN1 = 4 ADFM=0'"
	nop
	bsf	ADCON0,CHS1	; select channel 2 which is not ADC analog pin
	call	Convert

	movlw	0x31		; select ch 12 (CREF)
	movwf   ADCON0
	call	Convert
   .assert "W == 0xB7, '*** FAILED 16f690 ADC test - chan=12 (CVREF 3.59v) Vref = Vddi ADFM=0'"
	nop
	BANKSEL VRCON
	bcf	VRCON,C1VREN	; turn off CVREF
	BANKSEL ADCON0
	call	Convert
   .assert "W == 0x00, '*** FAILED 16f690 ADC test - chan=12 (CVREF off) Vref = Vddi ADFM=0'"
	nop

	movlw	0x35		; select ch 13 (0.6V)
	movwf   ADCON0
	call	Convert
   .assert "W == 0x1e, '*** FAILED 16f690 ADC test - chan=13 (0.6v) Vref = Vddi ADFM=0'"
	nop

	BANKSEL ANSEL
	clrf	ANSEL		; return ports to digital I/O
	BANKSEL ADCON0
	clrf	ADCON0

	return

Convert:

        clrf    adc_cnt              ;flag set by the interrupt routine

        bsf     ADCON0,GO       ;Start the A/D conversion

        btfss   adc_cnt,0            ;Wait for the interrupt to set the flag
         goto   $-1

	BANKSEL	ADRESH
        movf    ADRESH,W                ;Read the high 8-bits of the result

        return

	
test_eusart:
	;; USART Initialization
	;;
	;; Turn on the high baud rate (BRGH), disable the transmitter,
	;; disable synchronous mode.
	;;
	
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


  .assert "(portb & 0x80) == 0x80, '*** FAILED: TX bit initilized as high'"
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
;  At 4800 baud each bit takes 0.208 msec. (1/4800)
;  TRMT will be low > 9 bits and < 10 bits or between 1.875 and 2.083 msec.
;  with oscillator at 8MHz and TMR0 / 64 expect between 58 and 65
;  TMR0 cycles. (time * 8000000) / (4 * 64)

	BANKSEL TMR0
	movf	TMR0,W

  .assert "tmr0 > 58 && tmr0 < 65, '*** FAILED baud rate'"
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
;  start + 12 bit times or 2.70 msec. With 8Mhz TMR0 / 64 is 84 TMR0 counts.
;  8Mhz * 0.00208 / (4 * 64)

	movf	TMR0,W

  .assert "tmr0 > 80 && tmr0 < 88, '*** FAILED sync pulse'"
	nop

	return



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

test_ssp:
    ;banksel	WDTCON
    ;movlw	0x16
    ;movwf	WDTCON
    clrwdt
    banksel	ANSEL
    clrf	ANSEL
    clrf	ANSELH
    banksel	TRISA
    bcf		SDO_TRIS	; SDO
    bcf		SCK_TRIS	; SCK
    bcf		OPTION_REG,NOT_RABPU
    movlw	0xff
    movwf	SSPSTAT
  .assert "sspstat == 0xc0, 'SPI BSSP sspstat not writable'"
    nop
    clrf	SSPSTAT
    bcf  STATUS,RP0	; bank 0


;
;  	Test SPI Master mode
;
    movlw	0x21	; SSPEN | SPI master Fosc/16
    movwf	SSPCON
    bcf		PIR1,SSPIF
    movlw	0xab
    movwf	SSPBUF

    btfss	PIR1,SSPIF
    goto	$-1

  .assert "(sspstat & 1) == 1, '*** FAILED BSSP SPI Master BF not set'"
    nop
    movf	SSPBUF,W
  .assert "(sspstat & 1) == 0, '*** FAILED BSSP SPI Master BF not cleared'"
    nop
  .assert "W == 0x54, '*** FAILED BSSP SPI Master wrong data'"
    nop

;
;	TEST SPI Slave mode with SS
;
    clrf	SSPCON
    bsf  STATUS,RP0	; bank 1
    bcf		DRV_CLOCK_TRIS 	; external SCK drive
    bsf		SCK_TRIS	; SCK
    bsf		SS_TRIS 	; SS
    bcf		SS_DRIVE_TRI	; SS drive output
    bcf  STATUS,RP0	; bank 0
    bcf		SS_DRIVE	; drive SS low
    bcf		DRV_CLOCK
    movlw	0x24	; SSPEN | SPI slave mode SS enable
    movwf	SSPCON
    bcf		PIR1,SSPIF
    movlw	0xab
    movwf	SSPBUF
    bsf		DRV_CLOCK
    bcf		DRV_CLOCK
    movwf	SSPBUF	; test WCOL set
  .assert "(sspcon & 0x80) == 0x80, '*** FAILED BSSP SPI WCOL set'"
    nop
    bcf		SSPCON,WCOL	; clear WCOL bit
  .assert "(sspcon & 0x80) == 0x00, '*** FAILED BSSP SPI WCOL was cleared'"
    nop
    clrf	loopcnt
loop2:
    incf	loopcnt,F
    bsf		DRV_CLOCK
    bcf		DRV_CLOCK
    btfss	PIR1,SSPIF
    goto	loop2

    movf	SSPBUF,W
  .assert "W == 0x54, '*** FAILED BSSP SPI Slave data'"
    nop
;
;	Test Slave receive overrun
;
   movlw	0x10
   movwf	loopcnt
loop4:
    bsf		DRV_CLOCK
    bcf		DRV_CLOCK
    decfsz	loopcnt,F
    goto	loop4
  .assert "(sspcon & 0x40) == 0x40, '*** FAILED BSSP SPI SSPOV'"
    nop

;
;  	Test SPI Master mode TMR2
;
    clrf	SSPCON
    bsf  STATUS,RP0	; bank 1
    bsf		DRV_CLOCK_TRIS	; external SCK drive off
    bcf		SCK_TRIS	; SCK output
    movlw	0x1
    movwf	PR2
    bcf  STATUS,RP0	; bank 0
    clrf	TMR2
    movlw	0x3C	; prescale = 1 postscale 16
    movwf	T2CON

    movlw	0x23	; SSPEN | SPI master TMR2
    movwf	SSPCON
    bcf		PIR1,SSPIF
    movlw	0xab
    movwf	SSPBUF

loop3:
    btfss	PIR1,SSPIF
    goto	loop3

  .assert "(sspstat & 1) == 1, '*** FAILED BSSP SPI Master TMR2, BF not set'"
    nop
    movf	SSPBUF,W
  .assert "(sspstat & 1) == 0, '*** FAILED BSSP SPI Master TMR2, BF not cleared'"
    nop
  .assert "W == 0x54, '*** FAILED BSSP SPI Master TMR2 wrong data'"
    nop

    return

  end
