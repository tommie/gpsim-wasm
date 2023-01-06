;
; Test reading, writing and dump for external i2c DS1307 module
; 
; Copyright (c) 2007 Roy Rankin
; MACROS and some code included the following
;************************************************************************
;*	Microchip Technology Inc. 2006
;*	02/15/06
;*	Designed to run at 20MHz
;************************************************************************
; gpsim command
.command macro x
  .direct "C", x
  endm



	list		p=16F876A
	#include	p16f876a.inc
        include <coff.inc>


	__CONFIG _CP_OFF & _DEBUG_OFF & _WRT_OFF & _CPD_OFF  & _LVP_OFF  & _PWRTE_ON & _BODEN_ON & _WDT_OFF & _HS_OSC

	errorlevel	-302
        radix dec


variables	UDATA 0x30
time		RES	16
temp1           RES     1
temp2           RES     1
status_temp	RES	1
w_temp		RES	1

  global time
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

        movwf   w_temp
        swapf   STATUS,W
        movwf   status_temp
        btfsc   INTCON,RBIF
            goto rb_int

        .assert "\"***FAILED ds1307 unexpected interrupt\""
        nop
rb_int
	nop
	bcf	INTCON,RBIF
exit_int:

        swapf   status_temp,w
        movwf   STATUS
        swapf   w_temp,f
        swapf   w_temp,w
        retfie



;
MAIN	CODE
start	
   .sim "module lib libgpsim_modules"
   .sim "module lib libgpsim_extras"
   .sim "module load ds1307 ee"
   .sim "module load pu pu1"
   .sim "module load pu pu2"
   .sim "module load pu pu3"
   .sim "node n1"
   .sim "attach n1 portc4 pu1.pin portb4 ee.SDA" ; SDA
   .sim "node n2"
   .sim "attach n2 portc3 pu2.pin portb1 ee.SCL" ; SCL
   .sim "node n3"
   .sim "attach n3 pu3.pin portb0 ee.SQW"
   .sim "pu1.xpos = 240."
   .sim "pu1.ypos = 336."
   .sim "pu2.xpos = 216."
   .sim "pu2.ypos = 24."
   .sim "ee.xpos = 48."
   .sim "ee.ypos = 180."
   .sim "p16f876a.xpos = 240."
   .sim "p16f876a.ypos = 84."
   .sim "scope.ch0 = 'portc3'"
   .sim "scope.ch1 = 'portc4'"




	call	ProgInit		; Get everything running step-by-step
	BANKSEL PORTA
 	movlw	0x2F
	movwf	time+2
	movwf	time+5
	movlw	0x20
	movwf	time+8
 	movlw	0x3A
	movwf	time+0xb
	movwf	time+0xe
	nop
	call	is_ready
	call	read_ds1307
; Current date and time startting in ram location 30
;	sleep
	nop
        call	I2CSendResult
	call	write_eeprom
	call	is_ready
	call	read_ds1307
; 02/05/10 19:59:12 starting in ram location 30
   .assert "time == 0x30, \"*** FAILED ds1307 day 10's\""
        nop
	BANKSEL time
	movf	time+1,W
   .assert "W == 0x33, \"*** FAILED ds1307 day 1's\""
        nop
	movf	time+3,W
   .assert "W == 0x30, \"*** FAILED ds1307 month 10's\""
        nop
	movf	time+4,W
   .assert "W == 0x35, \"*** FAILED ds1307 month 1's\""
        nop
	movf	time+6,W
   .assert "W == 0x31, \"*** FAILED ds1307 year 10's\""
        nop
	movf	time+7,W
   .assert "W == 0x30, \"*** FAILED ds1307 year 1's\""
        nop
	movf	time+9,W
   .assert "W == 0x31, \"*** FAILED ds1307 hour 10's\""
        nop
	movf	time+10,W
   .assert "W == 0x39, \"*** FAILED ds1307 hour 1's\""
        nop
	movf	time+12,W
   .assert "W == 0x35, \"*** FAILED ds1307 min 10's\""
        nop
	movf	time+13,W
   .assert "W == 0x39, \"*** FAILED ds1307 min 1's\""
        nop
	movf	time+15,W
   .assert "W == 0x31, \"*** FAILED ds1307 sec 10's\""
        nop
	movf	time+16,W
   .assert "W == 0x32, \"*** FAILED ds1307 sec 1's\""
        nop
	sleep		; SQW causes wake
	nop

  .command "dump e ee dump.hex"
        nop

	call	is_ready
	call	read_ds1307
	BANKSEL time
	movf	time+16,W
   .assert "W == 0x33, \"*** FAILED ds1307 sec 1's after 1 sec\""
        nop

  .assert "\"*** PASSED p16f876a I2C DS1307 test\""
	goto $






