#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <vector>

// http://www.obelisk.me.uk/6502/
// https://www.youtube.com/watch?v=qJgsuQoy9bc
// http://www.6502.org/tutorials/6502opcodes.html

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

        std::string filename = "memory.bin";
        if (LoadFromFile(filename, 0x8000)) // Load file at address 0x8000
        {
            std::cout << "Memory loaded successfully from " << filename << std::endl;
        }
        else
        {
            std::cerr << "Failed to load memory from file." << std::endl;
        }
    }

    // Load binary file into memory
    bool LoadFromFile(const std::string& filename, u32 startAddress = 0)
    {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            std::cerr << "Error: Could not open file " << filename << std::endl;
            return false;
        }

        // Get file size
        std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        if (startAddress + fileSize > MAX_MEM)
        {
            std::cerr << "Error: File size exceeds available memory." << std::endl;
            return false;
        }

        // Read file content into memory
        if (!file.read(reinterpret_cast<char*>(&Data[startAddress]), fileSize))
        {
            std::cerr << "Error: Could not read file into memory." << std::endl;
            return false;
        }

        return true;
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
    Byte SP; //stack pointer. Even though memory is the size of a word, the first to hexadecimals of the SP are always 00, so it can be stored in only a singe byte.

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
        SP = 0xFF;

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




        // Do the startup sequence. Now i am just cheating by directly reading where the PC should start
        Byte lowerStartAddress = memory[0xFFFC];
        Byte upperStartAddress = memory[0xFFFD];
        Word startAddress = (upperStartAddress << 8) | lowerStartAddress;
        PC = startAddress;
    };

    Word FetchWord(u32& Cycles, Mem& memory)
    {
        //6502 is little endian
        //First grab the low byte
        Word Data = memory[PC]; // It doesnt matter that memory[] only gives a byte
        PC++;
        Cycles--;
        // Next we grab the high byte. To combine, we use the OR operator
        Data |= (memory[PC] << 8);
        PC++;
        Cycles--;
        return Data;
    }

    Byte FetchByte(u32& Cycles, Mem& memory)
    {
        Byte Data = memory[PC];
        PC++;
        Cycles--;
        return Data;
    }

    Byte ReadByte (u32& Cycles, Mem& memory, Word address) // Read one Byte from memory at address. This consumes 1 cycle
    {
        // The PC is not used to read a byte.
        Byte Data = memory[address];
        Cycles--;
        return Data;
    }

    void StoreByte(u32& Cycles, Mem& memory, Byte value, Word address)
    {
        memory[address] = value; // This takes a lot of C++ syntax to actually make happen
        Cycles--; // This operation takes 1 cycle
    }

    void pushBytetoStack(Byte data,u32& Cycles, Mem& memory)
    {
        // Assert that the stack pointer is not at it's maximum of 0x01FF
        memory[SP | 0x0100] = data;
        SP--;
        Cycles--;
    }

    Byte pullBytefromStack(u32& Cycles, Mem& memory)
    {
        SP++;
        Byte data = memory[SP | 0x0100];
        Cycles--;
        return data;
    }

    void SetPC (u32& Cycles, Word Address)
    {
        PC = Address; // may or may not have to be + or - 1
        Cycles--; // Setting the program counter takes one clock cycle
    }

    void SetStatusNZbasedonA()
    {
        // set zero flag if neccesary
        Z = ((A==0x00) ? true : false);
        // set negative flag if neccesary
        N = (((A & 0x80) == 0x80) ? true : false);
    }

    void SetStatusNZbasedonX()
    {
        // set zero flag if neccesary
        Z = ((X==0x00) ? true : false);
        // set negative flag if neccesary
        N = (((X & 0x80) == 0x80) ? true : false);
    }

    void ADC(Byte operand) // This happens internally and takes NO cycles
    {
        Word sum = A + operand + C; // We need a Word to hold the sum in order to process the overflow
        C = (sum > 0xFF ) ? 1 : 0; // Check for overflow
        Byte result = sum& 0xFF; // Cast the result back to a Byte / uint_8
        Z = ( result == 0 ) ? 1 : 0; // Check for the zero flag
        N = ( result & 0x80 ) ? 1 : 0; // Check for the negative flag ( aka the leading bit is set)
        V = ((A ^ result ) & ( operand & result ) & 0x80 ) ? 1 : 0;
        /* Set oVerflow flag (check for signed overflow)
        Overflow happens when the sign of A and the operand are the same, but the sign of the result is different
        idk how it works with adding including the Carry bit*/
        A = result; // Store the result in the A register
    }

    void AND(Byte operand) // This happens internally and takes NO cycles
    {
        Byte result = A & operand; // bitwise AND
        A = result;
        N = (A & 0x80) ? 1 : 0;
        Z = (A == 0) ? 1 : 0;
    }
