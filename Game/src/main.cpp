#include "pch.h"
#include <game.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#ifdef _WIN32
#include "platform/win32_platform.h"
#include "renderer/d3d11_renderer.h"
#endif

void Init();
void Run();
void HandleInput();
void InitGame(int gameResolutionWidth, int gameResolutionHeight);
void UpdateGame(const float dt);

[[nodiscard]] std::string ReadEntireFile(const std::string& path);
[[nodiscard]] Texture CreateErrorTexture();
[[nodiscard]] Texture LoadTexture(const std::string& path);
[[nodiscard]] std::unordered_map<char, FontGlyph> LoadFontGlyphs(const std::string& path);

static GameState _GameState;

static std::unique_ptr<Platform> _Platform;
static std::unique_ptr<Renderer> _Renderer;

static uint32_t WindowWidth = 1280;
static uint32_t WindowHeight = 720;

static uint32_t GameResolutionWidth = 1280;
static uint32_t GameResolutionHeight = 720;

static bool VSync{ true };
static int FPS = 0;
static bool Running;

void Init()
{
    GameResolutionWidth = min(WindowWidth, GameResolutionWidth);
	GameResolutionHeight = min(WindowHeight, GameResolutionHeight);

#ifdef _WIN32
    _Platform = std::make_unique<Win32Platform>();
    _Renderer = std::make_unique<D3D11Renderer>();
#endif
    assert(_Platform && _Renderer);

    _Platform->PlatformInitWindow(WindowWidth, WindowHeight, L"Window");
	_Platform->PlatformInitInput();

    _Renderer->InitRenderer(
        GameResolutionHeight, GameResolutionWidth, *_Platform, _GameState);

    InitGame(GameResolutionWidth, GameResolutionHeight);
}

void Run()
{
    Running = true;
    auto previousTime = std::chrono::steady_clock::now();

    while (Running)
    {
        static double fpsTimer{};
        static int fpsFrameCount{};
        static float deltaTime{};

        fpsTimer += deltaTime;
        fpsFrameCount++;

        if (fpsTimer >= 1.0)
        {
            FPS = static_cast<int>(fpsFrameCount / fpsTimer);
            fpsFrameCount = 0;
            fpsTimer = 0.0;
        }

        V3 textColor = { 1.0f, 1.0f, 1.0f };
        float textScale = 0.75f;
        const std::string fpsStr = std::move(std::format("FPS: {}", FPS));

        _Platform->PlatformUpdateWindow(Running);
        HandleInput();
		_Platform->PlatformUpdateInput();

		V2 mousePos = _Platform->GetMousePosition();

        UpdateGame(deltaTime);

        _Renderer->RenderScene(_GameState);
        _Renderer->RenderText(_GameState,
            GameResolutionWidth, GameResolutionHeight,
            fpsStr, 0, 0, textScale, textColor);
        _Renderer->RenderText(_GameState,
            GameResolutionWidth, GameResolutionHeight,
            "www123", 0, 21, 0.4f, textColor);
        _Renderer->PresentSwapChain(VSync);

        auto currentTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = currentTime - previousTime;
        previousTime = currentTime;
        deltaTime = static_cast<float>(elapsed.count());
    }
}

void InitGame(int gameResolutionWidth, int gameResolutionHeight)
{
    //Camera information
    _GameState.CamPosition = { 0.0f, 3.0f, -8.0f };
    _GameState.CamTarget = { 0.0f, 0.0f, 0.0f };
    _GameState.CamUp = { 0.0f, 1.0f, 0.0f };

    //Set the View matrix
    _GameState.CamView = MatrixLookAt(
        _GameState.CamPosition, _GameState.CamTarget, _GameState.CamUp);

    //Set the Projection matrix
    _GameState.CamProjection = MatrixPerspective(
        0.4f * 3.14f, static_cast<float>(gameResolutionWidth) / gameResolutionHeight, 1.0f, 1000.0f);
    auto fontGlyphs = LoadFontGlyphs("C:/Windows/Fonts/calibri.ttf");
    _GameState.LoadedFontGlyphs = std::move(fontGlyphs);

    _GameState.GlobalDirectionalLight.Direction = { .X = -0.25f, .Y = -0.5f, .Z = -1.0f };
    _GameState.GlobalDirectionalLight.Ambient = { .X = 0.15f, .Y = 0.15f, .Z = 0.15f };
    _GameState.GlobalDirectionalLight.Diffuse = { .X = 0.8f, .Y = 0.8f, .Z = 0.8f };
}