;****************************** SUBROUTINES  ****************************
;************************************************************************

START_I2C	MACRO
	banksel	SSPCON2			; Generate I2C start
	bsf	SSPCON2,SEN
	btfsc	SSPCON2,SEN
	goto	$-1	
	banksel PORTA
	ENDM

RSTART_I2C	MACRO
	banksel	SSPCON2			; Generate I2C repeat start
	bsf	SSPCON2,RSEN
	btfsc	SSPCON2,RSEN
	goto	$-1	
	banksel PORTA
	ENDM

STOP_I2C	MACRO
	banksel	SSPCON2			; Generate I2C stop
	bsf	SSPCON2,PEN
	btfsc	SSPCON2,PEN
	goto	$-1	
	banksel PORTA
	ENDM

IDLE_WAIT_I2C	MACRO
	banksel SSPCON2
	movlw	0x1f
	andwf	SSPCON2,W
	BNZ	$-3
	btfsc	SSPSTAT,R_W
	goto	$-1
	banksel PORTA
	ENDM




;****************** Initialize Registers and Variables  *****************
;************************************************************************
ProgInit

	banksel	PORTA
	clrf	PORTA			; Set all bits to zero on Port A
	banksel TRISA
	clrf	TRISA
	


	banksel	SSPADD
	movlw	0x0C			; Set I2C baud rate to 385 kHz
	movwf	SSPADD
	bcf	OPTION_REG,INTEDG
	banksel	SSPCON
	movlw	0x08			; Set for I2C master mode
	movwf	SSPCON
	movlw	0x28			; Enable I2C
	movwf	SSPCON
        bsf	INTCON,INTE

	return





;************************************************************************
I2CSendResult
	banksel	SSPCON2			; Generate I2C start, bus collision
	bsf	SSPCON2,PEN
	btfsc	SSPCON2,PEN
	goto	$-1	

	bsf	SSPCON2,SEN
	bcf	TRISB,1
	IDLE_WAIT_I2C
   .assert  "(pir2 & 0x08) == 0x08, \"FAILED BCLIF for start\""
	nop
	banksel TRISB
	bsf	TRISB,1
	banksel PIR2
	bcf	PIR2,BCLIF

	banksel	SSPCON2			; Generate I2C start
	bsf	SSPCON2,SEN
	IDLE_WAIT_I2C

  .assert "(portc & 0x18) == 0, \"FAILED Start SCL, SDL low\""
	nop
  .assert "(pir1 & 0x08) == 0x08, \"FAILED Start SSPIF set\""
	nop
  .assert "(pir2 & 0x08) == 0x00, \"FAILED Start BCLIF clear\""
	nop
  .assert "(sspstat & 0x3f) == 0x08, \"FAILED Start S bit set\""
	nop

	banksel PIR1
	bcf	PIR1,SSPIF

	banksel	SSPCON2			; Generate I2C restart
	bsf	SSPCON2,RSEN
	btfsc	SSPCON2,RSEN
	goto	$-1	

  .assert "(portc & 0x18) == 0, \"FAILED RStart SCL, SDL low\""
	nop
  .assert "(pir1 & 0x08) == 0x08, \"FAILED RStart SSPIF set\""
	nop
  .assert "(pir2 & 0x08) == 0x00, \"FAILED RStart BCLIF clear\""
	nop
  .assert "(sspstat & 0x3f) == 0x08, \"FAILED RStart S bit set\""
	nop

	banksel PIR1
	bcf	PIR1,SSPIF

	banksel SSPCON2
	bsf	SSPCON2,ACKDT
	bsf	SSPCON2,ACKEN
	btfsc	SSPCON2,ACKEN
	goto	$-1	
  .assert "(portc & 0x18) == 0x10, \"FAILED ACKEN SCL low, SDL high\""
	nop
  .assert "(pir1 & 0x08) == 0x08, \"FAILED ACKEN SSPIF set\""
	nop
  .assert "(pir2 & 0x08) == 0x00, \"FAILED ACKEN BCLIF clear\""
	nop
  .assert "(sspstat & 0x3f) == 0x08, \"FAILED ACKEN S bit set\""
	nop
	bcf	SSPCON2,ACKDT

	bsf	SSPCON2,PEN
	btfsc	SSPCON2,PEN
	goto	$-1	

	banksel PIR1
	bcf	PIR1,SSPIF

	banksel	SSPCON2			; Generate I2C restart
	bsf	SSPCON2,RSEN
	btfsc	SSPCON2,RSEN
	goto	$-1	

  .assert "(portc & 0x18) == 0, \"FAILED RStart SCL, SDL low\""
	nop
  .assert "(pir1 & 0x08) == 0x08, \"FAILED RStart SSPIF set\""
	nop
  .assert "(pir2 & 0x08) == 0x00, \"FAILED RStart BCLIF clear\""
	nop
  .assert "(sspstat & 0x3f) == 0x08, \"FAILED RStart S bit set\""
	nop

	banksel PIR1
	bcf	PIR1,SSPIF

	banksel	SSPCON2			; Generate I2C stop
	bsf	SSPCON2,PEN
	btfsc	SSPCON2,PEN
	goto	$-1	

  .assert "(portc & 0x18) == 0x18, \"FAILED Stop SCL, SDL high\""
	nop
  .assert "(pir1 & 0x08) == 0x08, \"FAILED Stop SSPIF set\""
	nop
  .assert "(pir2 & 0x08) == 0x00, \"FAILED Stop BCLIF clear\""
	nop
  .assert "(sspstat & 0x3f) == 0x10, \"FAILED Stop P bit set\""
	nop
	return
