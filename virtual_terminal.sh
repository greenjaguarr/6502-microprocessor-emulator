#!/bin/bash

# File names for pipes
IN_PIPE="in_stream"
OUT_PIPE="out_stream"

# Create pipes if they don't exist
[[ ! -p "$IN_PIPE" ]] && mkfifo "$IN_PIPE"
[[ ! -p "$OUT_PIPE" ]] && mkfifo "$OUT_PIPE"

# Ensure terminal is restored on exit
trap "stty sane; exit" SIGINT SIGTERM

# Put terminal in raw mode
stty -echo -icanon

# Open the pipes for reading and writing
exec 3> "$IN_PIPE"   # fd 3 → write to CPU's input
exec 4< "$OUT_PIPE"  # fd 4 → read from CPU's output

# Infinite loop: read keys from keyboard and read CPU output
while true; do
    # Check if a key was pressed
    if read -t 0.05 -n 1 key; then
        # Send the key to the CPU input pipe
        echo -n "$key" >&3
    fi

    # Check if CPU wrote a character
    if read -t 0.05 -n 1 cpu_char <&4; then
        # Print it on the terminal
        echo -n "$cpu_char"
    fi
done