void HandleInput()
{
    
    if (_Platform->IsKeyDown(KeyCode::KEY_ESCAPE))
    {
		Running = false;
    }
    if (_Platform->IsKeyDown(KeyCode::KEY_W))
    {
        _GameState.CamPosition.Z += 0.1f;
		_GameState.CamTarget.Z += 0.1f;
    }
    if (_Platform->IsKeyDown(KeyCode::KEY_S))
    {
        _GameState.CamPosition.Z -= 0.1f;
		_GameState.CamTarget.Z -= 0.1f;
	}
    if (_Platform->IsKeyDown(KeyCode::KEY_A))
    {
        _GameState.CamPosition.X -= 0.1f;
		_GameState.CamTarget.X -= 0.1f;
	}
    if (_Platform->IsKeyDown(KeyCode::KEY_D))
    {
        _GameState.CamPosition.X += 0.1f;
		_GameState.CamTarget.X += 0.1f;
    }
    if (_Platform->IsKeyDown(KeyCode::KEY_SPACE))
    {
        _GameState.CamPosition.Y += 0.1f;
		_GameState.CamTarget.Y += 0.1f;
    }
    if (_Platform->IsKeyDown(KeyCode::KEY_LEFT_CTRL))
    {
        _GameState.CamPosition.Y -= 0.1f;
		_GameState.CamTarget.Y -= 0.1f;
    }
}

void UpdateGame(const float dt)
{
    //Keep the cubes rotating
    _GameState.Rot += .5f * dt;
    if (_GameState.Rot > 6.28f)
        _GameState.Rot = 0.0f;

    _GameState.GlobalDirectionalLight.Ambient = { 0.4f, 0.4f, 0.4f };
    _GameState.GlobalDirectionalLight.Color = { 1.0f, 1.0f, 1.0f };
    V3 lightDir = Normalize({ 0.5f, 1.0f, 0.5f });
    _GameState.GlobalDirectionalLight.Direction = { lightDir.X, lightDir.Y, lightDir.Z, 0.0f };

    //Reset cube1World
    _GameState.Cube1World = MatrixIdentity();

    _GameState.Rotation = MatrixRotationY(_GameState.Rot);
    _GameState.Translation = MatrixTranslation(0.0f, 0.0f, 4.0f);

    //Set cube1's world space using the transformations
    _GameState.Cube1World = _GameState.Translation * _GameState.Rotation;

    //Reset cube2World
    _GameState.Cube2World = MatrixIdentity();

    //Define cube2's world space matrix
    _GameState.Rotation = MatrixRotationY(_GameState.Rot);
    _GameState.Scale = MatrixScaling(1.3f, 1.3f, 1.3f);

    //Set cube2's world space matrix
    _GameState.Cube2World = _GameState.Rotation * _GameState.Scale;

    _GameState.CamView = MatrixLookAt(
        _GameState.CamPosition, _GameState.CamTarget, _GameState.CamUp);
}

std::unordered_map<char, FontGlyph> LoadFontGlyphs(const std::string& path)
{
    std::unordered_map<char, FontGlyph> result;

    assert(_Renderer);

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
        glyph.TextureView = _Renderer.get()->CreateTextureView(fontTexture);
        glyph.Size = { .X = static_cast<float>(width), .Y = static_cast<float>(-height) };
        glyph.Bearing = { .X = static_cast<float>(xOffset), .Y = static_cast<float>(yOffset + pixelHeight) };
        glyph.Advance = scale * static_cast<float>(advance);

        result.emplace(codepoint, glyph);

        stbtt_FreeBitmap(bitmap, nullptr);
    }

    return result;
}

std::string ReadEntireFile(const std::string& path)
{
    std::ifstream file(path.c_str(), std::ios::binary | std::ios::ate);
    if (!file)
    {
        std::print("Failed to open file: {}", path);
        return {};
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string buffer(size, '\0');
    if (!file.read(buffer.data(), size))
    {
        std::print("Failed to read file: {}", path);
        return {};
    }
    return buffer;
}

int main()
{
    Init();
	Run();
}
