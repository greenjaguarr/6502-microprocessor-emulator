#include <stdio.h>
#include <stdlib.h>

// http://www.obelisk.me.uk/6502/
// https://www.youtube.com/watch?v=qJgsuQoy9bc

using u32 = unsigned int;
using Byte = unsigned char;
using Word = unsigned short;
struct Mem
{
    static constexpr u32 MAX_MEM = 0xFFFF;
    Byte Data[MAX_MEM];
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

    void Reset()
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
    };
};

int main()
{

    CPU cpu;
    return 0;

}