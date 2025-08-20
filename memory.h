#ifndef MEMORY_H
#define MEMORY_H

#include <vector>
#include <iostream>
#include <fstream>

using u32 = unsigned int;
using Byte = unsigned char;
using Word = unsigned short;

int in_fd = -1;
int out_fd = -1;
int infile_fd = -1;

class Mem {
protected:
    std::vector<Byte> memory;
    Word start_address, end_address;

public:
    Mem(Word start, Word end);
    virtual ~Mem() = default;

    virtual Byte Read(Word address);
    virtual void Write(Word address, Byte value);
    bool LoadFromFile(const std::string& filename);
    Word GetSize() const { return end_address - start_address + 1; }
};

class RAM : public Mem {
public:
    RAM(Word start, Word end);
};

class ROM : public Mem {
public:
    ROM(Word start, Word end);
    void Write(Word address, Byte value) override;
    void LoadData(const std::vector<Byte>& data);
};

class ExternalRegister {
    Byte value;  // It has only one address, so it can only store a single byte
    Word address;

public:
    ExternalRegister(Word addr);

    Byte Read();
    void Write(Byte newValue);
    
    Word GetAddress() const { return address; }
};


class UnifiedMemory {
    RAM ram;
    ROM rom;
    ExternalRegister CHAR_IO;
    ExternalRegister INSTREAM;
    ExternalRegister INSTATUS;
    ExternalRegister OUTSTREAM;
    ExternalRegister OUTSTATUS;
    ExternalRegister FILEIN; // take a file as input
    ExternalRegister STDOUT; // write only output
public:
    UnifiedMemory();
    Byte Read(Word address);
    void Write(Word address, Byte value);
    bool LoadFromFile(const std::string &filename);
};

#endif // MEMORY_H
