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

#define DEBUG false


class Mem {
protected:
    std::vector<Byte> memory;
    Word start_address, end_address;
public:
    Mem(Word start, Word end) 
        : start_address(start), end_address(end), memory(end - start + 1, 0) {}

    virtual Byte Read(Word address) {
        if (address < start_address || address > end_address) {
            throw std::out_of_range("Address out of range");
        }
        return memory[address - start_address];
    }

    virtual void Write(Word address, Byte value) {
        if (address < start_address || address > end_address) {
            throw std::out_of_range("Address out of range");
        }
        memory[address - start_address] = value;
    }

    // Load binary file into ROM
    bool LoadFromFile() {
        const std::string& filename = "memory.bin";
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filename << std::endl;
            return false;
        }

        // Get file size
        std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg);



        std::cout << "Memory loaded successfully from " << filename << std::endl;
        return true;
    }



    virtual ~Mem() = default;
};

class RAM : public Mem {
public:
    RAM(Word start, Word end) : Mem(start, end) {}
};

class ROM : public Mem {
public:
    ROM(Word start, Word end) : Mem(start, end) {}

    void Write(Word address, Byte value) override {
        throw std::runtime_error("Cannot write to ROM");
    }

    void LoadData(const std::vector<Byte>& data) {
        if (data.size() > memory.size()) {
            throw std::runtime_error("Data too large for ROM");
        }
        std::copy(data.begin(), data.end(), memory.begin());
    }
};

class UnifiedMemory {
    RAM ram;       // Example: RAM covering 0x0000 to 0x1FFF
    ROM rom;       // Example: ROM covering 0xC000 to 0xFFFF
    // std::unordered_map<Word, Byte> ioRegisters; // Simulated I/O registers

public:
    UnifiedMemory()
        : ram(0x0000, 0x3FFF), rom(0x8000, 0xFFFF) {}

    Byte Read(Word address) {
        if (address <= 0x3FFF) {
            return ram.Read(address);
        // } else if (address >= 0x2000 && address <= 0x3FFF) {
            // I/O Registers
            // return ioRegisters[address % 8]; // Example: NES-like mirroring
        } else if (address >= 0x8000) {
            // ROM
            return rom.Read(address);
        } else {
            throw std::out_of_range("Address not mapped");
        }
    }

    void Write(Word address, Byte value) {
        if (address <= 0x3FFF) {
            // RAM with mirroring
            ram.Write(address, value);
        // } else if (address >= 0x2000 && address <= 0x3FFF) {
            // I/O Registers
            // ioRegisters[address % 8] = value;
        } else if (address >= 0x8000) {
            // ROM is read-only
            throw std::runtime_error("Cannot write to ROM");
        } else {
            throw std::out_of_range("Address not mapped");
        }
    }

    bool LoadROM(const std::string &filename) {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filename << std::endl;
            return false;
        }

        std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        // if (fileSize > rom.GetSize()) {
        //     std::cerr << "Error: ROM file is too large" << std::endl;
        //     return false;
        // }

        std::vector<Byte> buffer(fileSize);
        if (file.read(reinterpret_cast<char *>(buffer.data()), fileSize)) {
            rom.LoadData(buffer);
            return true;
        }

        return false;
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

    void Reset(UnifiedMemory &memory) {
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


    Word FetchWord(u32& Cycles, UnifiedMemory& memory)
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

    Byte FetchByte(u32& Cycles, UnifiedMemory& memory) // 1 cycle
    {
        Byte Data = memory.Read(PC);
        PC++;
        Cycles--;
        return Data;
    }

    Byte ReadByte (u32& Cycles, UnifiedMemory& memory, Word address) // Read one Byte from memory at address. This consumes 1 cycle
    {
        // The PC is not used to read a byte.
        Byte Data = memory.Read(address);
        Cycles--;
        return Data;
    }

    void StoreByte(u32& Cycles, UnifiedMemory& memory, Byte value, Word address)
    {
        memory.Write(address ,value); // This takes a lot of C++ syntax to actually make happen
        Cycles--; // This operation takes 1 cycle
        if (DEBUG){
            std::cout << "[DEBUG] Wrote Byte to " << std::hex << address << " value " << std::dec << (int)value << std::endl;
        }
    }

    void pushBytetoStack(Byte data,u32& Cycles, UnifiedMemory& memory)
    {
        // Assert that the stack pointer is not at it's maximum of 0x01FF
        memory.Write(SP | 0x0100, data);
        SP--;
        Cycles--;
    }

    Byte pullBytefromStack(u32& Cycles, UnifiedMemory& memory)
    {
        SP++;
        Byte data = memory.Read(SP | 0x0100);
        Cycles--;
        return data;
    }

    void SetPC_absolute (u32& Cycles, Word Address)
    {
        PC = Address; // may or may not have to be + or - 1
        Cycles--; // Setting the program counter takes one clock cycle
    }

    void SetPC_relative(u32 & Cycles, Byte offset) // offset is signed TODO
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
    static constexpr Byte INS_BVS = 0x70; // branch if overflow is set                      2 bytes weird amount of cycles



    void Execute(u32 Cycles, UnifiedMemory& memory) // Cycles: for how many clockcycles do we want to execute?
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
                    Word Address = FetchWord(Cycles, memory); // Fetch the address where the value is located. 2 cycles
                    Byte value = ReadByte(Cycles, memory, Address); // 1 cycle
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

    void store_output_file(UnifiedMemory & memory)
    {
        std::ofstream outputFile("output.bin", std::ios::binary); // open a binary file. This is something we can write to
        // check if the file was succesfully opened. Idk why this is neccesary
        if (!outputFile) { std::cerr << "Error opening file!" << std::endl; return;}
        // Write the memory contents from 0x6000 to 0x6FFF to the file
        for (Word address = 0x2000; address <= 0x3FFF; ++address) {
            Byte data = memory.Read(address);
            outputFile.write(reinterpret_cast<char*>(&data), sizeof(Byte));
        }
    std::cout << "Output file stored successfully." << std::endl;
    }
};


int main() {
    // Initialize UnifiedMemory which combines ROM, RAM, and I/O
    UnifiedMemory memory;

    // Load the ROM into memory
    if (!memory.LoadROM("memory.bin")) {
        std::cerr << "Failed to load ROM data. Exiting." << std::endl;
        return 1;
    }

    // Initialize the CPU and reset it
    CPU cpu;
    cpu.Reset(memory);

    // Execute a certain number of clock cycles
    int cycles = 500;  // Adjust the number of cycles to simulate
    cpu.Execute(cycles, memory); // You can modify Execute() to accept memory

    // Store the output in a file from memory region 0x6000-0x6FFF
    cpu.store_output_file(memory);

    return 0;
};
