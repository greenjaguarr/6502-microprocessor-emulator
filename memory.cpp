#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream> // this is for the character input stream i thinkclear

#include "memory.h"
// http://www.obelisk.me.uk/6502/
// https://www.youtube.com/watch?v=qJgsuQoy9bc
// http://www.6502.org/tutorials/6502opcodes.html

using u32 = unsigned int;
using Byte = unsigned char;
using Word = unsigned short;

#define DEBUG false

int in_fd = -1;
int out_fd = -1;
int infile_fd = -1;

// class Mem {
// protected:
// std::vector<Byte> memory;
// Word start_address, end_address;
// public:
Mem::Mem(Word start, Word end)  // abstract base class for RAM and ROM
        : start_address(start), end_address(end), memory(end - start + 1, 0) {}

Byte Mem::Read(Word address) {
        if (address < start_address || address > end_address) {
            throw std::out_of_range("Address out of range");
        }
        return memory[address - start_address];
    }

void Mem::Write(Word address, Byte value) {
        if (address < start_address || address > end_address) {
            throw std::out_of_range("Address out of range");
        }
        memory[address - start_address] = value;
    }

    // Load binary file into ROM
bool Mem::LoadFromFile(const std::string& filename) {
        // const std::string& filename = "memory.bin";
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



RAM::RAM(Word start, Word end) : Mem(start, end) {}


ROM::ROM(Word start, Word end) : Mem(start, end) {}

void ROM::Write(Word address, Byte value) {
        throw std::runtime_error("Cannot write to ROM");
    }

void ROM::LoadData(const std::vector<Byte>& data) {
        if (data.size() > memory.size()) {
            throw std::runtime_error("Data too large for ROM");
        }
        std::copy(data.begin(), data.end(), memory.begin());
    }

ExternalRegister::ExternalRegister(Word addr)
        {
            address = addr;
            value = 0;
        }
Byte ExternalRegister::Read() {
    return value;
}
void ExternalRegister::Write(Byte newValue) {
    value = newValue;
}   

UnifiedMemory::UnifiedMemory()
        : ram(0x0000, 0x3FFF), rom(0x8000, 0xFFFF), 
        CHAR_IO(0x5000), INSTREAM(0x6008),
         INSTATUS(0x6009), OUTSTREAM(0x600A), 
         OUTSTATUS(0x600B), FILEIN(0x6004), STDOUT(0x6005)
        {

            out_fd = open("out_stream", O_WRONLY | O_NONBLOCK);
            in_fd = open("in_stream", O_RDONLY | O_NONBLOCK);
            infile_fd = open("input_text.txt", O_RDONLY);
            if (infile_fd < 0) {
                perror("Failed to open text file");
                exit(1);
            }
        }

Byte UnifiedMemory::Read(Word address) {
    
    if (DEBUG) {printf("Memory is read accessed at address %04X \n", address);}

    if (address <= 0x3FFF) {
        return ram.Read(address);
    // } else if (address >= 0x2000 && address <= 0x3FFF) {
        // I/O Registers
        // return ioRegisters[address % 8]; // Example: NES-like mirroring
    } else if (address >= 0x8000) {
        // ROM
        return rom.Read(address);
    } else if (address == 0x5000) {
        // External register for ben eater's 6502 computer
        return CHAR_IO.Read();
    } else if (address == 0x5001) {// 20481
        // External register for ben eater's 6502 computer
        if (DEBUG) {
        printf("some read is happening on address %d\n", address);};
        return 0xFF; // this indicates that there is a character to read
    } else if (address == 0x5002) {// 20482
        // External register for ben eater's 6502 computer
        if (DEBUG){
        printf("some read is happening on address %d\n", address);};
        return 0;
    } else if (address == 0x5003) {// 20483
        // External register for ben eater's 6502 computer
        if (DEBUG){
        printf("some read is happening on address %d\n", address);}
        return 0;



    // fix the input and output stream. a separate bash program is responsible for making the pipes and managing
    } else if (address == 0x6008) {
        // Since we are taking a char form the instream, the current char is no longer valid
        Byte c;
        c = INSTREAM.Read(); // This just gives the value that is in the register
        INSTATUS.Write(INSTATUS.Read() & 0b11110111); // set the bit to 0:: indicate no new char available
        // printf("Reading a char at 6008, char is %c , with hex value %02X \n", c, c);
        return c;
    } else if (address == 0x6009) {
        Byte c;
        ssize_t n = read(in_fd, &c, 1);  // try to read 1 byte
        if (DEBUG) {printf("Trying to read byte from in_stream, i got %c \n", c);}
        if (n == 1) {
            // Successfully read a byte, store it in INSTREAM and set status bit
            INSTREAM.Write(c);
            INSTATUS.Write(0x08); // set bit 3: new char available
            if (DEBUG) printf("Read new char from pipe: %c\n", c);
        } else if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            // No data available yet; normal for non-blocking
            if (DEBUG) printf("No new char available\n");
            INSTATUS.Write(0x10); // set bit 4: no char and no problem, try again later

        } else if (n == 0) {
            // Pipe closed (EOF)
            // printf("Pipe closed\n");
            INSTATUS.Write(0x20); // EOF. Pack it up
            // printf("Warning: Pipe closed \n");
        } else {
            // Real error
            perror("read failed");
            INSTATUS.Write(0x40); // Big error. Exit immediately
        }
        return INSTATUS.Read();
    } else if (address == 0x600A) {
        return OUTSTREAM.Read();
    } else if (address == 0x600B) {
        return OUTSTATUS.Read();
    } else if (address == 0x6004){
        Byte c;
        ssize_t n = read(infile_fd, &c, 1);
        if (n==1) {
            return c;
        }
        else {
            return 0;
        }
    } else {

        printf("Tried to read from address %d\n", address);
        throw std::out_of_range("Address not mapped");
    }
}

