#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <vector>

#include "memory.h"
#include "cpu.h"



int main() {
    // Initialize UnifiedMemory which combines ROM, RAM, and I/O
    UnifiedMemory memory;

    // Load the ROM into memory
    if (!memory.LoadFromFile("memory.bin")) {
        std::cerr << "Failed to load ROM data. Exiting." << std::endl;
        return 1;
    }

    // Initialize the CPU and reset it
    CPU cpu;
    cpu.Reset(memory);

    // Execute a certain number of clock cycles
    int cycles = 500;  // Adjust the number of cycles to simulate
    cpu.Execute(cycles, memory); // You can modify Execute() to accept memory

    // Store the output in a file from memory region 0x6000-0x6FFF
    cpu.store_output_file("output.bin", 0x2000, 0x3fff, memory);

    return 0;
};
