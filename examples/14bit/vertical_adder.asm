	;; vertical_adder.asm
	;;
	;; a 7-instruction vertical adder

	list	p=16c84

include "p16c84.inc"

  cblock  0x0c

	k,r,t
  endc
	
	org 0


	movlw	0xf0
	movwf	k		;The 'constant'

	movlw	0xcc
	movwf	r		;The 'register'
	
	movlw	0xaa		;The 'carry in'


	;; vert_add - a 7-instruction vertical adder
	;;
	
vert_add:
	movwf	t		;Save the input carries
	xorwf	k,w
	xorwf	r,f
	andwf	t,f
	andwf	r,w
	xorwf	t,w
	xorwf	k,w



	goto	vert_add


  end
