
   ;;  port_stim - exercise gpsim PortStimulus module
   ;;
   ;; 
   ;;  Tests performed:
   ;;
   ;;     A PortStimulus module is instantiated and connected to an 18F452
   ;;     The port, tris, and latch register addresses of the PortStimulus
   ;;     are mapped into the 18F452's RAM address space. 


        list    p=18f452                ; list directive to define processor
        include <p18f452.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

.command macro x
  .direct "C", x
  endm

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA        UDATA
failures        RES     1

;; The Port Stimulus port, tris, and latch registers get mapped to these addresses:
P1_DATA   UDATA  0x10
P1_Port RES 1
P1_Tris RES 1
P1_Lat  RES 1
P1_Pullup  RES 1

;; Alternate addresses to map the port stimulus.
P1_AltPort RES 1
P1_AltTris RES 1
P1_AltLat  RES 1
P1_AltPullup  RES 1

  GLOBAL P1_Port, P1_Tris, P1_Lat, P1_Pullup
  GLOBAL P1_AltPort, P1_AltTris, P1_AltLat, P1_AltPullup

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE  0x000
        nop
   .sim "module library libgpsim_modules"

   .sim "module load PortStimulus P1"
   .sim "P1.xpos = 250.0"
   .sim "P1.ypos = 100.0"

   ;# First, create 8 nodes:

   .sim "node  n0"
   .sim "node  n1"
   .sim "node  n2"
   .sim "node  n3"

   .sim "node  n4"
   .sim "node  n5"
   .sim "node  n6"
   .sim "node  n7"

   ;# Now tie the ports together:

   .sim "attach n0 portb0 P1.p1"
   .sim "attach n1 portb1 P1.p2"
   .sim "attach n2 portb2 P1.p3"
   .sim "attach n3 portb3 P1.p4"

   .sim "attach n4 portb4 P1.p5"
   .sim "attach n5 portb5 P1.p6"
   .sim "attach n6 portb6 P1.p7"
   .sim "attach n7 portb7 P1.p8"

   ;# specify the PortStimulus addresses.
   ;These are the PIC addresses to where the PortStimulus registers are mapped.
   .sim "P1.portAdr = &P1_Port"
   .sim "P1.trisAdr = &P1_Tris"
   .sim "P1.latAdr  = &P1_Lat"
   .sim "P1.pullupAdr  = &P1_Pullup"

        CLRF    failures        ;Assume success

        CLRF    BSR

        MOVLW   0xff
        MOVWF   TRISB           ;Port B is an input

        CLRF    P1_Pullup       ;Turn off the pullups (should already be off).
        CLRF    P1_Tris         ;Stimulus port is an output
        CLRF    P1_Lat

a_to_b_loop:
        MOVWF   P1_Lat          ;PortStimulus and Port B are externally
        XORWF   PORTB,W         ;connected. So we should see the
        bnz     FAILED          ;same thing on each port.

        DECFSZ  PORTB,W
         goto   a_to_b_loop

   ; Now let the P1's Pullups drive the bus. We'll simulate an 
   ; open collector with a pullup resistor. The TRIS register 
   ; will switch the driver direction. When configured as an output,
   ; the port module will drive low. When configured as an input, 
   ; the pull up resistors will drive high.

        COMF    P1_Pullup,F     ;Turn on all of the pullups 
        COMF    P1_Tris,F       ;Port Stimulus is now an input port
        CLRF    P1_Lat          ;make all of the drivers low

        MOVLW   0xff
a_to_b_loop_pullups:
        MOVWF   P1_Tris         ;PortStimulus and Port B are externally
        XORWF   PORTB,W         ;connected. So we should see the
        bnz     FAILED          ;same thing on each port.

        DECFSZ  PORTB,W
         goto   a_to_b_loop_pullups

   ; Now let's write from PORTB to the PortStimulus.

        MOVLW   0xff            ;Make the PortStimulus an input
        MOVWF   P1_Tris
        COMF    TRISB,F         ;Port B is now an output port

        CLRW

b_to_a_loop:
        MOVWF   LATB            ;Drive to Pic's Port B
        XORWF   P1_Port,W       ;Read the result on the PortStimulus.
        bnz     FAILED          ;Should be the same thing on each port.

        DECFSZ  PORTB,W
         bra    b_to_a_loop

  ; Now lets remap the port stimulus.
        CLRF    P1_AltLat

        MOVLW   0x55
        MOVWF   LATB
  .assert "P1.port == 0x55        ,'Failed - Unable to write to memory mapped port'"
        nop
    ; Next check that reading the register as a register returns the expected value.
    ; Note, we have to use the reg() operator because had we written 'P1_Port == 0x55' 
    ; we'd be comparing 0x55 to the symbolic representation of the register we replaced 
    ; during the remap
  .assert "reg(&P1_Port) == 0x55        ,'Failed - Unable to write to memory mapped port'"
        nop

  .command "P1.portAdr = &P1_AltPort"
  .command "P1.trisAdr = &P1_AltTris"
  .command "P1.latAdr  = &P1_AltLat"
        nop

  .assert "P1.port == 0x55        ,'Failed - Unable to read port stimulus after remapping'"
        nop

     ; note
  .assert "P1_Port == 0           ,'Failed - Unable to restore register after port remapping'"
        nop
  .assert "reg(&P1_Port) == 0     ,'Failed - Unable to restore register after port remapping'"
        nop

  .assert "reg(&P1_AltPort) == 0x55  ,'Failed - Unable to remap port register a second time'"
        nop

  .assert  "'*** PASSED 18F452 port test'"
        bra     $

FAILED:
  .assert  "'*** FAILED 18F452 port test'"
        bra     $

  end
