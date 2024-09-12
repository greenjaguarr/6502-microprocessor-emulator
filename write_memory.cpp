// write an array of memory that is exactly 65K (the number that is a power of 2) long

using Byte = unsigned char;
using u32 = unsigned int;
const u32 MAX_MEM = 65536;

int main()
{
    //initialise
    Byte memory[MAX_MEM];
    for (int i = 0; i<MAX_MEM; i++)
    {
        memory[i] = 0x00;
    }
    // 0x0000 - 0x3FFF is RAM
    // 0x0000 - 0x00FF is the zero page
    // 0x0100 - 0x01FF is the stack memory

    // 0x4000 - 0x7FFF is unused ( as of now)

    // 0x8000 - 0xFFFF is ROM memory. This contains the program
    memory[0x8000] = 0xA9;
    memory[0x8001] = 0x55;

    // 0xFFFC & 0xFFFD contain the reset vector (little endian)
    memory[0xFFFC] = 0x80;
    memory[0xfffd] = 0x00;
    return 0;
}