//http://www.6502.org/tutorials/6502opcodes.html

    static constexpr Byte INS_LDA_IM = 0xA9; // instruction load A immediate                2 bytes 2 cycles
    static constexpr Byte INS_LDA_ZP = 0xA5; // instruction load A from zero page.          2 bytes 3 cycles
    static constexpr Byte INS_JMP_ABS = 0x4C; //Jump to absolute address.                   3 bytes 3 cycles
    static constexpr Byte INS_JSR = 0x20; // Jump to SubRoutine.                            3 bytes 6 cycles
    static constexpr Byte INS_NOP = 0xEA; // The no-op instruction                          1 byte 1 cycle
    static constexpr Byte INS_ADC_IM = 0x69; // add with carry immediate.                   2 bytes 2 cycles 
    static constexpr Byte INS_ADC_ABS = 0x6D; // add with carry absolute address.           3 bytes 4 cycles 
    static constexpr Byte INS_AND_IM = 0x29; // AND the value of A with an immediate value. 2 bytes 2 cycles
    static constexpr Byte INS_AND_ABS = 0x2D; // AND A with the value in the address        3 bytes 4 cycles
    static constexpr Byte INS_ASL_A = 0x0A; // arithmatic shift left. bit0=0, C=bit7        1 byte  2 cycles
    static constexpr Byte INS_STA_ABS = 0x8D; // store A intoRAM(TODOmakeonlytop8Kwritable) 3 bytes 4 cycles
    static constexpr Byte INS_RTS = 0x60; // return from subroutine. load PC from stack.    1 byte  6 cycles
    static constexpr Byte INS_LSR_A = 0x4A; // logical shift right. bit7=0 C=bit0           1 byte  2 cycles
    static constexpr Byte INS_STA_ABSX = 0x9D; //sta a address abs + the value in the x reg 3 bytes 5 cycles
    static constexpr Byte INS_LDX_IM = 0xA2; // load X with immediate value. loops?         2 bytes 2 cycles
    static constexpr Byte INS_INX = 0xE8; // increment X register                           1 btye  2 cycles
    static constexpr Byte INS_LDA_ABS = 0xAD; // load A abs                                 3 bytes 4 cycles

    void Execute(u32 Cycles, Mem & memory) // Cycles: for how many clockcycles do we want to execute?
    {
        const u32 STARTCYCLES = Cycles;
        while ((Cycles > 0) && (Cycles<= STARTCYCLES))
        {
            // debug info
            // std::cout << "[DEBUG]" << Cycles << std::endl;
            // step 1: fetch next instruction from memory
            Byte Instruction = FetchByte (Cycles, memory);

            // set 2: execute instruction. We swich here based on what instruction is fetched
            switch (Instruction)
            {
                case INS_ADC_IM:
                {
                    Byte operand = FetchByte(Cycles, memory); // consumes 1 cycle/ The first cycle was consumed by fetching hte instruction
                    ADC(operand);
                }break;
                case INS_ADC_ABS:
                {
                    Word Address = FetchWord(Cycles, memory); // it takes 2 cycles to fetch a Word;
                    Byte operand = ReadByte(Cycles, memory, Address); // it takes 1 cycle to fetch the Byte
                    ADC(operand); // extracted function. it adds and sets the flags
                }break;
                case INS_AND_IM:
                {
                    Byte operand = FetchByte(Cycles, memory); // 1 cycle
                    AND(operand); // do the AND stuff
                }break;
                case INS_AND_ABS:
                {
                    Word address = FetchWord(Cycles, memory); // 2 cycles
                    Byte operand = ReadByte(Cycles, memory, address); // 1 cycle
                    AND(operand);
                }break;
                case INS_ASL_A:
                {
                    C = A & 0x80; // Store bit 7 in Carry flag
                    A =  A << 1; // shift left 1
                    Cycles--; // this costs 1 cycle
                    SetStatusNZbasedonA();
                    
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
    	            SetPC(Cycles,Address); // Set the Program Counter to the desired Address
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
                    Byte operand = FetchByte (Cycles, memory);
                    // store value in A register
                    A = operand;
                    SetStatusNZbasedonA();
                }break;
                case INS_LDA_ZP: // load A from zero page
                {
                    Byte operand = FetchByte (Cycles, memory); // operand indicates where in the zero page, the value is located
                    Word address = 0x0000 | operand;  //yt vid doet dit niet????
                    Byte value = ReadByte(Cycles, memory, address);
                    A = value;
                    SetStatusNZbasedonA();
                }break;
                case INS_LDA_ABS:
                {
                    Word address = FetchWord(Cycles, memory); // Fetch the address where the value is located. 2 cycles
                    Byte value = ReadByte(Cycles, memory, address); // 1 cycle
                    A = value;
                    SetStatusNZbasedonA();
                }break;
                case INS_LDX_IM:
                {
                    Byte operand = FetchByte (Cycles, memory);
                    // store value in X register
                    X = operand;
                    SetStatusNZbasedonX();
                }break;
                case INS_NOP:
                {
                    // This means to do nothing
                    // The PC is already incremented by reading the NOP instruction and the reading already consumes a cycle
                }break;
                case INS_STA_ABS:
                {
                    Word address = FetchWord(Cycles,memory); // 2 cycles
                    StoreByte(Cycles,memory, A, address); // 1 cycle. Store into RAM
                    // TODO make only RAM = 0x0000-0x7FFF writable
                    // ROM = 0x8000-0xFFFF is read-only

                }break;
                case INS_STA_ABSX:
                {
                    Word base_address = FetchWord(Cycles,memory); // 2 cycles
                    Word Address = base_address + X; // add value of the X register to the address
                    Cycles--; // this adding of X takes 1-2 cycles depending on wether or not the address crosses into another page
                    Byte basepage = base_address >> 8;
                    Byte resultpage = Address >> 8;
                    if (basepage != resultpage){Cycles--;} // check for the crossing of the page
                    StoreByte(Cycles, memory, A, Address); // store A in the address.

                }break;
                case INS_RTS:
                {
                    Byte lowerAddress = pullBytefromStack(Cycles,memory); // takes 1 cycle
                    Byte upperAddress = pullBytefromStack(Cycles, memory); // takes 1 cycle
                    Word Address = upperAddress << 8 | lowerAddress;
                    SetPC(Cycles, Address); // takes 1 cycle

                }break;
                case INS_LSR_A:
                {
                    C = A & 0x01; // set the carry flag
                    A = A >> 1; // shift right 1
                    Cycles--;
                    SetStatusNZbasedonA();
                }break;

            default:
            {
                printf("Instruction not handled %d", Instruction);
            }
                break; // The instruction was not found. Make the cpu crash
            }
        }

    }

    void store_output_file(Mem & memory)
    {
        std::ofstream outputFile("output.bin", std::ios::binary); // open a binary file. This is something we can write to
        // check if the file was succesfully opened. Idk why this is neccesary
        if (!outputFile) { std::cerr << "Error opening file!" << std::endl; return;}
        // write the stuff
        outputFile.write(reinterpret_cast<const char*> (&memory[0x6000]), 0x1000);
        outputFile.close();
        return;
    }
};



int main()
{
    Mem mem;
    CPU cpu;
    cpu.Reset(mem);

    cpu.Execute(500, mem); // dit moet op dit moment het precieze aantal clock cycles weten van tevoren.
    
    // In this CPU, memory address 0x6000-0x6FFF is reserved for output.
    cpu.store_output_file(mem);
    
    return 0;

}