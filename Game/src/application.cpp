#include "pch.h"
#include "application.h"
#include <game.cpp>

std::unordered_map<char, FontGlyph> Application::LoadFontGlyphs(
    const std::string& path, Renderer& renderer)
{
    std::unordered_map<char, FontGlyph> result;

    std::string ttfBuffer = ReadEntireFile(path);
    assert(ttfBuffer.size()); // Ensure the font file was read successfully

    const unsigned char* data = (unsigned char*)ttfBuffer.data();

    stbtt_fontinfo font{};
    if (!stbtt_InitFont(&font, data, stbtt_GetFontOffsetForIndex(data, 0)))
    {
        std::println("Failed to initialize font from file: {}", path);
        return {};
    }

    constexpr float pixelHeight = 32;
    const float scale = stbtt_ScaleForPixelHeight(&font, pixelHeight);
    int width, height, xOffset, yOffset, advance, lsb;

    for (int codepoint = 32; codepoint < 128; ++codepoint)
    {
        const auto bitmap = stbtt_GetCodepointBitmap(
            &font, 0, scale, codepoint, &width, &height, &xOffset, &yOffset);
        stbtt_GetCodepointHMetrics(&font, codepoint, &advance, &lsb);

        if (width == 0 || height == 0)
        {
            stbtt_FreeBitmap(bitmap, nullptr);
            continue;
        }

        Texture fontTexture{};
        fontTexture.Width = width;
        fontTexture.Height = height;
        fontTexture.Pixels.assign(static_cast<size_t>(width * height * 4), 0);

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                const unsigned char value = bitmap[y * width + x];
                const int dst = (y * width + x) * 4;
                fontTexture.Pixels[dst + 0] = 255;
                fontTexture.Pixels[dst + 1] = 255;
                fontTexture.Pixels[dst + 2] = 255;
                fontTexture.Pixels[dst + 3] = value;
            }
        }

        FontGlyph glyph{};
        glyph.TextureView = renderer.CreateTextureView(fontTexture);
        glyph.Size = { .X = static_cast<float>(width), .Y = static_cast<float>(-height) };
        glyph.Bearing = { .X = static_cast<float>(xOffset), .Y = static_cast<float>(yOffset + pixelHeight) };
        glyph.Advance = scale * static_cast<float>(advance);

        result.emplace(codepoint, glyph);

        stbtt_FreeBitmap(bitmap, nullptr);
    }

    return result;
}

void Application::InitGame(int gameResolutionWidth, int gameResolutionHeight,
    GameState& gameState, Renderer& renderer)
{
    //Camera information
    gameState.CamPosition = { 0.0f, 3.0f, -8.0f };
    gameState.CamTarget = { 0.0f, 0.0f, 0.0f };
    gameState.CamUp = { 0.0f, 1.0f, 0.0f };

    //Set the View matrix
    gameState.CamView = MatrixLookAt(gameState.CamPosition, gameState.CamTarget, gameState.CamUp);

    //Set the Projection matrix
    gameState.CamProjection = MatrixPerspective(
        0.4f * 3.14f, static_cast<float>(gameResolutionWidth) / gameResolutionHeight, 1.0f, 1000.0f);
    auto fontGlyphs = LoadFontGlyphs("C:/Windows/Fonts/calibri.ttf", renderer);
    gameState.LoadedFontGlyphs = std::move(fontGlyphs);

    gameState.GlobalDirectionalLight.Direction = { .X = -0.25f, .Y = -0.5f, .Z = -1.0f };
    gameState.GlobalDirectionalLight.Ambient = { .X = 0.15f, .Y = 0.15f, .Z = 0.15f };
    gameState.GlobalDirectionalLight.Diffuse = { .X = 0.8f, .Y = 0.8f, .Z = 0.8f };
}

void Application::UpdateGame(const float dt, GameState& gameState)
{
    //Keep the cubes rotating
    gameState.Rot += .5f * dt;
    if (gameState.Rot > 6.28f)
        gameState.Rot = 0.0f;

    gameState.GlobalDirectionalLight.Ambient = { 0.4f, 0.4f, 0.4f };
    gameState.GlobalDirectionalLight.Color = { 1.0f, 1.0f, 1.0f };
    V3 lightDir = Normalize({ 0.5f, 1.0f, 0.5f });
    gameState.GlobalDirectionalLight.Direction = { lightDir.X, lightDir.Y, lightDir.Z, 0.0f };

    //Reset cube1World
    gameState.Cube1World = MatrixIdentity();

    gameState.Rotation = MatrixRotationY(gameState.Rot);
    gameState.Translation = MatrixTranslation(0.0f, 0.0f, 4.0f);

    //Set cube1's world space using the transformations
    gameState.Cube1World = gameState.Translation * gameState.Rotation;

    //Reset cube2World
    gameState.Cube2World = MatrixIdentity();

    //Define cube2's world space matrix
    gameState.Rotation = MatrixRotationY(gameState.Rot);
    gameState.Scale = MatrixScaling(1.3f, 1.3f, 1.3f);

    //Set cube2's world space matrix
    gameState.Cube2World = gameState.Rotation * gameState.Scale;
}

Texture Application::CreateErrorTexture()
{
    Texture texture{};

    constexpr size_t width = 256;
    constexpr size_t height = 256;
    auto pixels = static_cast<unsigned char*>(malloc(width * height * 4));

    if (!pixels)
    {
        return texture;
    }

    // Checkerboard pattern
    for (auto y = 0; y < height; ++y)
    {
        for (auto x = 0; x < width; ++x)
        {
            const int index = (y * width + x) * 4;
            constexpr int checkSize = 16;

            if (x / checkSize % 2 == y / checkSize % 2)
            {
                pixels[index + 0] = 255; // R
                pixels[index + 1] = 0;   // G
                pixels[index + 2] = 255; // B
                pixels[index + 3] = 255; // A
            }
            else
            {
                pixels[index + 0] = 0;   // R
                pixels[index + 1] = 0;   // G
                pixels[index + 2] = 0;   // B
                pixels[index + 3] = 255; // A
            }
        }
    }

    texture.Width = width;
    texture.Height = height;
    texture.Pixels = std::vector<unsigned char>(pixels, pixels + (width * height * 4));

    free(pixels);

    return texture;
}

std::string Application::ReadEntireFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file)
    {
        std::print("Failed to open file: {}", filename);
        return {};
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string buffer(size, '\0');
    if (!file.read(buffer.data(), size))
    {
        std::print("Failed to read file: {}", filename);
        return {};
    }
    return buffer;
}

Texture Application::LoadTexture(const std::string& filename)
{
    int width, height, channels;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!data)
    {
        return CreateErrorTexture();
    }

    Texture texture;
    texture.Width = width;
    texture.Height = height;
    texture.Pixels = std::vector<unsigned char>(data, data + (width * height * 4));
    return texture;
}

