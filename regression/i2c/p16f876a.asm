;
; Test reading, writing, dump and load for external i2c EEPROM module
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


variables	UDATA 0x30
temp1           RES     1
temp2           RES     1


STARTUP CODE
	NOP
	goto	start
	NOP
	NOP
	NOP
PROG1 	CODE

;
start	
   .sim "break c 0x100000"
   .sim "module lib libgpsim_modules"
   .sim "module load e24xx024 ee"
   .sim "module load pu pu1"
   .sim "module load pu pu2"
   .sim "node n1"
   .sim "attach n1 portc4 pu1.pin portb4 ee.SDA" ; SDA
   .sim "node n2"
   .sim "attach n2 portc3 pu2.pin portb1 ee.SCL" ; SCL
   .sim "node n3"
   .sim "attach n3 porta0 ee.WP"
   .sim "node n4"
   .sim "attach n4 porta1 ee.A0"
   .sim "node n5"
   .sim "attach n5 porta2 ee.A1"
   .sim "node n6"
   .sim "attach n6 porta3 ee.A2"
   .sim "scope.ch0 = 'portc3'"
   .sim "scope.ch1 = 'portc4'"
   .sim "pu1.xpos = 240."
   .sim "pu1.ypos = 336."
   .sim "pu2.xpos = 216."
   .sim "pu2.ypos = 24."
   .sim "ee.xpos = 48."
   .sim "ee.ypos = 180."
   .sim "p16f876a.xpos = 240."
   .sim "p16f876a.ypos = 84."




	call	ProgInit		; Get everything running step-by-step
        call	I2CSendResult
	call	write_eeprom
	call	is_ready
	call	read_eeprom
  .command "dump e ee dump.hex"
        nop
	call	is_ready
	call	clr_eeprom
  .command "load e ee dump.hex"
	nop
	call	is_ready
	call	read_eeprom

  .assert "'*** PASSED p16f876a I2C test'"
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
	banksel	SSPCON
	movlw	0x08			; Set for I2C master mode
	movwf	SSPCON
	movlw	0x28			; Enable I2C
	movwf	SSPCON
	

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
   .assert  "(pir2 & 0x08) == 0x08, 'FAILED BCLIF for start'"
	nop
	banksel TRISB
	bsf	TRISB,1
	banksel PIR2
	bcf	PIR2,BCLIF

	banksel	SSPCON2			; Generate I2C start
	bsf	SSPCON2,SEN
	IDLE_WAIT_I2C

  .assert "(portc & 0x18) == 0, 'FAILED Start SCL, SDL low'"
	nop
  .assert "(pir1 & 0x08) == 0x08, 'FAILED Start SSPIF set'"
	nop
  .assert "(pir2 & 0x08) == 0x00, 'FAILED Start BCLIF clear'"
	nop
  .assert "(sspstat & 0x3f) == 0x08, 'FAILED Start S bit set'"
	nop

	banksel PIR1
	bcf	PIR1,SSPIF

	banksel	SSPCON2			; Generate I2C restart
	bsf	SSPCON2,RSEN
	btfsc	SSPCON2,RSEN
	goto	$-1	

  .assert "(portc & 0x18) == 0, 'FAILED RStart SCL, SDL low'"
	nop
  .assert "(pir1 & 0x08) == 0x08, 'FAILED RStart SSPIF set'"
	nop
  .assert "(pir2 & 0x08) == 0x00, 'FAILED RStart BCLIF clear'"
	nop
  .assert "(sspstat & 0x3f) == 0x08, 'FAILED RStart S bit set'"
	nop

	banksel PIR1
	bcf	PIR1,SSPIF

	banksel SSPCON2
	bsf	SSPCON2,ACKDT
	bsf	SSPCON2,ACKEN
	btfsc	SSPCON2,ACKEN
	goto	$-1	
  .assert "(portc & 0x18) == 0x10, 'FAILED ACKEN SCL low, SDL high'"
	nop
  .assert "(pir1 & 0x08) == 0x08, 'FAILED ACKEN SSPIF set'"
	nop
  .assert "(pir2 & 0x08) == 0x00, 'FAILED ACKEN BCLIF clear'"
	nop
  .assert "(sspstat & 0x3f) == 0x08, 'FAILED ACKEN S bit set'"
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

  .assert "(portc & 0x18) == 0, 'FAILED RStart SCL, SDL low'"
	nop
  .assert "(pir1 & 0x08) == 0x08, 'FAILED RStart SSPIF set'"
	nop
  .assert "(pir2 & 0x08) == 0x00, 'FAILED RStart BCLIF clear'"
	nop
  .assert "(sspstat & 0x3f) == 0x08, 'FAILED RStart S bit set'"
	nop

	banksel PIR1
	bcf	PIR1,SSPIF

	banksel	SSPCON2			; Generate I2C stop
	bsf	SSPCON2,PEN
	btfsc	SSPCON2,PEN
	goto	$-1	

  .assert "(portc & 0x18) == 0x18, 'FAILED Stop SCL, SDL high'"
	nop
  .assert "(pir1 & 0x08) == 0x08, 'FAILED Stop SSPIF set'"
	nop
  .assert "(pir2 & 0x08) == 0x00, 'FAILED Stop BCLIF clear'"
	nop
  .assert "(sspstat & 0x3f) == 0x10, 'FAILED Stop P bit set'"
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
	movlw	0xa0		; write command to eeprom
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
	movlw	0xa0		; write command to eeprom
	call	I2C_send_w
  .assert "(sspcon2 & 0x40) == 0x00, 'FAILED write command to eeprom ACK'"
	nop
  .assert "(sspstat & 0x01) == 0x00, 'FAILED write to eeprom BF clear'"
	nop

	banksel PIR1
	bcf	PIR1,SSPIF
	movlw	0x00		; write eeprom address
	call	I2C_send_w
  .assert "(sspcon2 & 0x40) == 0x00, 'FAILED write address to eeprom ACK'"
	nop
	return

