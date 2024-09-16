    .org $8000

    ldx #$00
    lda #$01 ; setup
    sta $0200
    jsr store_val
    sta $0201
    jsr store_val

    jmp step1

; loop
step1:
    lda $0200
    adc $0201
    sta $0200
    jsr store_val

    jmp step2

step2:
    lda $0201
    adc $0200
    sta $0201
    jsr store_val

    jmp step1

;halt
loop:
    jmp loop


store_val:
    sta $6000,X
    inx ;increment X
    rts


    .org $FFFC
    .word $8000
    .word $0000