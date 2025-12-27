#pragma once

#include "game.h"
#include "platform/platform.h"
#include "renderer/renderer.h"

class Application
{
public:
    void Init();
    void Run();

    void InitGame(int gameResolutionWidth, int gameResolutionHeight);
    void UpdateGame(const float dt);

    std::string ReadEntireFile(const std::string& path);

    Texture CreateErrorTexture();
    Texture LoadTexture(const std::string& path);

    std::unordered_map<char, FontGlyph> LoadFontGlyphs(
        const std::string& path);

public:
    GameState _GameState;

    std::unique_ptr<Platform> _Platform;
    std::unique_ptr<Renderer> _Renderer;
};