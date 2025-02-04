#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <vector>

#include "memory.h"
#include "cpu.h"

#define memory_source "memory.bin"
#define output_file "output.bin"
#define n_cycles 500
#define start_address 0x2000
#define end_address 0x3fff

int main() {
    printf("Hello, World!\n");
    // Initialize UnifiedMemory which combines ROM, RAM, and I/O
    UnifiedMemory memory;

    // Load the ROM into memory
    if (!memory.LoadFromFile(memory_source)) {
        std::cerr << "Failed to load ROM data. Exiting." << std::endl;
        return 1;
    }

    // Initialize the CPU and reset it
    CPU cpu;
    cpu.Reset(memory);

    // Execute a certain number of clock cycles
    cpu.Execute(n_cycles, memory); // You can modify Execute() to accept memory

    cpu.dump_contents();
    // Store the output in a file from memory region 0x6000-0x6FFF
    cpu.store_output_file(output_file, start_address, end_address, memory);

    return 0;
};
