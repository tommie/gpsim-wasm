		;; 
	list	p=12c508
        radix   dec

	nolist
include "p12x.inc"

	list



  __config _wdt_on

  cblock  0x07

	temp,temp1,temp2
  endc
	
	org 0

start:	
	goto	start

	
	end