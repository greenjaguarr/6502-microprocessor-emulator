    .org $8000

OUT = $6005
    LDX #0
    LDA #65
    STA OUT
    jsr store_val
    LDA #66
    STA OUT
    jsr store_val
    LDA #67
    STA OUT
    jsr store_val
    LDA #68
    STA OUT
    jsr store_val
    LDA #69
    STA OUT
    jsr store_val
    LDA #107
    STA OUT
    jsr store_val

store_val:
    sta $3000,X ; address = 3000 + X
    inx ;increment X
    rts


halt:
    
    jmp halt
    ; Reset vector
    .org $FFFC
    .word $8000
    .word $0000
