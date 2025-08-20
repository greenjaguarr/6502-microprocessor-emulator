#!/bin/bash

# Virtual terminal for 6502 emulator

IN_PIPE="in_stream"
OUT_PIPE="out_stream"

# Create named pipes if they don't exist
[ ! -p "$IN_PIPE" ] && mkfifo "$IN_PIPE"
[ ! -p "$OUT_PIPE" ] && mkfifo "$OUT_PIPE"

# Ensure cleanup on exit
cleanup() {
    stty sane   # restore terminal
    echo
    exit
}
trap cleanup EXIT

# Configure terminal: raw mode, no echo
stty -icanon -echo

# Background: read from emulator output and print
cat "$OUT_PIPE" &

# Main loop: read from keyboard and send to emulator input
while true; do
    # read one char at a time
    IFS= read -r -n1 -d '' char
    if [ "$char" ]; then
        if [ "$char" = $'\r' ]; then
        # Map Enter (CR) → newline (0x0A)
            printf '\n' > "$IN_PIPE"
            # echo -n "\n" > "$IN_PIPE"
        # send char to emulator
        # echo -n "$char" > "$IN_PIPE"
        else
            # echo -n "$char" > "$IN_PIPE"
            printf '%s' "$char" > "$IN_PIPE"
        fi
    fi
done
