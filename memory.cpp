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

    bool LoadFromFile(const std::string &filename) {
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


};
