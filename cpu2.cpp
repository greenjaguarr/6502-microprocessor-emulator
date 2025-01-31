#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <vector>

#include "memory.h"
#include "cpu2.h"

// http://www.obelisk.me.uk/6502/
// https://www.youtube.com/watch?v=qJgsuQoy9bc
// http://www.6502.org/tutorials/6502opcodes.html

using u32 = unsigned int;
using Byte = unsigned char;
using Word = unsigned short;

#define DEBUG false

void CPU::Reset(UnifiedMemory &memory) {
    PC = 0xFFFC;
    SP = 0xFF;

    // Reset flags and registers
    C = Z = I = D = B = V = N = 0;
    A = X = Y = 0;

    // Read the reset vector (stored in ROM at 0xFFFC and 0xFFFD)
    Byte lowerStartAddress = memory.Read(0xFFFC);
    Byte upperStartAddress = memory.Read(0xFFFD);
    PC = (upperStartAddress << 8) | lowerStartAddress;
}

Word CPU::FetchWord(u32& Cycles, UnifiedMemory& memory) {
    //6502 is little endian
    //First grab the low byte
    Word Data = memory.Read(PC); // It doesnt matter that memory[] only gives a byte
    PC++;
    Cycles--;
    // Next we grab the high byte. To combine, we use the OR operator
    Data |= (memory.Read(PC) << 8);
    PC++;
    Cycles--;
    return Data;
}

Byte CPU::FetchByte(u32& Cycles, UnifiedMemory& memory) {
    Byte Data = memory.Read(PC);
    PC++;
    Cycles--;
    return Data;
}

Byte CPU::ReadByte(u32& Cycles, UnifiedMemory& memory, Word address) {
    Byte Data = memory.Read(address);
    Cycles--;
    return Data;
}

void CPU::StoreByte(u32& Cycles, UnifiedMemory& memory, Byte value, Word address) {
    memory.Write(address, value);
    Cycles--;
    if (DEBUG) {
        std::cout << "[DEBUG] Wrote Byte to " << std::hex << address << " value " << std::dec << (int)value << std::endl;
    }
}

void CPU::pushBytetoStack(Byte data, u32& Cycles, UnifiedMemory& memory) {
    memory.Write(SP | 0x0100, data);
    SP--;
    Cycles--;
}

Byte CPU::pullBytefromStack(u32& Cycles, UnifiedMemory& memory) {
    SP++;
    Byte data = memory.Read(SP | 0x0100);
    Cycles--;
    return data;
}

void CPU::SetPC_absolute(u32& Cycles, Word Address) {
    PC = Address;
    Cycles--;
}

void CPU::SetPC_relative(u32& Cycles, Byte offset) {
    Word previous_PC = PC;
    bool signBit = (offset >= 0x80);
    if (signBit) {
        offset = offset & 0x7F;
        PC += offset;
        PC -= 0x80;
        Cycles--;
    } else {
        PC += offset;
        Cycles--;
    }
    if ((previous_PC >> 8) != (PC >> 8)) {
        Cycles--;
    }
}

void CPU::SetStatusNZbasedonA() {
    Z = ((A == 0x00) ? true : false);
    N = (((A & 0x80) == 0x80) ? true : false);
}

void CPU::SetStatusNZbasedonX() {
    Z = ((X == 0x00) ? true : false);
    N = (((X & 0x80) == 0x80) ? true : false);
}

void CPU::ADC(Byte operand) {
    Word sum = A + operand + C;
    C = (sum > 0xFF) ? 1 : 0;
    Byte result = sum & 0xFF;
    Z = (result == 0) ? 1 : 0;
    N = (result & 0x80) ? 1 : 0;
    V = ((A ^ result) & (operand & result) & 0x80) ? 1 : 0;
    A = result;
}

void CPU::AND(Byte operand) {
    Byte result = A & operand;
    A = result;
    N = (A & 0x80) ? 1 : 0;
    Z = (A == 0) ? 1 : 0;
}