write_eeprom
	call 	write_eeprom_address
	banksel PIR1
	bcf	PIR1,SSPIF
	movlw	0x80		; write data1
	call	I2C_send_w
  .assert "(sspcon2 & 0x40) == 0x00, 'FAILED write data1 to eeprom ACK'"
	nop

	banksel PIR1
	bcf	PIR1,SSPIF
	movlw	0x81		; write data2
	call	I2C_send_w
  .assert "(sspcon2 & 0x40) == 0x00, 'FAILED write data2 to eeprom ACK'"
	nop
	nop
	call I2C_stop
	return

clr_eeprom
	call 	write_eeprom_address
	banksel PIR1
	bcf	PIR1,SSPIF
	movlw	0x00		; write data1
	call	I2C_send_w
  .assert "(sspcon2 & 0x40) == 0x00, 'FAILED write data1 to eeprom ACK'"
	nop

	banksel PIR1
	bcf	PIR1,SSPIF
	movlw	0x00		; write data2
	call	I2C_send_w
  .assert "(sspcon2 & 0x40) == 0x00, 'FAILED write data2 to eeprom ACK'"
	nop
	nop
	call I2C_stop
	return


read_eeprom

	call 	write_eeprom_address
	banksel	SSPCON2		; Generate I2C repeated start
	bsf	SSPCON2,RSEN
	btfsc	SSPCON2,RSEN
	goto	$-1	
	movlw	0xa1		; Send address/read to eeprom
	banksel SSPBUF
	movwf	SSPBUF
	banksel SSPCON2		; wait for idle (ACKEN,RCEN,PEN,RSEN,SEN) == 0
	movlw	0x1f
	andwf	SSPCON2,W
	BNZ	$-3
	btfsc	SSPSTAT,R_W	; also R_W == 0
	goto	$-1

  .assert "(sspstat & 0x01) == 0x00, 'FAILED address/read to eeprom BF clear'"
	nop

	banksel PIR1
	bcf	PIR1,SSPIF

	banksel	SSPCON2
	bsf	SSPCON2,RCEN	; read data from eeprom
	btfsc	SSPCON2,RCEN
	goto	$-1	
  .assert "(pir1 & 0x08) == 0x08, 'FAILED RCEN SSPIF set'"
	nop
  .assert "(pir2 & 0x08) == 0x00, 'FAILED RCEN BCLIF clear'"
	nop
  .assert "(sspstat & 0x01) == 0x01, 'FAILED RCEN BF set'"
	nop
	banksel SSPBUF
	movf	SSPBUF,W
  .assert "W == 0x80, 'FAILED RCEN, read Data'"
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
	bsf	SSPCON2,RCEN	; read next byte
	btfsc	SSPCON2,RCEN
	goto	$-1	
	banksel SSPBUF
	movf	SSPBUF,W
  .assert "W == 0x81, 'FAILED RCEN, read Data2'"
	nop

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
