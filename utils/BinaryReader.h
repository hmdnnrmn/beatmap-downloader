#pragma once
#include <fstream>
#include <string>
#include <vector>

class BinaryReader {
public:
    BinaryReader(const std::vector<unsigned char>& data);

    unsigned char ReadByte();
    unsigned char PeekByte();
    short ReadShort();
    int ReadInt();
    long long ReadLong();
    float ReadFloat();
    double ReadDouble();
    bool ReadBool();
    unsigned int ReadULEB128();
    std::string ReadString();
    void Skip(int bytes);
    void Seek(size_t offset);
    size_t Tell();
    void Dump(size_t offset, size_t count);

private:
    const std::vector<unsigned char>& buffer;
    size_t pos;
};
