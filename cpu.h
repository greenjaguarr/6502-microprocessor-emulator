#ifndef CPU_H
#define CPU_H

#include <cstdint>
#include <vector>
#include <string>

#include "memory.h"

class CPU {
public:
    CPU();
    void Reset(UnifiedMemory& memory);
    // void loadProgram(const std::vector<uint8_t>& program, uint16_t startAddress);
    void Execute(u32 Cycles, UnifiedMemory& memory);
    void store_output_file(const std::string& filename, uint16_t start, uint16_t end, UnifiedMemory& memory);

private:
    uint8_t memory[65536]; // 64KB memory space
    uint16_t PC;  // Program Counter
    uint8_t A, X, Y; // Registers
    uint8_t status; // Status register
    uint8_t SP; // Stack Pointer
    
    Byte FetchByte(u32& cycles, UnifiedMemory& memory);
    Word FetchWord(u32& cycles, UnifiedMemory& memory);
};

#endif // CPU_H
