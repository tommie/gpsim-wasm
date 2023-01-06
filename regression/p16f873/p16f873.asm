        list    p=p16f873
        radix   dec

  __config _WDT_OFF

include "p16f873.inc"

;Regression Test for 16f873's data EEPROM
;

  cblock        0x20
        adr_cnt
        data_cnt
        status_temp
  endc

    ; W shadowing for interrupts
    ; When an interrupt occurs, we don't know what the current bank settings
    ; are. The solution here is to declare two temporaries that have the
    ; same base address. That way we don't need to worry about the bank setting.

  cblock        0x70
        w_temp          ; W is stored here during interrupts if the
                        ; bank bits point to bank 0 or 2
  endc
  cblock        0xf0
        w_temp_shadow   ; W is stored here during interrupts if the
                        ; bank bits point to bank 0 or 2
  endc

;;========================================================================
;;========================================================================
;
;       Start 
;
        org     0

        goto    start

        org     4

    ;;************** 
    ;; Interrupt
    ;;*************

        movwf   w_temp
        swapf   STATUS,W
        clrf    STATUS          ;Bank 0
        movwf   status_temp

        bsf     STATUS,RP0

        btfss   (EECON1 & 0x7f),EEIF
         goto   check

;;; eeprom has interrupted
        bcf     (EECON1 & 0x7f),EEIF

check:
        clrf    STATUS          ;bank 0
        swapf   status_temp,W
        movwf   STATUS
        swapf   w_temp,F
        swapf   w_temp,W
        retfie
    ;;*************
    ;; end of interrupt
    ;;*************
     

start:  
        clrf    STATUS          ;Point to Bank 0
        clrf    adr_cnt
        clrf    data_cnt
        incf    data_cnt,F
        bsf     INTCON,EEIE

l1:     

        movf    adr_cnt,W
        bcf     STATUS,RP0
        bsf     STATUS,RP1
        movwf   EEADR ^ 0x100
        movf    data_cnt,W
        movwf   EEDATA ^ 0x100

        bcf     INTCON,GIE      ;Disable interrupts while enabling write

        bsf     STATUS,RP0
        bsf     (EECON1 ^ 0x180),WREN    ;Enable eeprom writes

        movlw   0x55            ;Magic sequence to enable eeprom write
        movwf   (EECON2 ^ 0x180)
        movlw   0xaa
        movwf   (EECON2 ^ 0x180)

        bsf     (EECON1 ^ 0x180),WR      ;Begin eeprom write

        bsf     INTCON,GIE      ;Re-enable interrupts
        
        btfsc   (EECON1 & 0x7f),WR
         goto   $-1

        clrf    STATUS          ; Point back to bank0
        incf    adr_cnt,W
        andlw   0x3f
        movwf   adr_cnt

        skpz
         goto   l1

        incfsz  data_cnt,F
         goto   l1

        goto    l1              ;A place to set a break point (for timing how long
                                ;it takes to fill the eeprom 256 times

        org     0x2100
        de      "Linux is cool!",0
        de      0xaa,0x55,0xf0,0x0f
        de      'g','p','s','i','m'

   end
