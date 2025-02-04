#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <vector>

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
        memory.Write(address ,value); // This takes a lot of C++ syntax to actually make happen
        Cycles--; // This operation takes 1 cycle
        if (DEBUG){
            std::cout << "[DEBUG] Wrote Byte to " << std::hex << address << " value " << std::dec << (int)value << std::endl;
        }
    }

void CPU::pushBytetoStack(Byte data,u32& Cycles, UnifiedMemory& memory)
    {
        // Assert that the stack pointer is not at it's maximum of 0x01FF
        memory.Write(SP | 0x0100, data);
        SP--;
        Cycles--;
    }

Byte CPU::pullBytefromStack(u32& Cycles, UnifiedMemory& memory)
    {
        SP++;
        Byte data = memory.Read(SP | 0x0100);
        Cycles--;
        return data;
    }

void CPU::SetPC_absolute (u32& Cycles, Word Address)
    {
        PC = Address; // may or may not have to be + or - 1
        Cycles--; // Setting the program counter takes one clock cycle
    }

void CPU::SetPC_relative(u32 & Cycles, Byte offset) // offset is signed TODO
    {
        Word previous_PC = PC;
        // offset is signed. compensate for this
        bool signBit = (offset >= 0x80); // check for the sign bit = -128 place
        if (signBit)
        {
            offset = offset && 0x7F; // remove the first bit
            PC += offset; // add the offset to theprogram counter
            PC -= 0x80; // subtract 128 from the program counter for the sign bit
            Cycles--;
        } else{
            PC += offset; // add value of the offset to jmp around in the program
            Cycles--;
        }
        // offset = offset - 0x80;} // remove the leading bit
        // PC += offset; // add the offset first
        // Cycles--; // this takes one cycle
        // if (signBit == true ) { // 2nd add the sign. This is in the same cycle as step 1
        //     PC-=128;
        // }
        // check for if a PAGE boundy is crossed
        if ((previous_PC >> 8 ) != (PC >> 8)){
            Cycles--;
        }
        // PC--; // compensate for that the PC has to run ( idk what this means when i wrote it)
    }

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
void CPU::SBC(Byte operand){
    Word temp = A - operand - (1 - C);
    C = (temp < 0x100) ? 1 : 0;                            // Carry is set if no borrow occurs
    Z = ((temp & 0xFF) == 0) ? 1 : 0;
    N = (temp & 0x80) ? 1 : 0;
    V = ((A ^ temp) & (A ^ operand) & 0x80) ? 1 : 0;
    A = temp & 0xFF;
}

//http://www.6502.org/tutorials/6502opcodes.html

Byte CPU::AM_IM(u32 Cycles, UnifiedMemory& memory) // addressing mode: immediate
    {
        // takes one cycle. adds one to the program counter
        Byte operand = FetchByte(Cycles, memory); // fetch the operand
        return operand;
    }

Byte CPU::AM_ABS(u32 Cycles, UnifiedMemory& memory) // you need to provide the address where the operand is
{
    // advances pc by 2. takes 3 cycles
    Word address = FetchWord(Cycles, memory); // it takes 2 cycles to fetch a Word;
    Byte operand = ReadByte(Cycles, memory, address); // it takes 1 cycle to fetch the Byte
}


void CPU::Execute(u32 Cycles, UnifiedMemory& memory) // Cycles: for how many clockcycles do we want to execute?
    {
        const u32 STARTCYCLES = Cycles;
        while ((Cycles > 0) && (Cycles<= STARTCYCLES))
        {
            // debug info
            if (DEBUG){
                std::cout << "[DEBUG]" << Cycles << std::endl;
            }
            // step 1: fetch next instruction from memory
            Byte Instruction = FetchByte (Cycles, memory);

            // set 2: execute instruction. We swich here based on what instruction is fetched
            switch (Instruction)
            {
                case INS_ADC_IM:
                {
                    Byte operand = AM_IM(Cycles, memory); // fetch the operand
                    ADC(operand);
                }break;
                case INS_ADC_ABS:
                {
                    Byte operand = AM_ABS(Cycles, memory);
                    ADC(operand); // extracted function. it adds and sets the flags
                }break;
                case INS_AND_IM:
                {
                    Byte operand = AM_IM(Cycles, memory);
                    AND(operand); // do the AND stuff
                }break;
                case INS_AND_ABS:
                {
                    Byte operand = AM_ABS(Cycles, memory);
                    AND(operand);
                }break;
                case INS_ASL_A:
                {
                    C = A & 0x80; // Store bit 7 in Carry flag
                    A =  A << 1; // shift left 1
                    Cycles--; // this costs 1 cycle
                    SetStatusNZbasedonA();
                }break;
                case INS_CMP_IM:
                {
                    Byte operand = AM_IM(Cycles, memory);
                    CMP(operand);
                }break;
                case INS_CMP_ABS:
                {
                    Byte operand = AM_ABS(Cycles, memory);
                    CMP(operand);
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
                    Word Address = FetchWord(Cycles, memory); // 2 cycles
                    // The SP decends, so to use little endian, it must first store the MSB. Who came up with this BS
                    Byte upperByte = PC >> 8; // extract upper Byte
                    pushBytetoStack(upperByte,Cycles,memory); // one cycle
                    Byte lowerByte = PC & 0xFF; //extract the lower Byte
                    pushBytetoStack(lowerByte,Cycles,memory); // one cycle
                    // Push return location without -1 onto the stack.
                    // in hardware, it makes sense to store PC -1. It doesn't in software.
                    // set PC to new address ( that of the subroutine ). This is an operation which can be done in a single clock cycle
                    PC = Address;
                    Cycles--;
                }break;
                case INS_LDA_IM: // load A immediate
                {
                    Byte operand = AM_IM(Cycles, memory);
                    // store value in A register
                    A = operand;
                    SetStatusNZbasedonA();
                }break;
                case INS_LDA_ZP: // load A from zero page
                {
                    Byte lower_address = FetchByte (Cycles, memory); // operand indicates where in the zero page, the value is located
                    Word address = 0x0000 | (Word)lower_address;  //yt vid doet dit niet????
                    Byte operand = ReadByte(Cycles, memory, address);
                    A = operand;
                    SetStatusNZbasedonA();
                }break;
                case INS_LDA_ABS:
                {
                    Byte operand = AM_ABS(Cycles, memory);
                    A = operand;
                    SetStatusNZbasedonA();
                }break;
                case INS_LDX_IM:
                {
                    Byte operand = AM_IM(Cycles, memory);
                    // store value in X register
                    X = operand;
                    SetStatusNZbasedonX();
                }break;
                case INS_NOP:
                {
                    // This means to do nothing
                    // The PC is already incremented by reading the NOP instruction and the reading already consumes a cycle
                }break;
                case INS_SBC_IM:
                {
                    Byte operand = AM_IM(Cycles, memory);
                    SBC(operand);
                }break;
                case INS_SBC_ABS:
                {
                    Byte operand = AM_ABS(Cycles, memory);
                    SBC(operand);
                }
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

            default:
            {
                printf("Instruction not handled %d", Instruction);
            }
                break; // The instruction was not found. Make the cpu crash
                
            }
        }

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
