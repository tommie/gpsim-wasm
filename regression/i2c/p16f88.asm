
	list	p=16f88
        include <p16f88.inc>
        include <coff.inc>

        __CONFIG  _CONFIG1, _CP_OFF & _WDT_OFF &  _INTRC_IO & _PWRTE_ON & _LVP_OFF & _BODEN_OFF & _MCLR_OFF
        __CONFIG    _CONFIG2, _IESO_OFF & _FCMEN_OFF

	;; The purpose of this program is to test gpsim's ability to simulate a pic 16F88.
	;; Specifically, I2C

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

#define SCL_PIN	2
#define SDA_PIN 3
#define I2CPORT	PORTB
#define _SCL_D	I2CPORT,SCL_PIN
#define _SCL	TRISB,SCL_PIN
#define _SDA	TRISB,SDA_PIN

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
                        bsf     STATUS,RP0              ; select page 1
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
	movwf   FSR

	movf    INDF,W
	movwf   StoreTemp_1     ; temporarily store contents of (_SourcePointer_ -1)
	movlw   _Sub_Address_
	movwf   INDF           ; store temporarily the sub-address at (_SourcePointer_ -1)

	call    _i2c_block_write        ; write _BYTES_+1 block of data

	movf    StoreTemp_1,W
	movwf   (_SourcePointer_ - 1) ; restore contents of (_SourcePointer_ - 1)
	
;	call    TxmtStopBit     ; Issue a stop bit to end transmission

	ENDM
I2C_WR_SUB2      MACRO   _BYTES_, _SourcePointer_, _Sub_Address_, _Sub_Address2_

	movlw   (_BYTES_ + 2)
	movwf   tempCount

	movlw   (_SourcePointer_ - 2)
	movwf   FSR

	movf    INDF,W
	movwf   StoreTemp_1     ; temporarily store contents of (_SourcePointer_ -2)
	movlw   _Sub_Address_
	movwf   INDF           ; store temporarily the sub-address at (_SourcePointer_ -2)
	movlw   (_SourcePointer_ - 1)
	movwf   FSR
	movlw	_Sub_Address2_
	movwf	INDF
	movlw   (_SourcePointer_ - 2)
	movwf   FSR

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
	movwf   FSR

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
	movwf   FSR             ; FIFO destination address pointer

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
INT_VAR        UDATA

IO_buf  RES 10
IN_buf  RES 10

GPR_DATA                UDATA_SHR

w_temp RES  1
status_temp RES  1
SlaveAddr RES 1              ; Slave Addr must be loader into this reg
SlaveAddrHi RES 1            ; for 10 bit addressing mode
DataByte RES 1               ; load this reg with the data to be transmitted
BitCount RES 1               ; The bit number (0:7) transmitted or received
Bus_Status RES 1             ; Status Reg of I2C Bus for both TXMT & RCVE
Bus_Control RES 1            ; control Register of I2C Bus
DelayCount RES 1
DataByteCopy RES 1           ; copy of DataByte for Left Shifts (destructive)

SubAddr RES 1                ; sub-address of slave (used in I2C_HIGH.ASM)
SrcPtr RES 1                 ; source pointer for data to be transmitted

tempCount RES 1              ; a temp variable for scratch RAM
StoreTemp_1 RES 1            ; a temp variable for scratch RAM, do not disturb contents

_End_I2C_Ram RES 1           ; unused, only for ref of end of RAM allocation



;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlw  high  start               ; load upper byte of 'start' label
        movwf  PCLATH                    ; initialize PCLATH
        goto   start                     ; go to beginning of program

	;; 
	;; Interrupt
	;; 
INT_VECTOR   CODE    0x004               ; interrupt vector location
	movwf	w_temp
	swapf	STATUS,W
	movwf	status_temp
	bcf	STATUS,RP0	;adcon0 is in bank 0

 if _CLOCK_STRETCH_CHECK                ; TMR0 Interrupts enabled only if Clock Stretching is Used
        btfss   INTCON,T0IF
        goto    check	                ; other Interrupts
        bsf     _TIME_OUT_              ; MUST set this Flag
        bcf     INTCON,T0IF
	goto	int_ret
 endif


check:
	btfss	PIR1,SSPIF
	goto	int_ret
	bcf	PIR1,SSPIF
	call	sspint
int_ret:
	swapf	status_temp,w
	movwf	STATUS
	swapf	w_temp,F
	swapf	w_temp,W
	retfie



