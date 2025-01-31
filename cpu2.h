#ifndef CPU_H
#define CPU_H

#include <cstdint>
#include <vector>
#include <string>

#include "memory.h"

class CPU {
public:
    void Reset(UnifiedMemory& memory);
    void Execute(u32 Cycles, UnifiedMemory& memory);
    void store_output_file(const std::string& filename, uint16_t start, uint16_t end, UnifiedMemory& memory);

private:
    uint8_t memory[65536]; // 64KB memory space
    uint16_t PC = 0;  // Program Counter
    uint8_t A = 0, X=0, Y=0; // Registers
    // uint8_t status; // Status register
    bool C=false, Z=false, I=false, D=false, B=false, V=false, N=false; // Flags
    uint8_t SP; // Stack Pointer

    Byte FetchByte(u32& cycles, UnifiedMemory& memory);
    Word FetchWord(u32& cycles, UnifiedMemory& memory);
    Byte ReadByte(u32& cycles, UnifiedMemory& memory, Word address);
    void StoreByte(u32& cycles, UnifiedMemory& memory, Byte value, Word address);
    void pushBytetoStack(Byte data, u32& cycles, UnifiedMemory& memory);
    Byte pullBytefromStack(u32& cycles, UnifiedMemory& memory);
    void SetPC_absolute(u32& cycles, Word Address);
    void SetPC_relative(u32& cycles, Byte offset);
    void SetStatusNZbasedonA();
    void SetStatusNZbasedonX();
    void ADC(Byte operand);
    void AND(Byte operand);

    static constexpr Byte INS_LDA_IM = 0xA9;
    static constexpr Byte INS_LDA_ZP = 0xA5;
    static constexpr Byte INS_JMP_ABS = 0x4C;
    static constexpr Byte INS_JSR = 0x20;
    static constexpr Byte INS_NOP = 0xEA;
    static constexpr Byte INS_ADC_IM = 0x69;
    static constexpr Byte INS_ADC_ABS = 0x6D;
    static constexpr Byte INS_AND_IM = 0x29;
    static constexpr Byte INS_AND_ABS = 0x2D;
    static constexpr Byte INS_ASL_A = 0x0A;
    static constexpr Byte INS_STA_ABS = 0x8D;
    static constexpr Byte INS_RTS = 0x60;
    static constexpr Byte INS_LSR_A = 0x4A;
    static constexpr Byte INS_STA_ABSX = 0x9D;
    static constexpr Byte INS_LDX_IM = 0xA2;
    static constexpr Byte INS_INX = 0xE8;
    static constexpr Byte INS_LDA_ABS = 0xAD;
    static constexpr Byte INS_BVS = 0x70;
};

#endif // CPU_H