void CPU::Execute(u32 Cycles, UnifiedMemory& memory) {
    const u32 STARTCYCLES = Cycles;
    while ((Cycles > 0) && (Cycles <= STARTCYCLES)) {
        if (DEBUG) {
            std::cout << "[DEBUG]" << Cycles << std::endl;
        }
        Byte Instruction = FetchByte(Cycles, memory);
        switch (Instruction) {
            case INS_ADC_IM: {
                Byte operand = FetchByte(Cycles, memory);
                ADC(operand);
            } break;
            case INS_ADC_ABS: {
                Word Address = FetchWord(Cycles, memory);
                Byte operand = ReadByte(Cycles, memory, Address);
                ADC(operand);
            } break;
            case INS_AND_IM: {
                Byte operand = FetchByte(Cycles, memory);
                AND(operand);
            } break;
            case INS_AND_ABS: {
                Word address = FetchWord(Cycles, memory);
                Byte operand = ReadByte(Cycles, memory, address);
                AND(operand);
            } break;
            case INS_ASL_A: {
                C = A & 0x80;
                A = A << 1;
                Cycles--;
                SetStatusNZbasedonA();
            } break;
            case INS_INX: {
                X += 1;
                Cycles--;
                SetStatusNZbasedonX();
            } break;
            case INS_JMP_ABS: {
                Word Address = FetchWord(Cycles, memory);
                SetPC_absolute(Cycles, Address);
            } break;
            case INS_JSR: {
                Word Address = FetchWord(Cycles, memory);
                Byte upperByte = PC >> 8;
                pushBytetoStack(upperByte, Cycles, memory);
                Byte lowerByte = PC & 0xFF;
                pushBytetoStack(lowerByte, Cycles, memory);
                PC = Address;
                Cycles--;
            } break;
            case INS_LDA_IM: {
                Byte operand = FetchByte(Cycles, memory);
                A = operand;
                SetStatusNZbasedonA();
            } break;
            case INS_LDA_ZP: {
                Byte operand = FetchByte(Cycles, memory);
                Word address = 0x0000 | operand;
                Byte value = ReadByte(Cycles, memory, address);
                A = value;
                SetStatusNZbasedonA();
            } break;
            case INS_LDA_ABS: {
                Word Address = FetchWord(Cycles, memory);
                Byte value = ReadByte(Cycles, memory, Address);
                A = value;
                SetStatusNZbasedonA();
            } break;
            case INS_LDX_IM: {
                Byte operand = FetchByte(Cycles, memory);
                X = operand;
                SetStatusNZbasedonX();
            } break;
            case INS_NOP: {
            } break;
            case INS_STA_ABS: {
                Word address = FetchWord(Cycles, memory);
                StoreByte(Cycles, memory, A, address);
            } break;
            case INS_STA_ABSX: {
                Word base_address = FetchWord(Cycles, memory);
                Word Address = base_address + X;
                Cycles--;
                Byte basepage = base_address >> 8;
                Byte resultpage = Address >> 8;
                if (basepage != resultpage) {
                    Cycles--;
                }
                StoreByte(Cycles, memory, A, Address);
            } break;
            case INS_RTS: {
                Byte lowerAddress = pullBytefromStack(Cycles, memory);
                Byte upperAddress = pullBytefromStack(Cycles, memory);
                Word Address = upperAddress << 8 | lowerAddress;
                SetPC_absolute(Cycles, Address);
            } break;
            case INS_LSR_A: {
                C = A & 0x01;
                A = A >> 1;
                Cycles--;
                SetStatusNZbasedonA();
            } break;
            case INS_BVS: {
                Byte offset = FetchByte(Cycles, memory);
                if (V == 1) {
                    SetPC_relative(Cycles, offset);
                }
            } break;
            default: {
                printf("Instruction not handled %d", Instruction);
            } break;
        }
    }
}

void CPU::store_output_file(const std::string& filename, uint16_t start, uint16_t end, UnifiedMemory& memory) {
    std::ofstream outputFile(filename, std::ios::binary);
    if (!outputFile) {
        std::cerr << "Error opening file!" << std::endl;
        return;
    }
    for (Word address = start; address <= end; ++address) {
        Byte data = memory.Read(address);
        outputFile.write(reinterpret_cast<char*>(&data), sizeof(Byte));
    }
    std::cout << "Output file stored successfully." << std::endl;
}