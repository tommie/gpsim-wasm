	list	p=16c84

	;; The purpose of this program is to test gpsim's ability to simulate looping instructions.
	;; It's also used to get a rough bench mark on performance. (set a break point after the loop,
	;; run the program, check the number of cycles...
	
  cblock  0x20

	temp1
	temp2
	temp3
	adr_cnt
	data_cnt

	w_temp
	status_temp

  endc

status	equ	3
rp0	equ	5

intcon	equ	0x0b
gie	equ	7
eeie	equ	6

eecon1	equ	0x08
rd	equ	0
wr	equ	1
wren	equ	2
eeif	equ	4

eecon2	equ	0x09
eedata	equ	0x08
eeadr	equ	0x09
	
	org 0

	goto	start
	
	org	4
start
	clrf	temp1
	clrf	temp2
	movlw   5
	movwf	temp3
begin
	decfsz	temp1,f
	 goto   begin
	decfsz  temp2,f
	 goto   begin
	decfsz  temp3,f
	 goto   begin


	goto    start

	end