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

    static constexpr Byte INS_LDA_IM = 0xA9; // instruction load A immediate ( laad een waarde direct uit de ROM)

    void Execute(u32 Cycles, Mem & memory) // Cycles: for how many clockcycles do we want to execute?
    {
        while (Cycles > 0)
        {
            // step 1: fetch next instruction from memory
            Byte Instruction = FetchByte (Cycles, memory);

            // set 2: execute instruction. We swich here based on what instruction is fetched
            switch (Instruction)
            {
                case INS_LDA_IM:
                {
                    Byte operand = FetchByte (Cycles, memory);
                    // store value in A register
                    A = operand;
                    // set zero flag if neccesary
                    Z = ((A==0x00) ? true : false);
                    N = (((A & 0x80) == 0x80) ? true : false);
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
    cpu.Execute(2, mem);
    return 0;

}