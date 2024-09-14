    .org $8000
    lda #$01
    ; sta $0200

part1:
    lda #$ff
    jmp part2:

part2:
    lda #$20
    jmp part1

    .org $FFFC
    .word $8000
    .word $0000