void UnifiedMemory::Write(Word address, Byte value) {
        if (DEBUG) {printf("Memory is write accessed at address %04X \n", address);}
        if (address <= 0x3FFF) {
            // RAM with mirroring
            ram.Write(address, value);
        // } else if (address >= 0x2000 && address <= 0x3FFF) {
            // I/O Registers
            // ioRegisters[address % 8] = value;
        } else if (address >= 0x8000) {
            // ROM is read-only
            throw std::runtime_error("Cannot write to ROM");
        } else if (address == 0x5000) {
            // External register for ben eater's 6502 computer
            CHAR_IO.Write(value);
        } else if (address == 0x5001) {// 20481
            // External register for ben eater's 6502 computer
            if (DEBUG) {
            printf("some write is happening on address %d\n", address);}
            return;
        } else if (address == 0x5002) {// 20482
            // External register for ben eater's 6502 computer
            if (DEBUG) {
            printf("some write is happening on address %d\n", address);}
            return;
        } else if (address == 0x5003) { // 20483
            // External register for ben eater's 6502 computer
            if (DEBUG) {
                printf("some write is happening on address %d\n", address);
            }
            return;
        } else if (address == 0x6008) {
            INSTREAM.Write(value);
            printf("There is a write to instream??");
        } else if (address == 0x6009) {
            INSTATUS.Write(value);
        } else if (address == 0x600A) {
            if (out_fd != -1) {
                write(out_fd, &value, 1); // write the Byte to the pipe
            }
            // now that we have written a Byte, we mark the status as newly written
            OUTSTATUS.Write(OUTSTATUS.Read() & 0xFE);
        } else if (address == 0x600B) {
            OUTSTATUS.Write(value);
        } else if (address == 0x6004) {
            throw std::runtime_error("Cannot write to this location");
        } else if (address == 0x6005) {  // our new output-only register
            std::cout << static_cast<char>(value);
            std::cout.flush(); // make sure it appears immediately
        } else {
            printf("Tried to write to address %d\n", address);
            throw std::out_of_range("Address not mapped");
        }
    }

bool UnifiedMemory::LoadFromFile(const std::string &filename) {
        // const std::string filename = "memory.bin";
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filename << std::endl;
            return false;
        }

        std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        if (fileSize > 0x8000) {
            std::cerr << "Error: File too large for ROM range" << std::endl;
            return false;
        }

        std::vector<Byte> data(fileSize);
        if (file.read(reinterpret_cast<char*>(data.data()), fileSize)) {
            rom.LoadData(data);  // Load the data into ROM
            std::cout << "Memory loaded successfully from " << filename << std::endl;
            return true;
        }

        return false;
    }

