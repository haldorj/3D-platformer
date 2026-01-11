#include <pch.h>
#include "sound.h"

static uint32_t ReadUInt32LE(const std::string& buf, size_t offset) 
{
    return  (uint8_t)buf[offset]             |
            ((uint8_t)buf[offset + 1] << 8)  |
            ((uint8_t)buf[offset + 2] << 16) |
            ((uint8_t)buf[offset + 3] << 24);
}

static uint16_t ReadUInt16LE(const std::string& buf, size_t offset) 
{
    return  (uint8_t)buf[offset]             |
            ((uint8_t)buf[offset + 1] << 8);
}

static std::vector<float> ConvertToFloat(const std::vector<char>& data, int bitsPerSample)
{
    std::vector<float> result;

    if (bitsPerSample == 16)
    {
        size_t sampleCount = data.size() / 2;
        result.resize(sampleCount);

        for (size_t i = 0; i < sampleCount; ++i)
        {
            int16_t sampleInt;
            std::memcpy(&sampleInt, &data[i * 2], sizeof(sampleInt)); // little-endian safe
            result[i] = static_cast<float>(sampleInt) / 32768.0f;     // normalize to [-1, 1]
        }
    }
    else if (bitsPerSample == 8)
    {
        size_t sampleCount = data.size();
        result.resize(sampleCount);

        for (size_t i = 0; i < sampleCount; ++i)
        {
            uint8_t sampleInt = static_cast<uint8_t>(data[i]);
            result[i] = (static_cast<float>(sampleInt) - 128.0f) / 128.0f;
        }
    }
    else if (bitsPerSample == 24)
    {
        size_t sampleCount = data.size() / 3;
        result.resize(sampleCount);
        const unsigned char* bytes = reinterpret_cast<const unsigned char*>(data.data());

        for (size_t i = 0; i < sampleCount; ++i)
        {
            // Assemble 3 bytes into a 32-bit signed int (little-endian)
            int32_t value = (bytes[i * 3]) |
                (bytes[i * 3 + 1] << 8) |
                (bytes[i * 3 + 2] << 16);

            // Sign-extend from 24 bits to 32 bits
            if (value & 0x00800000)
                value |= 0xFF000000;

            // Normalize to [-1, 1]
            result[i] = static_cast<float>(value) / 8388608.0f; // 2^23
        }
    }
    else if (bitsPerSample == 32)
    {
        size_t sampleCount = data.size() / 4;
        result.resize(sampleCount);

        for (size_t i = 0; i < sampleCount; ++i)
        {
            int32_t sampleInt;
            std::memcpy(&sampleInt, &data[i * 4], sizeof(sampleInt));
            result[i] = static_cast<float>(sampleInt) / 2147483648.0f;
        }
    }
    else
    {
        // Unsupported format
    }

    return result;
}


Sound LoadWavFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
    {
        std::cerr << "Failed to open file: " << path << "\n";
        return {};
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string buffer(size, '\0');
    if (!file.read(buffer.data(), size))
    {
        std::cerr << "Failed to read file: " << path << "\n";
        return {};
    }

    Sound sound{};
    WavHeader header = {};

    // --- Validate RIFF header ---
    if (buffer.substr(0, 4) != "RIFF" || buffer.substr(8, 4) != "WAVE") {
        std::cerr << "Not a valid WAV file.\n";
        return {};
    }

    header.ChunkSize = ReadUInt32LE(buffer, 4);

    // --- "fmt " chunk ---
    size_t fmtIndex = buffer.find("fmt ");
    if (fmtIndex == std::string::npos) {
        std::cerr << "No fmt chunk found.\n";
        return {};
    }

    header.SubChunk1Size =  ReadUInt32LE(buffer, fmtIndex + 4);
    header.AudioFormat =    ReadUInt16LE(buffer, fmtIndex + 8);
    header.NumChannels =    ReadUInt16LE(buffer, fmtIndex + 10);
    header.SampleRate =     ReadUInt32LE(buffer, fmtIndex + 12);
    header.ByteRate =       ReadUInt32LE(buffer, fmtIndex + 16);
    header.BlockAlign =     ReadUInt16LE(buffer, fmtIndex + 20);
    header.BitsPerSample =  ReadUInt16LE(buffer, fmtIndex + 22);

    // --- "data" chunk ---
    size_t dataIndex = buffer.find("data");
    if (dataIndex == std::string::npos) {
        std::cerr << "No data chunk found.\n";
        return {};
    }

    header.SubChunk2Size = ReadUInt32LE(buffer, dataIndex + 4);

    // --- Audio data ---
    size_t dataStart = dataIndex + 8;
    if (dataStart + header.SubChunk2Size > buffer.size()) {
        std::cerr << "Data chunk size exceeds file size.\n";
        return {};
    }

    std::vector<char> data;
    data.assign(buffer.begin() + dataStart, buffer.begin() + dataStart + header.SubChunk2Size);

    sound.SampleRate = header.SampleRate;
    sound.NumChannels = header.NumChannels;
    sound.BitsPerSample = header.BitsPerSample;
    sound.AudioBuffer = ConvertToFloat(data, header.BitsPerSample);

    std::cout << "Loaded WAV file: " << path << "\n";
    std::cout << "Channels: " << header.NumChannels << "\n";
    std::cout << "Sample Rate: " << header.SampleRate << "\n";
    std::cout << "Bits Per Sample: " << header.BitsPerSample << "\n";
    std::cout << "Data Size: " << header.SubChunk2Size << " bytes\n";

    return sound;
}
