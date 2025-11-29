#include "BinaryReader.h"
#include <cstring>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <string>

BinaryReader::BinaryReader(const std::vector<unsigned char>& data) : buffer(data), pos(0) {}

unsigned char BinaryReader::ReadByte() {
    if (pos >= buffer.size()) throw std::runtime_error("End of stream");
    return buffer[pos++];
}

unsigned char BinaryReader::PeekByte() {
    if (pos >= buffer.size()) throw std::runtime_error("End of stream");
    return buffer[pos];
}

short BinaryReader::ReadShort() {
    short v = 0;
    for (int i = 0; i < 2; ++i) {
        unsigned char b = ReadByte();
        v |= (static_cast<short>(b) << (i * 8));
    }
    return v;
}

int BinaryReader::ReadInt() {
    int v = 0;
    for (int i = 0; i < 4; ++i) {
        unsigned char b = ReadByte();
        v |= (static_cast<int>(b) << (i * 8));
    }
    return v;
}

long long BinaryReader::ReadLong() {
    long long v = 0;
    for (int i = 0; i < 8; ++i) {
        unsigned char b = ReadByte();
        v |= (static_cast<long long>(b) << (i * 8));
    }
    return v;
}

float BinaryReader::ReadFloat() {
    float v;
    unsigned int iVal = ReadInt();
    std::memcpy(&v, &iVal, 4);
    return v;
}

double BinaryReader::ReadDouble() {
    double v;
    long long lVal = ReadLong();
    std::memcpy(&v, &lVal, 8);
    return v;
}

bool BinaryReader::ReadBool() {
    return ReadByte() != 0;
}

unsigned int BinaryReader::ReadULEB128() {
    unsigned int result = 0;
    int shift = 0;
    while (true) {
        unsigned char b = ReadByte();
        result |= (static_cast<unsigned int>(b & 0x7f) << shift);
        if ((b & 0x80) == 0) break;
        shift += 7;
        if (shift >= 32) throw std::runtime_error("ULEB128 too large");
    }
    // std::cout << "ULEB128: " << result << std::endl;
    return result;
}

std::string BinaryReader::ReadString() {
    // std::cerr << "DEBUG: Start ReadString at " << pos << std::endl;
    unsigned char head = PeekByte();
    
    if (head == 0x00) {
        ReadByte(); // Consume 00
        return "";
    }
    
    if (head == 0x0B) {
        ReadByte(); // Consume 0B
        // std::cerr << "DEBUG: Header OK at " << pos << std::endl;
        unsigned int length = ReadULEB128();
        // std::cerr << "String Length: " << length << " at pos " << pos << std::endl;
        std::string s(length, '\0');
        for (unsigned int i = 0; i < length; ++i) {
            s[i] = ReadByte();
        }
        // std::cerr << "DEBUG: End ReadString at " << pos << std::endl;
        return s;
    }
    
    // Fallback: Aggressive raw string reading
    // Read until we hit a control character (< 0x20) or 0x0B
    std::string raw;
    while (pos < buffer.size()) {
        unsigned char c = PeekByte();
        if (c < 0x20 && c != 0x0B) { // Stop at control chars (except 0x0B which is standard header)
             break;
        }
        if (c == 0x0B) {
            break;
        }
        
        raw += (char)ReadByte();
    }
    
    if (!raw.empty()) {
        // std::cerr << "WARNING: Read raw string '" << raw << "' at " << (pos - raw.length()) << std::endl;
        // std::cerr << "DEBUG: ReadString returning raw string" << std::endl;
        return raw;
    }

    std::string err = "Invalid string header: " + std::to_string((int)head) + " at pos " + std::to_string(pos);
    throw std::runtime_error(err);
}

void BinaryReader::Skip(int bytes) {
    if (pos + bytes > buffer.size()) throw std::runtime_error("Skip out of bounds");
    pos += bytes;
}

void BinaryReader::Seek(size_t offset) {
    if (offset > buffer.size()) throw std::runtime_error("Seek out of bounds");
    pos = offset;
}

size_t BinaryReader::Tell() {
    return pos;
}

void BinaryReader::Dump(size_t offset, size_t count) {
    size_t start = offset;
    size_t end = (start + count < buffer.size()) ? start + count : buffer.size();
    
    std::cout << "Dump from " << start << " to " << end << ":" << std::endl;
    for (size_t i = start; i < end; ++i) {
        unsigned char b = buffer[i];
        std::cout << std::hex << (int)b << " ";
        if ((b >= 32 && b <= 126)) std::cout << "(" << (char)b << ") ";
        else std::cout << "(.) ";
        if ((i - start + 1) % 8 == 0) std::cout << std::endl;
    }
    std::cout << std::dec << std::endl;
}
