	list	p=16f84

	;; The purpose of this program is to test gpsim's capability to simulate
	;; the PIC's stack.
	;;
	;;   Start gpsim:
	;;   $ gpsim
	;;   then load the Startup command file 'stack_test.stc':	
	;;   gpsim> load c stack_test.stc
	;;
	;;    OR
	;;
	;;   invoke gpsim with the command file
	;;   $ gpsim -c stack_test.stc
	;;
	;;    OR
	;;
	;;   invoke the simualtion from the Makefile:
	;;   make sim

	;; In all cases, the stimulus file will load the simulation
	;; file and create the stimuli. In the Makefile case, the
	;; simulation file will be (re)created if the .asm has been
	;; been changed.
	;;
	;;


		
include "p16f84.inc"

  cblock  0x20
		
	temp1
	temp2
	temp3
  endc

	org	0
	goto	start
	
	org	4
start
	clrf	temp1		; a counter
	clrf	temp2		; a flag keeping track of the state of port b

init_tasks
	goto	Task_0_init
init_done


	return
begin

	;; Count the rising edges on portb bit 0
	
	movf	PORTB,w
	xorwf	temp2,w
	xorwf	temp2,f
	andwf	temp2,w
	andlw	1
	
	skpz
	  incf	temp1,f
	
	goto   begin

Task_0_init:
	call	Task_1_init
Task_0:
	return

Task_1_init:
	call	Task_2_init
Task_1:

	btfsc	PORTB,0
	goto	t1b
	call	$+1
t1a
	movlw	2
	return
t1b
	movlw	3
	return

Task_2_init:
	call	Task_3_init
Task_2:
	return

Task_3_init:
	call	Task_4_init
Task_3:
	return

Task_4_init:
	call	Task_5_init
Task_4:
	return

Task_5_init:
	call	Task_6_init
Task_5:
	return

Task_6_init:
	call	Task_7_init
Task_6:
	return

Task_7_init:
	call	init_done
Task_7:
	return

	end
