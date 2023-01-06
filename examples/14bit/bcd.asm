	list	p=16c84
  __config _wdt_off


include "p16c84.inc"
	
  cblock	0x0c
	hundreds,tens_and_ones
	bin,bint
  endc


	org	0

	goto    start

	org	4
start:
	clrf	bint
l1:
	
	call	binary_to_bcd
	incf	bint,f
	movf	bint,w
	movwf	bin
	goto	l1

	
;********************************************************
;binary_to_bcd - 8-bits
;
;Input 
;  bin  - 8-bit binary number
;Outputs
; hundreds - the hundreds digit of the BCD conversion
; tens_and_ones - the tens and ones digits of the BCD conversion

binary_to_bcd:

        CLRF    hundreds
        SWAPF   bin,W         ;Add the upper and lower nibbles
        ADDWF   bin,W         ;to get the one's digit
        ANDLW   0x0f
        SKPNDC                ;Go through a binary to bcd
         ADDLW  0x16          ;conversion for just the one's
        SKPNDC                ;digit
         ADDLW  0x06
        ADDLW   0x06
        SKPDC
         ADDLW  -0x06

        BTFSC   bin,4         ;Bit 4 is a special case
         ADDLW   0x16 - 1 + 0x6
        SKPDC
         ADDLW  -0x06

                              ;Now adjust the ten's digit
        BTFSC   bin,5         ;2^5 = 32, so add 3 to the ten's
         ADDLW  0x30          ;digit if bit 5 is set

        BTFSC   bin,6         ;2^6 = 64, so add 6
         ADDLW  0x60          ;if bit 6 is set

        BTFSC   bin,7         ;2^7 = 128, so add 2 (the ten's 
         ADDLW  0x20          ;digit) if bit 7 is set

        ADDLW   0x60          ;Convert the ten's digit to BCD
        RLF     hundreds,F    ;If there's a carry, then the input
        BTFSS   hundreds,0    ;was greater than 99.
         ADDLW  -0x60

        MOVWF   tens_and_ones
        BTFSC   bin,7         ;If msb is set then the hundred's
         INCF   hundreds,F    ;digit is a '2'

	
	return
	
	end
