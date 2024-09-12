#include <fstream>
#include <iostream>

using namespace std;

using Byte = unsigned char;
using u32 = unsigned int;
const u32 MAX_MEM = 65536/2;

int main()
{
    // Initialize memory array
    Byte memory[MAX_MEM];
    for (u32 i = 0; i < MAX_MEM; i++)
    {
        memory[i] = 0xEA; // The default option is NOP ( no operation ) this simple skips a clockcycle
    }
    
    // Memory layout:
    // 0x0000 - 0x3FFF is RAM
    // 0x0000 - 0x00FF is the zero page
    // 0x0100 - 0x01FF is the stack memory
    // 0x4000 - 0x7FFF is unused (as of now)
    // 0x8000 - 0xFFFF is ROM memory. This contains the program.

    // Example program in ROM memory (starting at 0x8000):
    memory[0x0000] = 0xA9; // LDA #$55
    memory[0x0001] = 0x55; // Load 0x55 into A

    // Set the Reset Vector (little-endian) to 0x8000
    memory[0x7FFC] = 0x00;
    memory[0x7FFD] = 0x80;

    // Open a binary file for writing (truncate if exists)
    fstream file("memory.bin", ios::out | ios::binary | ios::trunc);
    if (!file)
    {
        cerr << "Error: Could not open file for writing." << endl;
        return 1;
    }

    // Write the entire memory array to the file
    file.write(reinterpret_cast<const char*>(memory), MAX_MEM);

    if (!file.good())
    {
        cerr << "Error: Could not write to file." << endl;
        return 1;
    }

    file.close();
    cout << "Memory written successfully to memory.bin" << endl;

    return 0;
}
