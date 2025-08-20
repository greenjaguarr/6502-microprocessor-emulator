    .org $8000

INSTREAM  = $6008
INSTATUS  = $6009
OUTSTREAM = $600A
OUTSTATUS = $600B

; Main loop
main_loop:
    ; Check input status
    LDA INSTATUS
    AND #$08        ; mask bit 3 (new char available)
    BEQ main_loop   ; no char, loop

    ; Read char
    LDA INSTREAM

    ; Check if 'a' <= char <= 'z'
    CMP #'a'
    BCC check_nonletter  ; char < 'a', skip lowercase conversion
    CMP #'z'+1
    BCC lowercase_ok     ; char <= 'z', valid lowercase
    ; else fallthrough
check_nonletter:
    ; Check if char is letter A-Z
    CMP #'A'
    BCC main_loop      ; ignore non-letters
    CMP #'Z'+1
    BCC write_char      ; char <= 'Z', valid uppercase
    JMP main_loop       ; ignore other chars

lowercase_ok:
    ; Convert lowercase to uppercase
    SEC
    SBC #$20           ; 'a'-'A' = 0x20
write_char:
    ; Wait until output ready
wait_out:
    LDA OUTSTATUS
    AND #$01           ; bit 0 = ready/busy
    BEQ wait_out

    ; Write to output
    STA OUTSTREAM

    JMP main_loop

; Reset vector
    .org $FFFC
    .word $8000
    .word $0000
