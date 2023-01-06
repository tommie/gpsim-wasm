	;; p18c242.asm
	list	p=18c242
        radix   dec

	nolist
        include "p18c242.inc"
	list

	__config	CONFIG2H,0	;Disable WDT
	__config	DEVID1,0xab
	__config	DEVID2,0xcd
	
	
  cblock  0x02

	temp,temp1,temp2
  endc

	org 0
start

	bra	start
	end

