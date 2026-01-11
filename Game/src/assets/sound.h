#pragma once

struct Sound
{
	std::vector<float> AudioBuffer{};
	uint32_t SampleRate{};
	uint32_t NumChannels{};
    uint16_t BitsPerSample{};
};

struct WavHeader 
{
    char ChunkID[4];
    uint32_t ChunkSize;
    char Format[4];

    char SubChunk1ID[4];
    uint32_t SubChunk1Size;
    uint16_t AudioFormat;
    uint16_t NumChannels;
    uint32_t SampleRate;
    uint32_t ByteRate;
    uint16_t BlockAlign;
    uint16_t BitsPerSample;

    char SubChunk2ID[4];
    uint32_t SubChunk2Size;
};

Sound LoadWavFile(const std::string& path);