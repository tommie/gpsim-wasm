        ;; The purpose of this program is to test gpsim's ability to simulate
        ;; USART in the 14bit core.


        list    p=16f628
  __CONFIG _CP_OFF & _WDT_OFF & _HS_OSC & _PWRTE_ON & _LVP_OFF & _BODEN_OFF & _MCLRE_OFF



include "p16f628.inc"

#define RX_BUF_SIZE     0x10
#define ECHO            7
#define CTL_A           6

  cblock  0x20

        rx_buffer : RX_BUF_SIZE         ; must start at 0xn0
        line_buffer : RX_BUF_SIZE       ; must start at 0xn0
        temp1
        temp2
        temp3

        status_temp
        fsr_temp

        line_ptr
        rx_ptr
        cmd_mode

        line_first  ; first character in line buffer
        line_last   ; last character in line buffer
        rx_first    ; first character to rx
        rx_last     ; last character to rx
  endc
  cblock 0x70
        w_temp  ; avalable at any bank
  endc
        org 0

        goto    start

        org     4
        ;; Interrupt
        ;; 
        movwf   w_temp          ; save W
        swapf   STATUS,w
        bcf     STATUS,RP0
        movwf   status_temp     ; save STATUS

        btfsc   INTCON,PEIE
        btfss   PIR1,RCIF
        goto    int_done

;;;
        movf    FSR,w
        movwf   fsr_temp
        call    rx_put
        movf    fsr_temp,w
        movwf   FSR
        
int_done:       
        swapf   status_temp,w
        movwf   STATUS
        swapf   w_temp,f
        swapf   w_temp,w
        retfie



GREET    dt     "If you can see this then USART module can receive data\n"
         dt     "Focus on this window and type on keyboard. If charaters "
         dt     "appear in window,  USART module can send and reciever.\n", 0

send_greet
        movlw   GREET
        movwf   line_ptr
        goto    prompt_loop

        ;; ----------------------------------------------------
        ;;
        ;;            start
        ;;
start
        clrf    STATUS
        call    rx_init
        clrf    cmd_mode        ; initialize command mode
        clrf    PORTA
        clrf    PORTB
        movlw   0x07            ; turn off comparitor
        movwf   CMCON

        ;; USART Initialization
        ;;
        ;; Turn on the high baud rate (BRGH), disable the transmitter,
        ;; disable synchronous mode.
        ;;
        bsf     STATUS,RP0
        movlw   (1<<BRGH)
        movwf   TXSTA
        movlw   0x19
        movwf   SPBRG           ; 9600 baud with 4 MZ clock
        clrf    STATUS
        bsf     PORTB,2         ;Make sure the TX line drives high when
                                ;it is programmed as an output.
        bsf     STATUS,RP0
        movlw   0xc2            ; Make rb1 input and rb2 output. These are the
        movwf   TRISB           ; the RX and TX pins for the USART
        movlw   0               ; all "free" A ports output
        movwf   TRISA
        bcf     STATUS,RP0
        
        ;; Turn on the serial port
        movlw   (1<<SPEN) | (1<<CREN)
        movwf   RCSTA

        movf    RCREG,w
        bsf     INTCON,GIE
        bsf     INTCON,PEIE
        
        ;; Enable the transmitter
        bsf     STATUS,RP0
        bsf     TXSTA,TXEN
        bsf     PIE1,RCIE       ; set USART rx int
        bcf     STATUS,RP0

        bcf     cmd_mode,ECHO
greet:
        call    send_greet
;
;       loop in command mode looking for cn\r
;
command:
        bsf     cmd_mode,ECHO
loop:
        call    rx_get
        goto loop

rx_init                         ; purge rx buffer
        clrf    rx_first
        clrf    rx_last
        return

rx_put                          ; put received character onto rx queue
        movf    rx_last,w
        addlw   rx_buffer
        movwf   FSR
        movf    RCREG,w
        movwf   INDF
        btfss   cmd_mode,ECHO   ; Echo input ?
        goto    rx_more         ; no, leave

        movwf   temp2
        sublw   0x0d            ; is character a \r?
        btfsc   STATUS,Z
        goto    rx_newline

        movf    temp2,w
        call    putchar
        goto    rx_more

rx_newline
        call    newline

rx_more
        incf    rx_last,w
        andlw   0x0f
        movwf   rx_last
        return

newline
        movlw   0x0a
putchar
        btfss   PIR1,TXIF       ; wait for TX ready
        goto    $-1
        movwf   TXREG           ; echo RX'ed character
        return

rx_mt                           ; rxbuffer empty ?
        movf    rx_last,w       ; status,Z set if yes
        subwf   rx_first,w
        return

rx_get                          ; get character for rx buffer into W
        call rx_mt
        btfsc STATUS,Z
        return                  ; no character, return
        movf    rx_first,w
        addlw   rx_buffer
        movwf   FSR
        incf    rx_first,w
        andlw   0x0f
        movwf   rx_first
        movf    INDF,w
        return

prompt_loop:
        call    get_prompt      ; get next character
        andlw   0xff            ; return if zero
        btfsc   STATUS,Z
        return
        call    putchar
        incf    line_ptr,f
        goto    prompt_loop

get_prompt
        movf    line_ptr,w
        movwf   PCL

end
