;
; 
; Copyright (c) 2013 Roy Rankin
; MACROS and some code included the following
;************************************************************************
;*	Microchip Technology Inc. 2006
;*	02/15/06
;*	Designed to run at 20MHz
;************************************************************************

	list	p=16f1823
        include <p16f1823.inc>
        include <coff.inc>

	__CONFIG _CONFIG1, _CP_OFF & _WDTE_ON &  _FOSC_INTOSC & _PWRTE_ON &  _BOREN_OFF & _MCLRE_OFF & _CLKOUTEN_OFF
	__CONFIG _CONFIG2, _PLLEN_OFF 


	;; The purpose of this program is to test gpsim's ability 
	;; to simulate a pic 16F1823.
	;; Specifically, slave modes of I2C

        errorlevel -302 

; Printf Command
.command macro x
  .direct "C", x
  endm

_ClkIn          equ     8000000        ; Input Clock Frequency

_ClkOut         equ     (_ClkIn >> 2)

;
; Compute the delay constants for setup & hold times
;
_40uS_Delay     set     (_ClkOut/250000)
_47uS_Delay     set     (_ClkOut/212766)
_50uS_Delay     set     (_ClkOut/200000)


TRUE    equ     1
FALSE   equ     0

LSB     equ     0
MSB     equ     7

; These parameters are for the Software master
#define SCL_PIN	0
#define SDA_PIN 1
#define I2CPORT	PORTA
#define I2CTRI  TRISA
#define _SCL_D	I2CPORT,SCL_PIN
#define _SCL	I2CTRI,SCL_PIN
#define _SDA	I2CTRI,SDA_PIN
#define _SDA_D	I2CPORT,SDA_PIN

#define T0IE	TMR0IE
#define T0IF	TMR0IF

#define _ENABLE_BUS_FREE_TIME   TRUE
#define _CLOCK_STRETCH_CHECK    TRUE
#define _OPTION_INIT (0xC0 | 0x02) ; Prescaler to TMR0 for Appox 1 mSec timeout

;*****************************************************************************
;                       I2C Bus Status Reg Bit Definitions
;*****************************************************************************

#define _Bus_Busy       Bus_Status,0
#define _Abort          Bus_Status,1
#define _Txmt_Progress  Bus_Status,2
#define _Rcv_Progress   Bus_Status,3

#define _Txmt_Success   Bus_Status,4
#define _Rcv_Success    Bus_Status,5
#define _Fatal_Error    Bus_Status,6
#define _ACK_Error      Bus_Status,7

;*****************************************************************************
;                       I2C Bus Contro Register
;*****************************************************************************
#define _10BitAddr      Bus_Control,0
#define _Slave_RW       Bus_Control,1
#define _Last_Byte_Rcv  Bus_Control,2

#define _SlaveActive    Bus_Control,6
#define _TIME_OUT_      Bus_Control,7



RELEASE_BUS     MACRO
			banksel TRISA
                        bsf     _SDA            ; tristate SDA
                        bsf     _SCL            ; tristate SCL
;                       bcf     _Bus_Busy       ; Bus Not Busy, TEMP ????, set/clear on Start & Stop
                ENDM


;****************************************************************************
;  A MACRO To Load 8 OR 10 Bit Address To The Address Registers
;
;  SLAVE_ADDRESS is a constant and is loaded into the SlaveAddress Register(s)
;      depending  on 8 or 10 bit addressing modes
;****************************************************************************

LOAD_ADDR_10    MACRO   SLAVE_ADDRESS

	bsf     _10BitAddr      ; Slave has 10 bit address
	movlw   (SLAVE_ADDRESS & 0xff)
	movwf   SlaveAddr               ; load low byte of address
        movlw   (((SLAVE_ADDRESS >> 7) & 0x06) | 0xF0)  ; 10 bit addr 11110AA0
	movwf   SlaveAddr+1     ; hi order  address

	ENDM

LOAD_ADDR_8     MACRO   SLAVE_ADDRESS

	bcf     _10BitAddr      ; Set for 8 Bit Address Mode
	movlw   (SLAVE_ADDRESS & 0xff)
	movwf   SlaveAddr

	ENDM

