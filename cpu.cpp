#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <unistd.h>

#include "memory.h"
#include "cpu.h"

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


Word CPU::FetchWord(u32& Cycles, UnifiedMemory& memory)
    {
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

Byte CPU::FetchByte(u32& Cycles, UnifiedMemory& memory) // 1 cycle
    {
        Byte Data = memory.Read(PC);
        PC++;
        Cycles--;
        return Data;
    }

Byte CPU::ReadByte (u32& Cycles, UnifiedMemory& memory, Word address) // Read one Byte from memory at address. This consumes 1 cycle
    {
        // The PC is not used to read a byte.
        Byte Data = memory.Read(address);
        Cycles--;
        return Data;
    }

void CPU::StoreByte(u32& Cycles, UnifiedMemory& memory, Byte value, Word address)
    {
        memory.Write(address ,value);
        Cycles--; // This operation takes 1 cycle
        if (DEBUG){
            std::cout << "[DEBUG] Wrote Byte to " << std::hex << address << " value " << std::dec << (int)value << std::endl;
        }
    }

void CPU::pushBytetoStack(Byte data,u32& Cycles, UnifiedMemory& memory)
    {
        // Assert that the stack pointer is not at it's maximum of 0x01FF
        memory.Write(SP | 0x0100, data);
        SP--; // the stack pointer always points to the next free space
        Cycles--;
    }

Byte CPU::pullBytefromStack(u32& Cycles, UnifiedMemory& memory)
    {
        SP++; // the stack pointer always points to the next free space
        Byte data = memory.Read(SP | 0x0100);
        Cycles--;
        return data;
    }

void CPU::SetPC_absolute(u32& Cycles, Word address)
    {
        PC = address; // may or may not have to be + or - 1
        Cycles--; // Setting the program counter takes one clock cycle
    }

void CPU::SetPC_relative(u32 & Cycles, Byte offset) { // takes 1 or 2 cycles
    Word previous_PC = PC;

    int8_t signed_offset = static_cast<int8_t>(offset); // Correct sign extension
    PC += signed_offset; // Add signed offset directly
    Cycles--; // Always consume 1 cycle

    // Check if a page boundary is crossed
    if ((previous_PC & 0xFF00) != (PC & 0xFF00)) {
        Cycles--; // Additional cycle if crossing a page
    }
}


// extract repeat code

void CPU::SetStatusNZbasedonA()
    {
        // set zero flag if neccesary
        Z = ((A==0x00) ? true : false);
        // set negative flag if neccesary
        N = (((A & 0x80) == 0x80) ? true : false);
    }

void CPU::SetStatusNZbasedonX()
    {
        // set zero flag if neccesary
        Z = ((X==0x00) ? true : false);
        // set negative flag if neccesary
        N = (((X & 0x80) == 0x80) ? true : false);
    }

void CPU::SetStatusNZbasedonY()
    {
        // set zero flag if neccesary
        Z = ((Y==0x00) ? true : false);
        // set negative flag if neccesary
        N = (((Y & 0x80) == 0x80) ? true : false);
    }

// Instruction Set funcs. These can be combined with addressing mode funcs to make the instructions

void CPU::ADC(Byte operand) // This happens internally and takes NO cycles
    {
        Word sum = (Word)A + (Word)operand + C; // We need a Word to hold the sum in order to process the overflow
        C = (sum > 0xFF ) ? 1 : 0; // Check for overflow
        Byte result = (Byte)sum; // Cast the result back to a Byte / uint_8
        Z = ( result == 0 ) ? 1 : 0; // Check for the zero flag
        N = ( result & 0b10000000 ) ? 1 : 0; // Check for the negative flag ( aka the leading bit is set)
        V = ((A ^ result ) & ( operand & result ) & 0x80 ) ? 1 : 0; 
        /* Set oVerflow flag (check for signed overflow)
        Overflow happens when the sign of A and the operand are the same, but the sign of the result is different
        idk how it works with adding including the Carry bit*/
        A = result; // Store the result in the A register
    }

void CPU::AND(Byte operand) // This happens internally and takes NO cycles
    {
        Byte result = A & operand; // bitwise AND
        A = result;
        N = (A & 0x80) ? 1 : 0;
        Z = (A == 0) ? 1 : 0;
    }

void CPU::CMP(Byte operand){
    Word temp = A - operand;
    C = (A >= operand) ? 1 : 0;
    Z = (temp == 0) ? 1 : 0;
    N = (temp & 0x80) ? 1 : 0;
}
void CPU::CPX(Byte operand){
    Word temp = X - operand;
    C = (X >= operand) ? 1 : 0;
    Z = (temp == 0) ? 1 : 0;
    N = (temp & 0x80) ? 1 : 0;
}
void CPU::SBC(Byte operand){
    Word temp = A - operand - (1 - C);
    C = (temp < 0x100) ? 1 : 0;                            // Carry is set if no borrow occurs
    Z = ((temp & 0xFF) == 0) ? 1 : 0;
    N = (temp & 0x80) ? 1 : 0;
    V = ((A ^ temp) & (A ^ operand) & 0x80) ? 1 : 0;
    A = temp & 0xFF;
}
void CPU::LDA(Byte operand){
    A = operand;
    SetStatusNZbasedonA();
}
void CPU::LDX(Byte operand){
    X = operand;
    SetStatusNZbasedonX();
}
void CPU::EOR(Byte operand){
    A = A ^ operand;
    SetStatusNZbasedonA();
} 
void CPU::CPY(Byte operant){
    Word temp = Y - operant;
    C = (Y >= operant) ? 1 : 0;
    Z = (temp == 0) ? 1 : 0;
    N = (temp & 0x80) ? 1 : 0;
}
//http://www.6502.org/tutorials/6502opcodes.html

// Addressing Mode funcs

Byte CPU::AM_IM_LOAD(u32 Cycles, UnifiedMemory& memory) // addressing mode: immediate
    {
        // takes one cycle. adds one to the program counter
        Byte operand = FetchByte(Cycles, memory); // fetch the operand
        return operand;
    }

Byte CPU::AM_ABS_LOAD(u32 Cycles, UnifiedMemory& memory) // you need to provide the address where the operand is
{
    // advances pc by 2. takes 3 cycles
    Word address = FetchWord(Cycles, memory); // it takes 2 cycles to fetch a Word;
    Byte operand = ReadByte(Cycles, memory, address); // it takes 1 cycle to fetch the Byte
    return operand;
}
Byte CPU::AM_ABSY_LOAD(u32 Cycles, UnifiedMemory& memory)
{
    // takes 3-4 cycles
    Word base_address = FetchWord(Cycles, memory); // 2 cycles
    Word address = base_address + Y; // add value of the Y register to the address
    Cycles--; // this adding of Y takes 1-2 cycles depending on wether or not the address crosses into another page

    // Byte basepage = base_address >> 8;
    // Byte resultpage = address >> 8;
    // if (basepage != resultpage){Cycles--;} // check for the crossing of the page

    return address;
}
Byte CPU::AM_ZP_LOAD(u32 Cycles, UnifiedMemory& memory)
{
    
    Byte lower_address = FetchByte(Cycles, memory); // operand indicates where in the zero page, the value is located // 1 cycle
    Word address = 0x0000 | (Word)lower_address; // happends internally so no cycle taken
    Byte operand = ReadByte(Cycles, memory, address); // 1 cycle (IO operation)
    return operand;
}

Word CPU::AM_ABSY_STORE(u32 Cycles, UnifiedMemory& memory)
{
    // takes 4 cycles
    Word base_address = FetchWord(Cycles, memory); // 2 cycles
    Word address = base_address + Y; // add value of the Y register to the address
    Cycles--; // this adding of Y takes 1-2 cycles depending on wether or not the address crosses into another page

    // Byte basepage = base_address >> 8;
    // Byte resultpage = address >> 8;
    // if (basepage != resultpage){Cycles--;} // check for the crossing of the page

    // on STORE stuff, this page crossing stuff always takes the extra cycle
    Cycles--;

    return address;
}
Word CPU::AM_ZP_STORE(u32 Cycles, UnifiedMemory& memory)
{
    // takes 1 cycles
    Byte lower_address = FetchByte(Cycles, memory); // operand indicates where in the zero page, the value is located
    Word address = 0x0000 | (Word)lower_address;
    return address;
}

// this is where the magic happens
void CPU::Execute(u32 Cycles, UnifiedMemory& memory) // Cycles: for how many clockcycles do we want to execute?
    {
        const u32 STARTCYCLES = Cycles;
        while ((Cycles > 0) && (Cycles<= STARTCYCLES))
        {
            if (DEBUG) {printf("Next instruction\n");}
            // debug info
            if (DEBUG){
                std::cout << "[DEBUG]" << Cycles << " Cycles remaining" << std::endl;
            }
            // step 1: fetch next instruction from memory
            Byte Instruction = FetchByte(Cycles, memory);

            // set 2: execute instruction. We swich here based on what instruction is fetched
            if (DEBUG){printf("Instruction: 0x%02X\n", Instruction);printf("PC: 0x%02X\n", PC-1);};

            switch (Instruction)
            {
                case INS_ADC_IM:
                {
                    Byte operand = AM_IM_LOAD(Cycles, memory); // fetch the operand
                    ADC(operand);
                }break;
                case INS_ADC_ABS:
                {
                    Byte operand = AM_ABS_LOAD(Cycles, memory);
                    ADC(operand); // extracted function. it adds and sets the flags
                }break;
                case INS_AND_IM:
                {
                    Byte operand = AM_IM_LOAD(Cycles, memory);
                    AND(operand); // do the AND stuff
                }break;
                case INS_AND_ABS:
                {
                    Byte operand = AM_ABS_LOAD(Cycles, memory);
                    AND(operand);
                }break;
                case INS_ASL_A:
                {
                    C = A & 0x80; // Store bit 7 in Carry flag
                    A =  A << 1; // shift left 1
                    Cycles--; // this costs 1 cycle
                    SetStatusNZbasedonA();
                }break;
                case INS_BEQ:
                {
                    Byte offset = FetchByte(Cycles, memory); // 1 cycle
                    if (Z == 1)
                    {
                        SetPC_relative(Cycles, offset); // 1 cycle always + 1 cycle if PAGE is crossed
                    }
                }break;
                case INS_BNE:
                {
                    Byte offset = FetchByte(Cycles, memory); // 1 cycle
                    if (Z == 0)
                    {
                        SetPC_relative(Cycles, offset); // 1 cycle always + 1 cycle if PAGE is crossed
                    }
                }break;
                case INS_BMI:
                {
                    Byte offset = FetchByte(Cycles, memory); // 1 cycle
                    if (N == 1)
                    {
                        SetPC_relative(Cycles, offset); // 1 cycle always + 1 cycle if PAGE is crossed
                    }
                }break;
                case INS_BPL:
                {
                    Byte offset = FetchByte(Cycles, memory); // 1 cycle
                    if (N == 0)
                    {
                        SetPC_relative(Cycles, offset); // 1 cycle always + 1 cycle if PAGE is crossed
                    }
                }break;
                case INS_BCC:
                {
                    Byte offset = FetchByte(Cycles, memory); // 1 cycle
                    if (C == 0)
                    {
                        SetPC_relative(Cycles, offset); // 1 cycle always + 1 cycle if PAGE is crossed
                    }
                }break;
                case INS_CMP_IM:
                {
                    Byte operand = AM_IM_LOAD(Cycles, memory);
                    CMP(operand);
                }break;
                case INS_CMP_ABS:
                {
                    Byte operand = AM_ABS_LOAD(Cycles, memory);
                    CMP(operand);
                }break;
                case INS_CPX_IM:
                {
                    Byte operand = AM_IM_LOAD(Cycles, memory);
                    CPX(operand);
                }break;
                case INS_INX:
                {
                    X += 1; // increment the X register
                    Cycles--; // increment takes 1 cycle
                    SetStatusNZbasedonX(); // setstatus
                }break;
                case INS_JMP_ABS:
                {
                    Word Address = FetchWord(Cycles, memory); // it takes 2 cycles to fetch a Word
    	            SetPC_absolute(Cycles,Address); // Set the Program Counter to the desired Address
                    // This instruction does not affect any flags.
                }break;
                case INS_JSR:
                {
                    Word address = FetchWord(Cycles, memory); // 2 cycles
                    // The SP decends, so to use little endian, it must first store the MSB. Who came up with this BS
                    Byte upperByte = PC >> 8; // extract upper Byte
                    pushBytetoStack(upperByte,Cycles,memory); // one cycle
                    Byte lowerByte = PC & 0xFF; //extract the lower Byte
                    pushBytetoStack(lowerByte,Cycles,memory); // one cycle
                    // Push return location without -1 onto the stack.
                    // in hardware, it makes sense to store PC -1. It doesn't in software.
                    // set PC to new address ( that of the subroutine ). This is an operation which can be done in a single clock cycle
                    SetPC_absolute(Cycles, address); // takes 1 cycle
                }break;
                case INS_LDA_IM: // load A immediate
                {
                    Byte operand = AM_IM_LOAD(Cycles, memory);
                    // store value in A register
                    LDA(operand);
                }break;
                case INS_LDA_ZP: // load A from zero page
                {
                    Byte operand = AM_ZP_LOAD(Cycles, memory);
                    LDA(operand);
                }break;
                case INS_LDA_ABS:
                {
                    Byte operand = AM_ABS_LOAD(Cycles, memory);
                    LDA(operand);
                }break;
                case INS_LDA_ABSY:
                {
                    Byte operand = AM_ABSY_LOAD(Cycles, memory);
                    LDA(operand);
                } break;
                case INS_LDX_IM:
                {
                    Byte operand = AM_IM_LOAD(Cycles, memory);
                    // store value in X register
                    LDX(operand);
                }break;
                case INS_LDX_ZP: // load A from zero page
                {
                    Byte operand = AM_ZP_LOAD(Cycles, memory);
                    LDX(operand);
                }break;
                case INS_NOP:
                {
                    // This means to do nothing
                    // The PC is already incremented by reading the NOP instruction and the reading already consumes a cycle
                    // printf("[INFO] NOP \n");
                    usleep(1000000);
                }break;
                case INS_PHA:
                {
                    pushBytetoStack(A, Cycles, memory); // 1 cycle
                    // this instruction is inefficient. It takes 3 cycles
                    // with JSR, it takes two cycles to push the return address (two Bytes) This is because the stack pointer is decremented first on previous cycles
                    // with PHA, there is no previous cycle to decriment during.
                    // the push is the IO operation which cannot overlap with other IO operations. but the decrement can. since its not IO
                    Cycles--;
                }break;
                case INS_PLA:
                {
                    Byte result = pullBytefromStack(Cycles, memory); // 1 cycle
                    Cycles--; // this is here for the same reason as the PHA instruction
                    A = result;
                    SetStatusNZbasedonA();
                    Cycles--; // setting A and the flags takes an additional cycle
                }break;
                case INS_SBC_IM:
                {
                    Byte operand = AM_IM_LOAD(Cycles, memory);
                    SBC(operand);
                }break;
                case INS_SBC_ABS:
                {
                    Byte operand = AM_ABS_LOAD(Cycles, memory);
                    SBC(operand);
                }break;
                case INS_STA_ABS:
                {
                    // the addressing mode is absolute? but the operand is an address. so dont use AM_ABS
                    Word address = FetchWord(Cycles,memory); // 2 cycles
                    StoreByte(Cycles,memory, A, address); // 1 cycle. Store into RAM

                }break;
                case INS_STA_ABSX:
                {
                    Word base_address = FetchWord(Cycles,memory); // 2 cycles
                    Word address = base_address + X; // add value of the X register to the address
                    Cycles--; // this adding of X takes 1-2 cycles depending on wether or not the address crosses into another page
                    Byte basepage = base_address >> 8;
                    Byte resultpage = address >> 8;
                    if (basepage != resultpage){Cycles--;} // check for the crossing of the page
                    StoreByte(Cycles, memory, A, address); // store A in the address.

                }break;
                case INS_RTS:
                {
                    Byte lowerAddress = pullBytefromStack(Cycles,memory); // takes 1 cycle
                    Byte upperAddress = pullBytefromStack(Cycles, memory); // takes 1 cycle
                    Word Address = upperAddress << 8 | lowerAddress;
                    SetPC_absolute(Cycles, Address); // takes 1 cycle

                }break;
                case INS_LSR_A:
                {
                    C = A & 0x01; // set the carry flag
                    A = A >> 1; // shift right 1
                    Cycles--;
                    SetStatusNZbasedonA();
                }break;
                case INS_BVS:
                {
                    Byte offset = FetchByte(Cycles,memory); // 1 cycle
                    if (V == 1)
                    {
                        SetPC_relative(Cycles, offset); // 1 cycle always + 1 cycle if PAGE is crossed
                    } else {
                        ;
                    }
                }break;
                case INS_DEC_A:
                {
                    A -= 1; // decrement A
                    SetStatusNZbasedonA();
                }break;
                case INS_LDY_IM: // load Y immediate
                {
                    Byte operand = AM_IM_LOAD(Cycles, memory);
                    // store value in Y register
                    Y = operand;
                    SetStatusNZbasedonY();
                }break;
                case INS_LDY_ABS: // load Y immediate
                {
                    Byte operand = AM_ABS_LOAD(Cycles, memory);
                    // store value in Y register
                    Y = operand;
                    SetStatusNZbasedonY();
                }break;
                case INS_DEY:
                {
                    Y -= 1; // decrement Y
                    SetStatusNZbasedonY();
                }break;
                case INS_INY:
                {
                    Y += 1; // decrement Y
                    SetStatusNZbasedonY();
                }break;
                case INS_STA_ABSY:
                {
                    Word address = AM_ABSY_STORE(Cycles, memory); // takes 4 cycles
                    StoreByte(Cycles, memory, A, address); // 1 cycle
                }break;
                case INS_TAX:
                {
                    X = A; // transfer A to X
                    Cycles--; // this takes 1 cycle
                    SetStatusNZbasedonX();
                }break;
                case INS_TXA:
                {
                    A = X; // transfer A to X
                    Cycles--; // this takes 1 cycle
                    SetStatusNZbasedonA();
                }break;
                case INS_STA_ZP:
                {
                    Word address = AM_ZP_STORE(Cycles, memory); // takes 1 cycle
                    StoreByte(Cycles, memory, A, address); // 1 cycle
                }break;
                case INS_STX_ZP:
                {
                    Word address = AM_ZP_STORE(Cycles, memory);
                    StoreByte(Cycles, memory, X, address);
                }break;
                case INS_STY_ZP:
                {
                    Word address = AM_ZP_STORE(Cycles, memory);
                    StoreByte(Cycles, memory, Y, address);
                }break;
                case INS_EOR_IM:
                {
                    Byte operand = AM_IM_LOAD(Cycles, memory);
                    EOR(operand);
                }break;
                case INS_EOR_ABS:
                {
                    Byte operand = AM_ABS_LOAD(Cycles, memory);
                    EOR(operand);
                }break;
                case INS_EOR_ZP:
                {
                    Byte operand = AM_ZP_LOAD(Cycles, memory);
                    EOR(operand);
                }break;
                case INS_CPY_IM:
                {
                    Byte operand = AM_IM_LOAD(Cycles, memory);
                    CPY(operand);
                }break;
                case INS_CPY_ABS:
                {
                    Byte operand = AM_ABS_LOAD(Cycles, memory);
                    CPY(operand);
                }break;
                case INS_CPY_ZP:
                {
                    Byte operand = AM_ZP_LOAD(Cycles, memory);
                    CPY(operand);
                }break;
                case INS_BRK:
                {
                    Cycles=0; // this is a software stop
                    printf("Encountered BRK instruction. Aborting...\n");
                }break;

                default:
                {
                    printf("Instruction not handled %02X \n", Instruction);
                    Cycles = 0;
                }
                break; // The instruction was not found. Make the cpu crash
                
            }
        }
    }

void CPU::dump_contents(){
    std::cout << "A: " << (int)A << std::endl;
    std::cout << "X: " << (int)X << std::endl;
    std::cout << "Y: " << (int)Y << std::endl;
    std::cout << "PC: " << std::hex << PC << std::endl;
    std::cout << "SP: " << (int)SP << std::endl;
    std::cout << "C: " << C << std::endl;
    std::cout << "Z: " << Z << std::endl;
    std::cout << "I: " << I << std::endl;
    std::cout << "D: " << D << std::endl;
    std::cout << "B: " << B << std::endl;
    std::cout << "V: " << V << std::endl;
    std::cout << "N: " << N << std::endl;
}

void CPU::store_output_file(const std::string& filename, uint16_t start, uint16_t end, UnifiedMemory& memory)
    {
        std::ofstream outputFile(filename, std::ios::binary); // open a binary file. This is something we can write to
        // check if the file was succesfully opened. Idk why this is neccesary
        if (!outputFile) { std::cerr << "Error opening file!" << std::endl; return;}
        // Write the memory contents from 0x6000 to 0x6FFF to the file
        for (Word address = start; address <= end; ++address) {
            Byte data = memory.Read(address);
            outputFile.write(reinterpret_cast<char*>(&data), sizeof(Byte));
        }
    std::cout << "Output file stored successfully." << std::endl;
    }
