#include "database.h"
#include "utils/BinaryReader.h"
#include <iostream>
#include <fstream>

OsuDatabase::OsuDatabase() : dbInfo{0} {}

OsuDatabase::~OsuDatabase() {}

bool OsuDatabase::Load(const std::string& path) {
    std::cout << "Attempting to load osu!.db..." << std::endl;
    std::ifstream fs(path, std::ios::binary | std::ios::ate);
    if (!fs.is_open()) {
        std::cerr << "Failed to open " << path << std::endl;
        return false;
    }

    std::streamsize size = fs.tellg();
    fs.seekg(0, std::ios::beg);

    std::vector<unsigned char> buffer(size);
    if (!fs.read(reinterpret_cast<char*>(buffer.data()), size)) {
        std::cerr << "Failed to read file content" << std::endl;
        return false;
    }

    BinaryReader reader(buffer);
    // std::cout << "Buffer size: " << buffer.size() << std::endl;

    try {
        // 1. Skip 17-byte header
        // Version (4) + FolderCount (4) + AccountUnlocked (1) + UnlockDate (8)
        reader.Skip(17);

        // 2. Skip username string
        reader.ReadString();

        // 3. Read beatmap count (N)
        int beatmapCount = reader.ReadInt();
        // std::cout << "Beatmaps to parse: " << beatmapCount << std::endl;

        for (int i = 0; i < beatmapCount; ++i) {
            // EOF Check
            if (reader.Tell() >= buffer.size() - 100) { // Buffer safety margin
                // std::cout << "Near end of buffer at " << reader.Tell() << ". Stopping." << std::endl;
                break;
            }

            // 4. Skip 9 strings
            // Artist, ArtistUnicode, Title, TitleUnicode, Creator, Difficulty, Audio, MD5, OsuFile
            
            // Skip padding (0x00) before strings
            while (reader.PeekByte() == 0x00) {
                reader.ReadByte();
                if (reader.Tell() >= buffer.size()) break;
            }

            for (int j = 0; j < 9; ++j) {
                // Garbage skip logic (optional, but good for robustness)
                unsigned char b = reader.PeekByte();
                if (b != 0x0B && b != 0x00) {
                    while (reader.PeekByte() != 0x0B && reader.PeekByte() != 0x00 && reader.Tell() < buffer.size()) {
                        reader.ReadByte();
                    }
                }
                reader.ReadString();
            }

            // 5. Skip 39 bytes (Standard format: 1+2+2+2+8+4+4+4+4+8 = 39)
            reader.Skip(39);

            // 6. Skip 4 star rating arrays
            for (int j = 0; j < 4; ++j) {
                int count = reader.ReadInt();
                // Each pair is: 0x08 (1) + Int (4) + 0x0C (1) + Val (4 or 8)
                // Using 10 bytes (Float)
                reader.Skip(count * 10); 
            }

            // 7. Skip 12 bytes
            reader.Skip(12);

            // 8. Skip timing points array
            int timingPointCount = reader.ReadInt();
            reader.Skip(timingPointCount * 17);

            // 9. READ beatmap ID (bid)
            int beatmapId = reader.ReadInt();

            // 10. READ set ID (sid)
            int setId = reader.ReadInt();

            // Store IDs
            if (beatmapId > 0) existingBeatmapIds.insert(beatmapId);
            if (setId > 0) existingSetIds.insert(setId); 

            // if (i % 1000 == 0) std::cout << "Parsed " << i << "/" << beatmapCount << " (BID: " << beatmapId << ", SID: " << setId << ")" << std::endl;

            // 11. Skip 15 bytes
            reader.Skip(15);

            // 12. Skip 2 strings (Source, Tags)
            for (int j = 0; j < 2; ++j) {
                 reader.ReadString();
            }

            // 13. Skip 2 bytes (OnlineOffset)
            reader.Skip(2);

            // 14. Skip 1 string (Font)
            reader.ReadString();

            // 15. Skip 10 bytes (Unplayed, LastPlayed, IsOsz2)
            reader.Skip(10);

            // 16. Read Folder string
            reader.ReadString();

            // 17. Skip 15 bytes (Reduced from 22)
            reader.Skip(15);
            
            // 18. Handle Gap/Padding between beatmaps
            // Robust Approach: Chain Validation
            // We scan for the start of the next beatmap by looking for 4 consecutive valid strings:
            // Artist, ArtistUnicode, Title, TitleUnicode.
            // This pattern is extremely unlikely to appear in random garbage data.
            
            if (i < beatmapCount - 1) { 
                size_t scanStart = reader.Tell();
                bool found = false;
                size_t maxScan = 5000; // Safety limit

                for (size_t k = 0; k < maxScan; ++k) {
                    if (reader.Tell() + k >= buffer.size()) break;

                    // Check for 0x0B (String Header)
                    if (buffer[reader.Tell() + k] == 0x0B) {
                        try {
                            size_t currentPos = reader.Tell();
                            reader.Seek(currentPos + k); // Jump to potential Artist start

                            // Validate Chain of 4 Strings
                            bool chainValid = true;
                            for (int s = 0; s < 4; ++s) {
                                unsigned char header = reader.PeekByte();
                                if (header != 0x0B && header != 0x00) {
                                    chainValid = false;
                                    break;
                                }
                                reader.ReadString(); // Consume string
                            }

                            if (chainValid) {
                                // Found 4 valid strings in a row!
                                // Reset to the start of the chain (Artist)
                                reader.Seek(currentPos + k);
                                found = true;
                                break;
                            }

                            // Chain failed, restore and continue scanning
                            reader.Seek(currentPos);

                        } catch (...) {
                            reader.Seek(scanStart);
                        }
                    }
                }

                if (!found) {
                    // If scan failed, restore position (let the loop crash or handle it)
                    reader.Seek(scanStart);
                }
            }
        }
        
        std::cout << "Successfully loaded osu!.db" << std::endl;
        // std::cout << "Finished parsing. Total Beatmaps: " << existingBeatmapIds.size() << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error parsing DB: " << e.what() << std::endl;
        return false;
    }
    return true;
}
