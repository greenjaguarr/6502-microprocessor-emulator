    .org $8000
begin LDA

reset:
    lda #20
    jmp loop

loop:
    jmp loop