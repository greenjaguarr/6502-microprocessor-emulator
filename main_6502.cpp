#include <stdio.h>
#include <stdlib.h>

// http://www.obelisk.me.uk/6502/
// https://www.youtube.com/watch?v=qJgsuQoy9bc

using u32 = unsigned int;
using Byte = unsigned char;
using Word = unsigned short;
struct Mem
{
    static constexpr u32 MAX_MEM = 65536; // range van 0x0000 tot 0xFFFF
    Byte Data[MAX_MEM];

    void Initialise()
    {
        for (u32 i = 0x0000; i<MAX_MEM; i++) 
        {
            Data[i] = 0x00;
        }
    }

    // Read one Byte from memory
    Byte operator[](u32 Address ) const
    {
        // assert here that Address is 0 <= Address < MAX_MEM
        return Data[Address];
    }
    // Write one Byte to memory
    Byte &operator[](u32 Address) 
    {
        return Data[Address];
    }
};

struct CPU
{
    
    Word PC; //program counter
    Word SP; //stack pointer

    Byte A,X,Y; // registers

    // Processor status
    Byte C : 1; // carry flag
    Byte Z : 1; // zero flag
    Byte I : 1; // interrupt disable
    Byte D : 1; // decimal mode
    Byte B : 1; // break command
    Byte V : 1; // overflow flag
    Byte N : 1; // negative flag

    void Reset(Mem &memory)
    {
        PC = 0xFFFC;
        SP = 0x0100;

        C = 0;
        Z = 0;
        I = 0;
        D = 0;
        B = 0;
        V = 0;
        N = 0;

        A = 0x00;
        X = 0x00;
        Y = 0x00;

        memory.Initialise();
    };

    Byte FetchByte(u32& Cycles, Mem& memory)
    {
        Byte Data = memory[PC];
        PC++;
        Cycles--;
        return Data;
    }

    Byte ReadByte (u32& Cycles, Mem& memory, Word address)
    {
        Byte Data = memory[address];
        Cycles--;
        return Data;
    }

    void LDASetStatus()
    {
        // set zero flag if neccesary
        Z = ((A==0x00) ? true : false);
        // set negative flag if neccesary
        N = (((A & 0x80) == 0x80) ? true : false);
    }

    static constexpr Byte INS_LDA_IM = 0xA9; // instruction load A immediate ( laad een waarde direct uit de ROM)
    static constexpr Byte INS_LDA_ZP = 0xA5; // instruction load A from zero page. 

    void Execute(u32 Cycles, Mem & memory) // Cycles: for how many clockcycles do we want to execute?
    {
        while (Cycles > 0)
        {
            // step 1: fetch next instruction from memory
            Byte Instruction = FetchByte (Cycles, memory);

            // set 2: execute instruction. We swich here based on what instruction is fetched
            switch (Instruction)
            {
                case INS_LDA_IM: // load A immediate
                {
                    Byte operand = FetchByte (Cycles, memory);
                    // store value in A register
                    A = operand;
                    LDASetStatus();
                }break;
                case INS_LDA_ZP: // load A from zero page
                {
                    Byte operand = FetchByte (Cycles, memory); // operand indicates where in the zero page, the value is located
                    Word address = 0x0000 | operand;  //yt vid doet dit niet????
                    Byte value = ReadByte(Cycles, memory, address);
                    A = value;
                    LDASetStatus();
                }break;

            default:
            {
                printf("Instruction not handled %d", Instruction);
            }
                break; // The instruction was not found. Make the cpu crash
            }
        }
    }
};

int main()
{
    Mem mem;
    CPU cpu;
    cpu.Reset(mem);

    //temporarily customize memory
    mem[0xFFFC] = CPU::INS_LDA_IM;
    mem[0xFFFD] = 200;
    mem[0xFFFE] = CPU::INS_LDA_ZP;
    mem[0xFFFF] = 0x10;

    cpu.Execute(2, mem);
    cpu.Execute(3, mem);
    return 0;

}