;
;	repeatedly send command to eeprom until an ACK
;	is received back
;
;	The call of delay is not required for operation of the code,
;	but it speeds up the simulation with the GUI running. -- RRR
;
is_ready
	banksel SSPCON
	movlw	0x04
	movwf	temp2
	clrf	temp1
	call delay
	banksel	SSPCON2		; Generate I2C start
	bsf	SSPCON2,SEN
	btfsc	SSPCON2,SEN
	goto	$-1	
	movlw	0xd0		; write command to eeprom
	call	I2C_send_w
	call I2C_stop
	banksel	SSPCON2		; Generate I2C start
	btfsc	SSPCON2,ACKSTAT
	goto	is_ready
	banksel PIR2
	bcf	PIR2,BCLIF
	return
	
write_eeprom_address
	banksel	SSPCON2		; Generate I2C start
	bsf	SSPCON2,SEN
	btfsc	SSPCON2,SEN
	goto	$-1	
	movlw	0xd0		; write command to eeprom
	call	I2C_send_w
  .assert "(sspcon2 & 0x40) == 0x00, \"FAILED write command to eeprom ACK\""
	nop
  .assert "(sspstat & 0x01) == 0x00, \"FAILED write to eeprom BF clear\""
	nop

	banksel PIR1
	bcf	PIR1,SSPIF
	movlw	0x00		; write eeprom address
	call	I2C_send_w
  .assert "(sspcon2 & 0x40) == 0x00, \"FAILED write address to eeprom ACK\""
	nop
	return

write_eeprom
	call 	write_eeprom_address
	banksel PIR1
	bcf	PIR1,SSPIF
	movlw	0x12		; write seconds 12
	call	I2C_send_w
  .assert "(sspcon2 & 0x40) == 0x00, \"FAILED write data1 to eeprom ACK\""
	nop

	banksel PIR1
	bcf	PIR1,SSPIF
	movlw	0x59		; write minutes 59
	call	I2C_send_w
  .assert "(sspcon2 & 0x40) == 0x00, \"FAILED write data2 to eeprom ACK\""
	nop
	banksel PIR1
	bcf	PIR1,SSPIF
	movlw	0x19|0x40	; write hours 19 and 24 hour format
	call	I2C_send_w
	nop
	banksel PIR1
	bcf	PIR1,SSPIF
	movlw	0x01		; write Day of week (1-7)
	call	I2C_send_w
	nop
	banksel PIR1
	bcf	PIR1,SSPIF
	movlw	0x03		; write Day of month
	call	I2C_send_w
	nop
	banksel PIR1
	bcf	PIR1,SSPIF
	movlw	0x05		; write month
	call	I2C_send_w
	nop
	banksel PIR1
	bcf	PIR1,SSPIF
	movlw	0x10		; write year
	call	I2C_send_w
	nop
	banksel PIR1
	bcf	PIR1,SSPIF
	movlw	0x10		; control out=1 SQWE off
	call	I2C_send_w
	nop
	call I2C_stop
	return