;****************************************************************************
;                            I2C_WRITE_SUB
;                               
;  Writes a message just like I2C_WRITE, except that the data is preceeded 
;  by a sub-address to a slave device.
;        Eg. : A serial EEPROM would need an address of memory location for 
;	       Random Writes
;
;  Parameters :
;        _BYTES_ #of bytes starting from RAM pointer _SourcePointer_ (constant)
;        _SourcePointer_     Data Start Buffer pointer in RAM (file Registers)
;        _Sub_Address_       Sub-address of Slave (constant)
;
;   Sequence :
;               S-SlvAW-A-SubA-A-D[0]-A.....A-D[N-1]-A-P
;
;  If an error occurs then the routine simply returns and user should check for
;       flags in Bus_Status Reg (for eg. _Txmt_Success flag
;
;       Returns :       WREG = 1 on success, else WREG = 0
;
;  NOTE : The address of the slave must be loaded into SlaveAddress Registers,
;         and 10 or 8 bit mode addressing must be set
;
;  COMMENTS :
;       I2C_WR may prove to be more efficient than this macro in most situations
;       Advantages will be found for Random Address Block Writes for Slaves with
;       Auto Increment Sub-Addresses (like Microchip's 24CXX series Serial 
;	    EEPROMS)
;
;****************************************************************************

I2C_WR_SUB      MACRO   _BYTES_, _SourcePointer_, _Sub_Address_

	movlw   (_BYTES_ + 1)
	movwf   tempCount

	movlw   (_SourcePointer_ - 1)
	movwf   FSR0L

	moviw	FSR0
	movwf   StoreTemp_1     ; temporarily store contents of (_SourcePointer_ -1)
	movlw   _Sub_Address_
	movwi	FSR0

	call    _i2c_block_write        ; write _BYTES_+1 block of data

	movf    StoreTemp_1,W
	movwf   (_SourcePointer_ - 1) ; restore contents of (_SourcePointer_ - 1)
	
;	call    TxmtStopBit     ; Issue a stop bit to end transmission

	ENDM
I2C_WR_SUB2      MACRO   _BYTES_, _SourcePointer_, _Sub_Address_, _Sub_Address2_

	movlw   (_BYTES_ + 2)
	movwf   tempCount

	movlw   (_SourcePointer_ - 2)
	movwf   FSR0L

        moviw	FSR0
	movwf   StoreTemp_1     ; temporarily store contents of (_SourcePointer_ -2)
	movlw   _Sub_Address_
	movwi	FSR0
	movlw   (_SourcePointer_ - 1)
	movwf   FSR0L
	movlw	_Sub_Address2_
	movwi	FSR0
	movlw   (_SourcePointer_ - 2)
	movwf   FSR0L

	call    _i2c_block_write        ; write _BYTES_+1 block of data

	movf    StoreTemp_1,W
;	movwf   (_SourcePointer_ - 2) ; restore contents of (_SourcePointer_ - 1)
	
;	call    TxmtStopBit     ; Issue a stop bit to end transmission

	ENDM


;****************************************************************************
;                               I2C_WRITE
;
;  A basic macro for writing a block of data to a slave
;
;  Parameters :
;       _BYTES_            #of bytes starting from RAM pointer _SourcePointer_
;      _SourcePointer_     Data Start Buffer pointer in RAM (file Registers)
;
;   Sequence :
;               S-SlvAW-A-D[0]-A.....A-D[N-1]-A-P
;
;  If an error occurs then the routine simply returns and user should check for
;       flags in Bus_Status Reg (for eg. _Txmt_Success flag)
;
;  NOTE : The address of the slave must be loaded into SlaveAddress Registers, 
;        and 10 or 8 bit mode addressing must be set
;****************************************************************************


I2C_WR          MACRO   _BYTES_, _SourcePointer_

	movlw   _BYTES_
	movwf   tempCount
	movlw   _SourcePointer_
	movwf   FSR0L

	call    _i2c_block_write
	call    TxmtStopBit  ; Issue a stop bit for slave to end transmission
	
	ENDM

;*****************************************************************************
;
;                               I2C_READ
;
; The basic MACRO/procedure to read a block message from a slave device
;
;   Parameters :
;               _BYTES_         :  constant : #of bytes to receive
;               _DestPointer_   :  destination pointer of RAM (File Registers)
;
;   Sequence :
;               S-SlvAR-A-D[0]-A-.....-A-D[N-1]-N-P
;
;   If last byte, then Master will NOT Acknowledge (send NACK)
;
;  NOTE : The address of the slave must be loaded into SlaveAddress Registers, 
;     and 10 or 8 bit mode addressing must be set
;
;*****************************************************************************

I2C_READ        MACRO   _BYTES_, _DestPointer_


	movlw   (_BYTES_ -1)
	movwf   tempCount       ; -1 because, the last byte is used out of loop
	movlw   _DestPointer_   
	movwf   FSR0L             ; FIFO destination address pointer

	call    _i2c_block_read               

	ENDM

;***************************************************************************
;
;                               I2C_READ_SUB
;  This MACRO/Subroutine reads a message from a slave device preceeded by 
;  a write of the sub-address.
;  Between the sub-addrers write & the following reads, a STOP condition 
;  is not issued and a "REPEATED START" condition is used so that an other 
;  master will not take over the bus, and also that no other master will 
;  overwrite the sub-address of the same salve.
;
;   This function is very commonly used in accessing Random/Sequential reads 
;   from a memory device (e.g : 24Cxx serial of Serial EEPROMs from Microchip).
;
;  Parameters :
;               _BYTES_         # of bytes to read
;               _DestPointer_   The destination pointer of data to be received.
;               _BubAddress_    The sub-address of the slave
;
;  Sequence :
;		S-SlvAW-A-SubAddr-A-S-SlvAR-A-D[0]-A-.....-A-D[N-1]-N-P
;
;
;***************************************************************************

I2C_READ_SUB    MACRO   _BYTES_, _DestPointer_, _SubAddress_

	bcf     _Slave_RW       ; set for write operation
	call    TxmtStartBit    ; send START bit
	call    Txmt_Slave_Addr ; if successful, then _Txmt_Success bit is set


	movlw   _SubAddress_
	movwf   DataByte        ; START address of EEPROM(slave 1)
	call    SendData        ; write sub address
;
; do not send STOP after this, use REPEATED START condition
;

	I2C_READ _BYTES_, _DestPointer_

	ENDM
I2C_READ_SUB2    MACRO   _BYTES_, _DestPointer_, _SubAddress_, _SubAddress2_

	bcf     _Slave_RW       ; set for write operation
	call    TxmtStartBit    ; send START bit
	call    Txmt_Slave_Addr ; if successful, then _Txmt_Success bit is set


	movlw   _SubAddress_
	movwf   DataByte        ; START address of EEPROM(slave 1)
	call    SendData        ; write sub address
	movlw   _SubAddress2_
	movwf   DataByte        ; START address of EEPROM(slave 1)
	call    SendData        ; write sub address
;
; do not send STOP after this, use REPEATED START condition
;

	I2C_READ _BYTES_, _DestPointer_

	ENDM
;----------------------------------------------------------------------
;----------------------------------------------------------------------
 ;   cblock 0x20

	UDATA
ADD	RES 2
IO_buf	RES 10
IN_buf	RES 10

 ;   cblock 0xf0
	UDATA_SHR

w_temp 		RES 1
status_temp	RES 1
SlaveAddr	RES 1     ; Slave Addr must be loader into this reg
SlaveAddrHi	RES 1     ; for 10 bit addressing mode
DataByte	RES 1     ; load this reg with the data to be transmitted
BitCount	RES 1     ; The bit number (0:7) transmitted or received
Bus_Status	RES 1     ; Status Reg of I2C Bus for both TXMT & RCVE
Bus_Control	RES 1     ; control Register of I2C Bus
DelayCount	RES 1
DataByteCopy	RES 1     ; copy of DataByte for Left Shifts (destructive)

SubAddr		RES 1     ; sub-address of slave (used in I2C_HIGH.ASM)
SrcPtr		RES 1     ; source pointer for data to be transmitted

tempCount	RES 1     ; a temp variable for scratch RAM
StoreTemp_1	RES 1     ; a temp variable for scratch RAM, do not disturb contents
SEN_test	RES 1	  ; ACKTIM not tested

_End_I2C_Ram	RES 1     ; unused, only for ref of end of RAM allocation


;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlp  high  start               ; load upper byte of 'start' label
        goto   start                     ; go to beginning of program

INT_VECTOR	CODE	0X004

	;; 
	;; Interrupt
	;; 

	BANKSEL PIR1
 if _CLOCK_STRETCH_CHECK                ; TMR0 Interrupts enabled only if Clock Stretching is Used
        btfss   INTCON,T0IF
        goto    check	                ; other Interrupts
        bsf     _TIME_OUT_              ; MUST set this Flag
        bcf     INTCON,T0IF
	goto	int_ret
 endif


check:
	btfss	PIR1,SSP1IF
	goto	int_ret
	bcf	PIR1,SSP1IF
	call	sspint
int_ret:
	retfie



;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start:

   .sim "module lib libgpsim_modules"
   .sim "p16f1823.xpos = 48"
   .sim "p16f1823.ypos =  36"
   .sim "module load pu pu1"
   .sim "pu1.xpos = 216"
   .sim "pu1.ypos =  48"
   .sim "module load pu pu2"
   .sim "pu2.xpos = 216"
   .sim "pu2.ypos =  96"
   .sim "node n1"
   .sim "attach n1 portc0 pu1.pin porta0" ; ee.SCL"
   .sim "node n2"
   .sim "attach n2 portc1 pu2.pin porta1" ; ee.SDA"
   .sim "scope.ch0 = 'porta0'"
   .sim "scope.ch1 = 'porta1'"


    BANKSEL	APFCON
    bsf		APFCON,0   ;; should not change anything
    bsf		APFCON,6   ;; select alternate SDO
    bsf		APFCON,3   ;; select alternate T1G
    BANKSEL	ANSELA
    clrf	ANSELA
    clrf	ANSELC
    BANKSEL	OSCCON
    movlw	(0xe<<3)|2	; set internal RC to 8 Mhz
    movwf	OSCCON
 ;RRR   clrf        TRISA
    bsf		PIE1,SSP1IE	; allow SSP interrupts
    bsf		INTCON,GIE	; allow interrupts
    bsf		INTCON,PEIE	; allow interrupts
    BANKSEL	PORTA
;    bsf		PORTA,0	; Write protect
 ;   bsf		PORTA,1	; A0
    movlw	0x10
    movwf	tempCount
    movlw	IO_buf
    movwf	FSR0L

Fill_loop:
    movf	FSR0L,W
    movwi	FSR0++
    decfsz	tempCount,F
    goto	Fill_loop

    bsf		SEN_test,0
    banksel	SSPCON
    ; test slave receive of data with SEN set
    ; I2C Slave mode, 7-bit address
    movlw	(1<<SSPEN) | (1<<CKP) | 0x6	
    movwf	SSPCON
    movlw	0xa2
    movwf	SSPADD
    ;bsf		SSPCON2,SEN	; turn on clock stretch after ack
    banksel	PORTA
    
    call InitI2CBus_Master


    LOAD_ADDR_8 0xa2
    call IsSlaveActive
    btfss _SlaveActive
  .assert "'Slave not active'"
    nop
    LOAD_ADDR_8 0xa2
;    I2C_WR_SUB 8, IO_buf, 0x0c
    I2C_WR_SUB2 8, IO_buf, 0x0c, 0x0c
    call    TxmtStopBit     ; Issue a stop bit to end transmission
    movf    Bus_Status,W
  .assert "W == 0x10, '*** FAILED I2C write status'"
    nop

    nop

poll_ready:
    LOAD_ADDR_8 0xa2
    call IsSlaveActive
    btfss _SlaveActive
    goto poll_ready	; slave not active yet
    nop

    banksel SSPCON3
    movlw	(1<<DHEN)|(1<<AHEN)
     movwf	SSPCON3
;
;	write 0xa2 0x0c 0x0c to set an address
;	write RSTART 0xa3 to initiate read
;	read 8 bytes of data into ram starting at IN_buf
;
    banksel	PORTA
    LOAD_ADDR_8 0xa2
    I2C_READ_SUB2 8, IN_buf, 0x0c, 0x0c
    nop
    banksel	PORTA
    movf	IN_buf,W
  .assert "W == 0xf5, '*** FAILED read data'"
    nop
    movf	Bus_Status,W
  .assert "W == 0x30, '*** FAILED 8 bit read status'"
    nop

;
;	write 8 bytes of data in slave 10bit mode
;
    bcf		SEN_test,0
    BANKSEL	SSPCON
    movlw	(1<<DHEN)|(1<<AHEN)
    movwf	SSPCON3
    clrf	SSPCON2
    movlw	(1<<SSPEN) | (1<<CKP) | 0xf	
    movwf	SSPCON
    movlw	0xf0
    movwf	SSPADD
    BANKSEL	PORTA
    LOAD_ADDR_10 0x0c
    I2C_WR 8, IO_buf
    call    TxmtStopBit     ; Issue a stop bit to end transmission
    movf    Bus_Status,W
  .assert "(W & 0xc3) == 0x00, '*** FAILED I2C write status DHEN=1 AHEN=1'"
    nop

  .assert "'*** PASSED 16f1823 I2C slave test'"
    nop
    
    goto $

IsSlaveActive
                bcf     _Slave_RW       ; set for write operation
                call    TxmtStartBit    ; send START bit
                call    Txmt_Slave_Addr ; if successful, then _Txmt_Success bit is set
;
                bcf     _SlaveActive
                btfss   _ACK_Error      ; skip if NACK, device is not present or not responding
                bsf     _SlaveActive    ; ACK received, device present & listening
                call    TxmtStopBit
                return

include "i2c_low.inc"


_i2c_block_write:
	call    TxmtStartBit    ; send START bit
	bcf     _Slave_RW       ; set for write operation
	call    Txmt_Slave_Addr ; if successful, then _Txmt_Success bit is set
;
_block_wr1_loop:
	btfss   _Txmt_Success
	return
	moviw	FSR0++
	movwf   DataByte  ; start from the first byte starting at _DataPointer_
	call    SendData  ; send next byte, bus is our's !
	decfsz  tempCount, F
	goto    _block_wr1_loop  ; loop until desired bytes of data 
				 ;  transmitted to slave
	return
;
;****************************************************************************

_i2c_block_read:                
	call    TxmtStartBit    ; send START bit
	bsf     _Slave_RW       ; set for read operation
	bcf     _Last_Byte_Rcv  ; not a last byte to rcv
	call    Txmt_Slave_Addr ; if successful, then _Txmt_Success bit is set
	btfsc   _Txmt_Success
	goto    _block_rd1_loop ; end
	call    TxmtStopBit     ; Issue a stop bit for slave to end transmission
	retlw   FALSE           ; Error : may be device not responding
;
_block_rd1_loop:
	call    GetData
	movf    DataByte,W
	movwi   FSR0++	;start receiving data, starting at Destination Pointer
	decfsz  tempCount, F
	goto    _block_rd1_loop  ; loop until desired bytes of data transmitted to slave
	bsf     _Last_Byte_Rcv          ; last byte to rcv, so send NACK
	call    GetData
	movf    DataByte,W
	movwi	FSR0
	call    TxmtStopBit   ; Issue a stop bit for slave to end transmission
	retlw   TRUE

sspint:
	BANKSEL SSPSTAT
	btfsc	SSPSTAT,R_NOT_W	; write test ?
	goto	sspint_wr
	btfsc	SSPSTAT,UA	; UA bit set
	goto	sspint_ua
	btfsc	SSPSTAT,P
	goto	sspint_p
	btfss	SSPCON,CKP
	goto	sspint_ckp
	
	movf	SSPBUF,W
        return

sspint_wr:
	movlw	0xf5	; byte to send
	movwf	SSPBUF	
	bsf	SSPCON,CKP	; turn off clock stretch
	return

sspint_ua:
	movlw	0x0c	; second byte address
	movwf	SSPADD
	movf	SSPBUF,W
	return

sspint_p:
        movf    SSPBUF,W
        return

sspint_ckp:		; in clock hold
        movf    SSPBUF,W
	btfsc   SEN_test,0
	goto	skip_acktim
  .assert "(ssp1con3 & 0x80)==0x80, 'ACKTIM not set'"
	nop
	bcf SSPCON2,ACKDT ;ack data
skip_acktim:
	bsf SSPCON,CKP	  ;turn off clock stretching
	return


	end
