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
    void dump_contents();

private:
    // Byte memory[65536]; // 64KB memory space
    Word PC = 0;  // Program Counter
    Byte A = 0, X=0, Y=0; // Registers
    // uint8_t status; // Status register
    bool C=false, Z=false, I=false, D=false, B=false, V=false, N=false; // Flags
    Byte SP; // Stack Pointer

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
    void SetStatusNZbasedonY();
    void ADC(Byte operand);
    void AND(Byte operand);
    void CMP(Byte operand);
    void SBC(Byte operand);
    void LDA(Byte operand);

    Byte AM_IM_LOAD(u32 Cycles, UnifiedMemory& memory); // addressing mode: immediate
    Byte AM_ABS_LOAD(u32 Cycles, UnifiedMemory& memory);
    Byte AM_ABSY_LOAD(u32 Cycles, UnifiedMemory& memory);

    Word AM_ABSY_STORE(u32 Cycles, UnifiedMemory& memory);
    Word AM_ZP_STORE(u32 Cycles, UnifiedMemory& memory);

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
    static constexpr Byte INS_CMP_IM = 0xC9;
    static constexpr Byte INS_CMP_ABS = 0xCD;
    static constexpr Byte INS_SBC_IM = 0xE9;
    static constexpr Byte INS_SBC_ABS = 0xED;
    static constexpr Byte INS_BEQ = 0xF0;
    static constexpr Byte INS_PHA = 0x48;
    static constexpr Byte INS_PLA = 0x68;
    static constexpr Byte INS_DEC_A = 0x3A; // This is not a real instructionon the OG but it is in modern versions like the one in Ben Eater's videos
    static constexpr Byte INS_LDY_IM = 0xA0;
    static constexpr Byte INS_LDY_ABS = 0xAC; // didnt check if opcode is correct
    static constexpr Byte INS_DEY = 0x88;
    static constexpr Byte INS_BMI = 0x30;
    static constexpr Byte INS_STA_ABSY = 0x99;
    static constexpr Byte INS_BNE = 0xD0;
    static constexpr Byte INS_INY = 0xC8;
    static constexpr Byte INS_BPL = 0x10;
    static constexpr Byte INS_TAX = 0xAA;
    static constexpr Byte INS_STA_ZP = 0x85;
    static constexpr Byte INS_LDA_ABSY = 0xB9;
    static constexpr Byte INS_BCC = 0x90;
    static constexpr Byte INS_STX_ZP = 0x86;
    static constexpr Byte INS_STY_ZP = 0x84;
};

#endif // CPU_H