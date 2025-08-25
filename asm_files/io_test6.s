    .org $8000

IN_STREAM = $6008
IN_STATUS = $6009
OUT = $6005
OUTSTREAM = $600A
pipe_closed_amount = $10

OUTPUTBUFFER = $2000
OUT_WRTIEPOS = $10

; reserve input buffer space. We have a read and write pointer
INPUTBUFFER = $3F00
IN_WRITEPOS = $11
IN_READPOS = $12

start:
    LDA #0
    STA pipe_closed_amount
    LDY #0
loop:
    ; LDX pipe_closed_amount
    ; INX
    ; TXA
    ; CMP #10
    ; BPL connection_timeout
    ; STA pipe_closed_amount
    jsr get_char
    CPX #0
    BEQ wait ; no char, so we wait a sec and try again
    ; we have obtained a valid char! yay
    jsr store_val
    jmp loop

wait:
    NOP
    jmp loop

get_char:
    LDA IN_STATUS
    CMP #$40 ; big problem
    BEQ halt
    CMP #$20 ; EOF; The pipe may be closed
    BEQ pipe_closed
    
    LDX #0  ;pipe is not closed, reset counter
    STX pipe_closed_amount

    TAX ; save status in X

    CMP #$08 ; Char
    BEQ char
    CMP #$10 ; no char, no problem
    BEQ no_char

    ; all valid status codes handled

no_char:
    LDA #$FF
    jsr store_val
    LDA #0
    rts

char:
    LDA IN_STREAM
    rts

store_val:
    sta OUT
    sta OUTPUTBUFFER,Y ; address = 3000 + Y
    sta OUTSTREAM
    iny ;increment Y
    rts

connection_timeout:
    LDA #55
    STA $3FFE
    LDA pipe_closed_amount
    STA $3FFF

pipe_closed:
    NOP ; wait a sec
    LDA pipe_closed_amount
    TAX
    INX
    TXA
    STA pipe_closed_amount
    CMP #50
    BMI loop ; BPL or bMI ; This is where there is a RTS missing? I think that is why the stack keeps growing. I am leaking memory haha

end:
    LDA #61
    jsr store_val ; signify that end is here
    jsr store_val
    LDA #13
    jsr store_val
    LDA #10
    jsr store_val
    ; for debug purposes
    LDA pipe_closed_amount
    jsr store_val

halt:
    jmp halt

nmi:
    rti

irq:
    
    rti

    ; Reset vector
    .org $FFFA
    .word nmi
    .word start
    .word irq
