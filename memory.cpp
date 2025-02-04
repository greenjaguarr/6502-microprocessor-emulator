#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <vector>

#include "memory.h"
// http://www.obelisk.me.uk/6502/
// https://www.youtube.com/watch?v=qJgsuQoy9bc
// http://www.6502.org/tutorials/6502opcodes.html

using u32 = unsigned int;
using Byte = unsigned char;
using Word = unsigned short;

#define DEBUG false


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
    char input;
    std::cin >> input;  // Read a single character
    value = static_cast<Byte>(input);  // Store it as a Byte (unsigned char) // idk about this static cast thing
    std::cout << "\n Just read the character " << value << std::endl;
    return value;
}
void ExternalRegister::Write(Byte newValue) {
    // value = newValue;
    std::cout << newValue;
}   

UnifiedMemory::UnifiedMemory()
        : ram(0x0000, 0x3FFF), rom(0x8000, 0xFFFF), CHAR_IO(0x5000) {}

Byte UnifiedMemory::Read(Word address) {
        if (address <= 0x3FFF) {
            return ram.Read(address);
        // } else if (address >= 0x2000 && address <= 0x3FFF) {
            // I/O Registers
            // return ioRegisters[address % 8]; // Example: NES-like mirroring
        } else if (address >= 0x8000) {
            // ROM
            return rom.Read(address);
        } else if (address == 0x5000) {
            // External register
            return CHAR_IO.Read();
        } else if (address == 0x5001) {// 20481
            // External register
            printf("some bs is happening on address %d\n", address);
            return 0;
        } else if (address == 0x5002) {// 20482
            // External register
            printf("some bs is happening on address %d\n", address);
            return 0;
        } else if (address == 0x5003) {// 20483
            // External register
            printf("some bs is happening on address %d\n", address);
            return 0;
        } else {
            printf("Tried to read from address %d\n", address);
            throw std::out_of_range("Address not mapped");
        }
    }

void UnifiedMemory::Write(Word address, Byte value) {
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
            // External register
            CHAR_IO.Write(value);
        } else if (address == 0x5001) {// 20481
            // External register
            printf("some bs is happening on address %d\n", address);
            return;
        } else if (address == 0x5002) {// 20482
            // External register
            printf("some bs is happening on address %d\n", address);
            return;
        } else if (address == 0x5003) { // 20483
            // External register
            printf("some bs is happening on address %d\n", address);
            return;
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

