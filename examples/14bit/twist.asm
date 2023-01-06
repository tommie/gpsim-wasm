	list	p=16c84
  __config _wdt_off

	nolist
include "p16c84.inc"
	list
	
  cblock	0x0c

	x,y,A,B,num
  endc


	org	0

	goto    start

	org	4
start

	movlw	1
	movwf	A
	movlw	0x0f
	movwf	B

	movlw	0x0a
	movwf	x
	movlw	0x14
	movwf	y

t1:	
	call	twist
	incf	x,f
	incf	y,f
	goto	t1


;;; --------------------------------------------------
;;; twist
;;;
;;; The purpose of this routine is to evaluate:
;;;
;;;    A*x + B*y
;;;  ------------
;;;     A  +  B
;;;
;;; Such that A+B = 2^N
;;;
;;;  This formula is useful for several applications. For example,
;;; DTMF generartors sometimes need to weight the individual sine
;;; waves differently. The term used to describe the difference in
;;; amplitudes is 'twist'. It's the ratio of the amplitudes in dB.
;;;  Another application of this formulation is for low-pass filtering.
;;; If 'x' is the current filtered value, and 'y' is the latest
;;; sample, then this equation will be a recursive low-pass filter.
;;; z-transforms may be used to calculate the frequency response,
;;; but that's beyond the scope of this simple description. An intui-
;;; tive way to look at it is as weighted averages. If A is made
;;; large relative to B, then more emphasis is placed on the average.
;;; If B is large then the latest samples are given more weight.
;;;
;;; Algorithm:
;;;   This algorithm exploits the relationship:	 A + B = 2^N.
;;; For example, consider N=4. Then the (A,B) pairs are:
;;; 
;;; (1,15), (2,14), (3,13), (4,12), (5,11), (6,10), (7,9), (8,8)
;;; i.e. B = 2^N - A , and , A = 2^N - B
;;; 
;;; Upon closer examination, it's observed that the (A,B) pairs
;;; are two's complements of one another (modulo 2^N). E.g., the 2's
;;; complement of 1 mod 16 is 15. In general:
;;; 
;;;     B = (~A + 1) % 2^N
;;; 
;;; Substituting this into the twist formula yields:
;;;
;;;   A*x  + ((~A + 1) % 2^N)*y
;;;  ---------------------------
;;;         2^N
;;;
;;; A modulo N operation means that only the lower N bits are significant.
;;; So dropping the notation and keeping this observation in mind we
;;; can simplify this equation:
;;;
;;;   A*x + (~A + 1)*y     A*x + ~A*y + y
;;;  ------------------ = ----------------
;;;        2^N                  2^N
;;;
;;; Which can be evaluated in psuedo-C:
;;;
;;; int A;     /* initialized somewhere's else */
;;; int num;
;;;
;;; int twist ( int x, int y)
;;; {
;;;   int num = y;
;;;
;;;   num += (A & 0x01) ? x : y;
;;;   num >>= 1
;;; 
;;;   num += (A & 0x02) ? x : y;
;;;   num >>= 1
;;; 
;;;   num += (A & 0x04) ? x : y;
;;;   num >>= 1
;;; 
;;;   num += (A & 0x08) ? x : y;
;;;   num >>= 1
;;;
;;;   return num;
;;; }
;;; 
;;; Excution time: 1 + 5*N cycles (e.g. 21cycles for A+B=16=2^4)
;;;
;;; 
;;; 
	
twist:
	movf	y,w
	movwf	num

	btfsc	A,0
	 movf	x,w
	addwf	num,f
	rrf	num,f

	movf	y,w
	btfsc	A,1
	 movf	x,w
	addwf	num,f
	rrf	num,f

	movf	y,w
	btfsc	A,2
	 movf	x,w
	addwf	num,f
	rrf	num,f

	movf	y,w
	btfsc	A,3
	 movf	x,w
	addwf	num,f
	rrf	num,f

	return

	end

