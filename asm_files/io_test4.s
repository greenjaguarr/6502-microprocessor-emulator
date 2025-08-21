    .org $8000

IN = $6004
OUT = $6005
OUTPUTBUFFER = $2000

    LDX #0
    LDY #100
loop:
    DEY
    BEQ halt
    jsr get_char
    cmp #0
    BEQ end
    jsr store_val
    jmp loop


get_char:
    LDA IN
    rts


store_val:
    sta OUT
    sta OUTPUTBUFFER,X ; address = 3000 + X
    inx ;increment X
    rts

end:
    LDA #61
    jsr store_val
    LDA #13
    jsr store_val
    LDA #10
    jsr store_val

halt:
    
    jmp halt
    ; Reset vector
    .org $FFFC
    .word $8000
    .word $0000