;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start:

  .sim "module lib libgpsim_modules"
   .sim "p16f88.xpos = 96"
   .sim "p16f88.ypos = 144"

   .sim "module load pu pu1"
   .sim "pu1.xpos = 276"
   .sim "pu1.ypos = 72"

   .sim "module load pu pu2"
   .sim "pu2.xpos = 96"
   .sim "pu2.ypos = 48"

   .sim "node n1"
   .sim "attach n1 portb2 pu1.pin portb4" ; ee.SCL"
   .sim "node n2"
   .sim "attach n2 portb3 pu2.pin portb1" ; ee.SDA"
   .sim "node n3"
   ;.sim "attach n3 porta0 ee.WP"
   .sim "node n4"
   ;.sim "attach n4 porta1 ee.A0"
   .sim "node n5"
   ;.sim "attach n5 porta2 ee.A1"
   .sim "node n6"
   ;.sim "attach n6 porta3 ee.A2"
   .sim "scope.ch0 = 'portb2'"
   .sim "scope.ch1 = 'portb3'"


    bsf  STATUS,RP0	; bank 1
    movlw	0xf6	; set internal RC to 8 Mhz
    movwf	OSCCON
    clrf        TRISA
    bsf		PIE1,SSPIE	; allow SSP interrupts
    bsf		INTCON,GIE	; allow interrupts
    bsf		INTCON,PEIE	; allow interrupts
    bcf  STATUS,RP0	; bank 0
;    bsf		PORTA,0	; Write protect
    bsf		PORTA,1	; A0
    movlw	0x10
    movwf	tempCount
    movlw	IO_buf
    movwf	FSR

Fill_loop:
    movf	FSR,W
    movwf	INDF
    incf	FSR,F
    decfsz	tempCount,F
    goto	Fill_loop

    movlw	0x36	
    movwf	SSPCON
    bsf  STATUS,RP0	; bank 1
    movlw	0xa2
    movwf	SSPADD
    bcf  STATUS,RP0	; bank 0
    
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

poll_ready:
    LOAD_ADDR_8 0xa2
    call IsSlaveActive
    btfss _SlaveActive
    goto poll_ready	; slave not active yet
    nop

;
;	write 0xa2 0x0c 0x0c to set an address
;	write RSTART 0xa3 to initiate read
;	read 8 bytes of data into ram starting at IN_buf
;
    LOAD_ADDR_8 0xa2
    I2C_READ_SUB2 8, IN_buf, 0x0c, 0x0c
    nop
    bcf  STATUS,RP0	; bank 0
    movf	IN_buf,W
  .assert "W == 0xf5, '*** FAILED read data'"
    nop
    movf	Bus_Status,W
  .assert "W == 0x30, '*** FAILED 8 bit read status'"
    nop

;
;	write 8 bytes of data in slave 10bit mode
;
    bcf  STATUS,RP0	; bank 0
    movlw	0x37	
    movwf	SSPCON
    bsf  STATUS,RP0	; bank 1
    movlw	0xf0
    movwf	SSPADD
    bcf  STATUS,RP0	; bank 0
    LOAD_ADDR_10 0x0c
    I2C_WR 8, IO_buf

  .assert "'*** PASSED p16f88 I2C test'"
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
	movf    INDF,W
	movwf   DataByte  ; start from the first byte starting at _DataPointer_
	incf    FSR, F            
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
	movwf   INDF      ;start receiving data, starting at Destination Pointer
	incf    FSR, F
	decfsz  tempCount, F
	goto    _block_rd1_loop  ; loop until desired bytes of data transmitted to slave
	bsf     _Last_Byte_Rcv          ; last byte to rcv, so send NACK
	call    GetData
	movf    DataByte,W
	movwf   INDF
	call    TxmtStopBit   ; Issue a stop bit for slave to end transmission
	retlw   TRUE

sspint:
    	bsf  STATUS,RP0	; bank 1
	btfsc	SSPSTAT,R_W	; write test ?
	goto	sspint_wr
	btfsc	SSPSTAT,UA	; UA bit set
	goto	sspint_ua
	
    	bcf  STATUS,RP0	; bank 0
	movf	SSPBUF,W
        return

sspint_wr:
    	bcf  STATUS,RP0	; bank 0
	movlw	0xf5	; byte to send
	movwf	SSPBUF	
	bsf	SSPCON,CKP	; turn off clock stretch
	return

sspint_ua:
	movlw	0x0c	; second byte address
	movwf	SSPADD
    	bcf  STATUS,RP0	; bank 0
	movf	SSPBUF,W
	return

	end
