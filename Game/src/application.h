#pragma once
#include <renderer/renderer.h>
#include <game.h>

class Application
{
public:
    void InitGame(int gameResolutionWidth, int gameResolutionHeight,
        GameState& gameState, Renderer& renderer);
    void UpdateGame(const float dt, GameState& gameState);

    Texture CreateErrorTexture();
    std::string ReadEntireFile(const std::string& filename);
    Texture LoadTexture(const std::string& filename);

    std::unordered_map<char, FontGlyph> LoadFontGlyphs(
        const std::string& path, Renderer& renderer);
};