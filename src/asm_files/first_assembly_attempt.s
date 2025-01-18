    .org $8000

reset:
    lda #$20
    jmp loop

loop:
    jmp loop

    .reset $8000
    .word FFFC
