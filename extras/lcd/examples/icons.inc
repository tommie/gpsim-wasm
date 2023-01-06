;
;	createChar is called with W between 0 and 7 (0-2) with current bitmaps
;	It loads 8 bytes from a bitmask into the display CGRam starting at
;	address (W<<3) 	ie 0, 8, 16, 24, 32, 40, 48 or 56
createChar:
	movwf	buffer1
	movwf	buffer2
	bcf	STATUS,C
	rlf	buffer1,F
	rlf	buffer1,F
	rlf	buffer1,F	; buffer1 contains CGRam address
	movf	buffer1,W
	addlw	LCD_CMD_SET_CGRAM
	call	LCD_WRITE_CMD
	movlw	HIGH(battery)
	movwf	PCLATH
	movlw	LOW(battery)
	addwf	buffer1,W	; pixmap battery address + CGRam address
	btfsc	STATUS,C
	  incf	PCLATH,F
	movwf	buffer2
	movlw	8		; pixmap has 8 bytes to download
	movwf	buffer1
char_loop
	movf	buffer2,W
	call	ws2
	call	LCD_WRITE_DATA
	incf	PCLATH,F
	incfsz	buffer2,F
	  decf	PCLATH,F
	decfsz  buffer1,F
	  goto  char_loop
	return
pix_table:
;	retlw	LOW(battery)
;	retlw	HIGH(battery)
;	retlw	LOW(solar)
;	retlw	HIGH(solar)
;	retlw	LOW(pwm)
;	retlw	HIGH(pwm)
battery:  dt   01110b, 11011b, 10001b, 10001b, 11111b, 11111b, 11111b, 11111b
solar:	  dt   11111b, 10101b, 11111b, 10101b, 11111b, 10101b, 11111b, 00000b
pwm:	  dt   11101b, 10101b, 10101b, 10101b, 10101b, 10101b, 10101b, 10111b