clr_eeprom
	call 	write_eeprom_address
	banksel PIR1
	bcf	PIR1,SSPIF
	movlw	0x00		; write data1
	call	I2C_send_w
  .assert "(sspcon2 & 0x40) == 0x00, \"FAILED write data1 to eeprom ACK\""
	nop

	banksel PIR1
	bcf	PIR1,SSPIF
	movlw	0x00		; write data2
	call	I2C_send_w
  .assert "(sspcon2 & 0x40) == 0x00, \"FAILED write data2 to eeprom ACK\""
	nop
	nop
	call I2C_stop
	return


read_ds1307

	call 	write_eeprom_address
	banksel	SSPCON2		; Generate I2C repeated start
	bsf	SSPCON2,RSEN
	btfsc	SSPCON2,RSEN
	goto	$-1	
	movlw	0xd1		; Send address/read to eeprom
	banksel SSPBUF
	movwf	SSPBUF
	banksel SSPCON2		; wait for idle (ACKEN,RCEN,PEN,RSEN,SEN) == 0
	movlw	0x1f
	andwf	SSPCON2,W
	BNZ	$-3
	btfsc	SSPSTAT,R_W	; also R_W == 0
	goto	$-1

  .assert "(sspstat & 0x01) == 0x00, \"FAILED address/read to eeprom BF clear\""
	nop

	banksel PIR1
	bcf	PIR1,SSPIF

	banksel	SSPCON2
	bsf	SSPCON2,RCEN	; read seconds from RTC
	btfsc	SSPCON2,RCEN
	goto	$-1	
  .assert "(pir1 & 0x08) == 0x08, \"FAILED RCEN SSPIF set\""
	nop
  .assert "(pir2 & 0x08) == 0x00, \"FAILED RCEN BCLIF clear\""
	nop
  .assert "(sspstat & 0x01) == 0x01, \"FAILED RCEN BF set\""
	nop
	banksel SSPBUF
	movf	SSPBUF,W
	banksel PORTA
	movwf	time+0x10
	movwf	time+0xf
	movlw	0x0f
	andwf	time+0x10,f
	movlw	0x30
	addwf	time+0x10,f
 	movlw	0x70
	andwf	time+0xf,f
	RRF	time+0xf,f
	RRF	time+0xf,f
	RRF	time+0xf,f
	RRF	time+0xf,f
	movlw	0x30
	addwf	time+0xf,f
	nop
  
	banksel PIR1
	bcf	PIR1,SSPIF

	banksel	SSPCON2
	bcf	SSPCON2,ACKDT	; send ACK
	bsf	SSPCON2,ACKEN
	btfsc	SSPCON2,ACKEN
	goto	$-1	

	banksel PIR1
	bcf	PIR1,SSPIF

	banksel	SSPCON2
	bsf	SSPCON2,RCEN	; read minutes from RTC
	btfsc	SSPCON2,RCEN
	goto	$-1	
	banksel SSPBUF
	movf	SSPBUF,W
	banksel PORTA
	movwf	time+0xd
	movwf	time+0xc
	movlw	0x0f
	andwf	time+0xd,f
	movlw	0x30
	addwf	time+0xd,f
 	movlw	0xf0
	andwf	time+0xc,f
	RRF	time+0xc,f
	RRF	time+0xc,f
	RRF	time+0xc,f
	RRF	time+0xc,f
	movlw	0x30
	addwf	time+0xc,f
	nop

	banksel PIR1
	bcf	PIR1,SSPIF

	banksel	SSPCON2
	bcf	SSPCON2,ACKDT	; send ACK
	bsf	SSPCON2,ACKEN
	btfsc	SSPCON2,ACKEN
	goto	$-1	

	banksel PIR1
	bcf	PIR1,SSPIF

	banksel	SSPCON2
	bsf	SSPCON2,RCEN	; read Hours from RTC
	btfsc	SSPCON2,RCEN
	goto	$-1	
	banksel SSPBUF
	movf	SSPBUF,W

	banksel PORTA
	movwf	time+0xa
	movwf	time+0x9
	movlw	0x0f
	andwf	time+0xa,f
	movlw	0x30
	addwf	time+0xa,f
 	movlw	0x30
	andwf	time+0x9,f
	RRF	time+0x9,f
	RRF	time+0x9,f
	RRF	time+0x9,f
	RRF	time+0x9,f
	movlw	0x30
	addwf	time+0x9,f
	nop

	banksel	SSPCON2
	bcf	SSPCON2,ACKDT	; send ACK
	bsf	SSPCON2,ACKEN
	btfsc	SSPCON2,ACKEN
	goto	$-1	

	banksel PIR1
	bcf	PIR1,SSPIF

	banksel	SSPCON2
	bsf	SSPCON2,RCEN	; read day of week
	btfsc	SSPCON2,RCEN
	goto	$-1	
	banksel SSPBUF
	movf	SSPBUF,W


	banksel	SSPCON2
	bcf	SSPCON2,ACKDT	; send ACK
	bsf	SSPCON2,ACKEN
	btfsc	SSPCON2,ACKEN
	goto	$-1	

	banksel PIR1
	bcf	PIR1,SSPIF

	banksel	SSPCON2
	bsf	SSPCON2,RCEN	; read Day of month
	btfsc	SSPCON2,RCEN
	goto	$-1	
	banksel SSPBUF
	movf	SSPBUF,W

	banksel PORTA
	movwf	time+0x1
	movwf	time+0x0
	movlw	0x0f
	andwf	time+0x1,f
	movlw	0x30
	addwf	time+0x1,f
 	movlw	0x30
	andwf	time+0x0,f
	RRF	time+0x0,f
	RRF	time+0x0,f
	RRF	time+0x0,f
	RRF	time+0x0,f
	movlw	0x30
	addwf	time+0x0,f
	nop

	banksel	SSPCON2
	bcf	SSPCON2,ACKDT	; send ACK
	bsf	SSPCON2,ACKEN
	btfsc	SSPCON2,ACKEN
	goto	$-1	

	banksel PIR1
	bcf	PIR1,SSPIF

	banksel	SSPCON2
	bsf	SSPCON2,RCEN	; read month
	btfsc	SSPCON2,RCEN
	goto	$-1	
	banksel SSPBUF
	movf	SSPBUF,W

	banksel PORTA
	movwf	time+0x4
	movwf	time+0x3
	movlw	0x0f
	andwf	time+0x4,f
	movlw	0x30
	addwf	time+0x4,f
 	movlw	0x30
	andwf	time+0x3,f
	RRF	time+0x3,f
	RRF	time+0x3,f
	RRF	time+0x3,f
	RRF	time+0x3,f
	movlw	0x30
	addwf	time+0x3,f
	nop

	banksel	SSPCON2
	bcf	SSPCON2,ACKDT	; send ACK
	bsf	SSPCON2,ACKEN
	btfsc	SSPCON2,ACKEN
	goto	$-1	

	banksel PIR1
	bcf	PIR1,SSPIF

	banksel	SSPCON2
	bsf	SSPCON2,RCEN	; read Year
	btfsc	SSPCON2,RCEN
	goto	$-1	
	banksel SSPBUF
	movf	SSPBUF,W

	banksel PORTA
	movwf	time+0x7
	movwf	time+0x6
	movlw	0x0f
	andwf	time+0x7,f
	movlw	0x30
	addwf	time+0x7,f
 	movlw	0xf0
	andwf	time+0x6,f
	RRF	time+0x6,f
	RRF	time+0x6,f
	RRF	time+0x6,f
	RRF	time+0x6,f
	movlw	0x30
	addwf	time+0x6,f
	nop

	banksel PIR1
	bcf	PIR1,SSPIF

	banksel	SSPCON2
	bsf	SSPCON2,ACKDT	; send NACK
	bsf	SSPCON2,ACKEN
	btfsc	SSPCON2,ACKEN
	goto	$-1	

	return


I2C_stop
	banksel	SSPCON2			; Generate I2C stop
	bsf	SSPCON2,PEN
	btfsc	SSPCON2,PEN
	goto	$-1	

	return
	
delay
        decfsz  temp1,f
         goto   $+2
        decfsz  temp2,f
         goto   delay
        return

;********************** Output byte in W via I2C bus ********************
;************************************************************************
I2C_send_w
	banksel	SSPBUF			; Second byte of data (middle)
	movwf	SSPBUF
	banksel	SSPSTAT
	btfsc	SSPSTAT,R_W
	goto	$-1

	return
	




	end	
