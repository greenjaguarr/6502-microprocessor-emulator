    .org $8000

INSTREAM  = $6008
INSTATUS  = $6009
OUTSTREAM = $600A
OUTSTATUS = $600B

main_loop:
    ; Check input status (bit 3 = new char)
    LDA INSTATUS
    AND #$08
    BEQ main_loop   ; no char, loop

    ; Read input char
    LDA INSTREAM

    ; Convert lowercase a-z to uppercase
    AND #$DF        ; clears bit 5

    ; Check if char is A-Z
    CMP #'A'
    BCC main_loop   ; ignore non-letter
    CMP #'Z'+1
    BCC send_char   ; letter

    JMP main_loop   ; ignore other characters

send_char:
    ; Wait until output is ready (bit 0 = ready)
wait_out:
    LDA OUTSTATUS
    AND #$01
    BEQ wait_out

    ; Write char to output
    STA OUTSTREAM

    JMP main_loop

    ; Reset vector
    .org $FFFC
    .word $8000
    .word $0000
