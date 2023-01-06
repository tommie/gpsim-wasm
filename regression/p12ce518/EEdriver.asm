;
;       Program:          FL51XINC.ASM  V1.10
;       Revision Date:   
;                         09-09-97      Adapted to 12C51x parts
;			  01-Apr-1999	Added emulation hooks
;
;                         01-July-2004  Modified to suit LinTrain
;
; nolist
; comment this line out for use on real part
;#define EMULATED    
;
; PIC12C51X EEPROM communication code.  This code should be included in
; with the application.  These routines provide the following functionality:
; write a byte to a specified address.
; read a byte from a specified address.
; read a byte from the next address.
;
; Emulation Requires:
;     MPLAB-ICE
;     PCM16XA0 processor module
;     DVA12XP80 Device Adapter.
; Define EMULATED at the top of this file  (#define EMULATED)
;     This will set the I2C_PORT, SDA and SCL lines to communicate over 
;     Port A, pins 0 and 1.  It also assembles in the necessary TRIS 
;     instructions to allow reading from the SDA line.
;
; To convert the code for the actual part, simply comment out the #define EMULATED
; line and reassemble. 
;
;
; INTRODUCTION:
; The Microchip 12CE51x family of microntrollers are multichip modules 
; which contain a PIC12C508 microcontroller and a 24LC00 EEPROM.
; This application note is intended to provide users with highly compressed
; assembly code for communication between the EEPROM and the Microcontroller,
; which will leave the user a maximum amount of code space for the core 
; application.
;
;***************************************************************************
;***************************  EEPROM Subroutines  **************************
;***************************************************************************
; Communication for EEPROM based on I2C protocol, with Acknowledge.
;
; Byte_Write: Byte write routine
;       Inputs:  EEPROM Address   EEADDR
;                EEPROM Data      EEDATA
;       Outputs: Return 01 in W if OK, else return 00 in W
;
; Read_Current: Read EEPROM at address currently held by EE device. 
;       Inputs:  NONE
;       Outputs: EEPROM Data       EEDATA
;                Return 01 in W if OK, else return 00 in W
;
; Read_Random: Read EEPROM byte at supplied address
;       Inputs:  EEPROM Address    EEADDR
;       Outputs: EEPROM Data       EEDATA
;                Return 01 in W if OK, else return 00 in W
;
; Note: EEPROM subroutines will set bit 7 in ee_state register if the
;       EEPROM acknowledged OK, else that bit will be cleared.  This bit 
;       can be checked instead of refering to the value returned in W
;***************************************************************************
;
; OPERATION:
; For detailed operating information and other important information about 
; this code, see AN571.  This code was derived from AN571, with changes
; as appropriate for the specific hardware in the PIC12C51x parts.
;********************************************************************** 
;
;
;***************************************************************************
;***************************  Variable Listing  ****************************
;***************************************************************************
ok          equ     01h
no          equ     00h

#ifdef	EMULATED
i2c_port    equ     5		; Port A control register, used for I2C
scl         equ     01h         ; EEPROM Clock, SCL (I/O bit 7)
sda         equ     00h         ; EEPROM Data,  SDA (I/O bit 6)
#else
i2c_port    equ     GPIO        ; Port B control register, used for I2C
scl         equ     07h         ; EEPROM Clock, SCL (I/O bit 7)
sda         equ     06h         ; EEPROM Data,  SDA (I/O bit 6)
#endif

ee_ok       equ     07h         ; Bit 7 in ee_state used as OK flag for EE

; All variables declared in the RAMDEF.INC file


;***************************************************************************
;***************************  EEPROM Subroutines  **************************
;***************************************************************************
; Communication for EEPROM based on I2C protocol, with Acknowledge.
;
; WRITE_BYTE: Byte write routine
;       Inputs:  EEPROM Address   EEADDR
;                EEPROM Data      EEDATA
;       Outputs: Return 01 in W if OK, else return 00 in W
;
; READ_CURRENT: Read EEPROM at address currently held by EE device. 
;       Inputs:  NONE
;       Outputs: EEPROM Data       EEDATA
;                Return 01 in W if OK, else return 00 in W
;
; READ_RANDOM: Read EEPROM byte at supplied address
;       Inputs:  EEPROM Address    EEADDR
;       Outputs: EEPROM Data       EEDATA
;                Return 01 in W if OK, else return 00 in W
;
; Note: EEPROM subroutines will set bit 7 in ee_state register if the
;       EEPROM acknowledged OK, else that bit will be cleared.  This bit 
;       can be checked instead of refering to the value returned in W
;***************************************************************************
;********************** Set up EEPROM control bytes ************************
;***************************************************************************
read_current:
 movlw   b'10000100'        ; PC offset for read current addr.  EE_OK bit7='1'
 movwf   ee_state           ; Load PC offset
 goto    init_read_control

write_byte:
 movlw   b'10000000'        ; PC offset for write byte.  EE_OK: bit7 = '1'
 goto    init_write_control

read_random:
 movlw   b'10000011'        ; PC offset for read random.  EE_OK: bit7 = '1'

init_write_control:
 movwf   ee_state           ; Load PC offset register, value preset in W
 movlw   b'10100000'        ; Control byte with write bit, bit 0 = '0'
 
start_bit:
 bcf     i2c_port,sda       ; Start bit, SDA and SCL preset to '1'

;
;******* Set up output data (control, address, or data) and ee_bitcnt ********
;***************************************************************************
prep_transfer_byte:
 movwf   eebyte             ; Byte to transfer to EEPROM already in W
 movlw   .8                 ; ee_bitcnt to transfer 8 bits
 movwf   ee_bitcnt
#ifdef	EMULATED
 movlw   0x00		    ; make sure both are outputs
 tris    i2c_port
#endif

;
;************  Clock out data (control, address, or data) byte  ************
;***************************************************************************
output_byte:
 bcf     i2c_port,scl       ; Set clock low during data set-up
 rlf     eebyte,f           ; Rotate left, high order bit into carry bit
 bcf     i2c_port,sda       ; Set data low, if rotated carry bit is
 skpnc                      ;   a '1', then:
 bsf     i2c_port,sda       ; reset data pin to a one, otherwise leave low
 nop
 bsf     i2c_port,scl       ; clock data into EEPROM
 decfsz  ee_bitcnt,f        ; Repeat until entire byte is sent
 goto    output_byte
 nop
;
;**************************  Acknowledge Check *****************************
;***************************************************************************
 bcf     i2c_port,scl       ; Set SCL low, 0.5us < ack valid < 3us
 nop
 bsf     i2c_port,sda
#ifdef	EMULATED
 movlw	 (0x01 << sda)	    ; make SDA an input
 tris	 i2c_port
#endif
 goto    $+1                ; May be necessary for SCL Tlow  at low voltage,
 bsf     i2c_port,scl       ; Raise SCL, EEPROM acknowledge still valid
 btfsc   i2c_port,sda       ; Check SDA for acknowledge (low)
 bcf     ee_state,ee_ok     ; If SDA not low (no ack), set error flag
 bcf     i2c_port,scl       ; Lower SCL, EEPROM release bus
 btfss   ee_state,ee_ok     ; If no error continue, else stop bit
 goto    stop_bit
#ifdef	EMULATED
 movlw   0x00		    ; SDA back to an output
 tris    i2c_port
#endif

;
;*****  Set up program ee_bitcnt offset, based on EEPROM operating mode  *****
;***************************************************************************
 movf    ee_state,w
 andlw   b'00001111'
 addwf   PCL,f 
 goto    init_address       ;PC offset=0, write control done, send address
 goto    init_write_data    ;PC offset=1, write address done, send data
 goto    stop_bit           ;PC offset=2, write done, send stop bit
 goto    init_address       ;PC offset=3, write control done, send address
 goto    init_read_control  ;PC offset=4, send read control
 goto    read_byte_setup    ;PC offset=5, set ee_bitcnt and read byte
 goto    stop_bit           ;PC offset=6, random read done, send stop

;
;**********  Initalize EEPROM data (address, data, or control) bytes  ******
;***************************************************************************
init_address:
 incf    ee_state,f         ; Increment PC offset to 2 (write) or to 4 (read)
 movf    eeaddr,w           ; Put EEPROM address in W, ready to send to EEPROM
 goto    prep_transfer_byte


init_write_data:
 incf    ee_state,f         ; Increment PC offset to go to STOP_BIT next
 movf    eedata,w           ; Put EEPROM data in W, ready to send to EEPROM
 goto    prep_transfer_byte

init_read_control:
 bsf     i2c_port,scl       ; Raise SCL
 nop
 bsf     i2c_port,sda       ; raise SDA
 incf    ee_state,f         ; Increment PC offset to go to read_byte_setup next
 movlw   b'10100001'        ; Set up read control byte, ready to send to EEPROM
 goto    start_bit          ; bit 0 = '1' for read operation

;
;**************************  Read EEPROM data  *****************************
;***************************************************************************
read_byte_setup:
 bsf     i2c_port,sda
 nop
 bsf     i2c_port,scl       ; set data bit to 1 so we're not pulling bus down.
 movlw   .8                 ; Set ee_bitcnt so 8 bits will be read into EEDATA
 movwf   ee_bitcnt
#ifdef	EMULATED
 movlw   (0x01 << sda)
 tris    i2c_port
#endif

read_byte:
 bsf     i2c_port,scl       ; Raise SCL, SDA valid.  SDA still input from ack
 setc                       ; Assume bit to be read = 1
 btfss   i2c_port,sda       ; Check if SDA = 1
 clrc                       ; if SDA not = 1 then clear carry bit
 rlf     eedata,f           ; rotate carry bit (=SDA) into EEDATA
 bcf     i2c_port,scl       ; Lower SCL
 bsf     i2c_port,sda       ; reset SDA
 decfsz  ee_bitcnt,f        ; Loop until bit count expires
 goto    read_byte          ; Read next bit if not finished reading byte

 bsf     i2c_port,scl
 nop
 bcf     i2c_port,scl
;******************  Generate a STOP bit and RETURN  ***********************
;***************************************************************************
stop_bit:
#ifdef	EMULATED
 movlw	0x00	; set SDA as output
 tris	i2c_port
#endif
 bcf     i2c_port,sda       ; SDA=0, on TRIS, to prepare for transition to '1' 
 bsf     i2c_port,scl       ; SCL = 1 to prepare for STOP bit
 goto    $+1                ; 4 NOPs neccessary for I2C spec Tsu:sto = 4.7us                  
 goto    $+1
 bsf     i2c_port,sda       ; Stop bit, SDA transition to '1' while SCL high
 
 btfss   ee_state,ee_ok     ; Check for error
 retlw   no                 ; if error, send back NO 
 retlw   ok                 ; if no error, send back OK
;
;Note: SDA and SCL still being driven by master, both set to outputs.
;****************************************************************************
 list
;************************  End EEPROM Subroutines  **************************
