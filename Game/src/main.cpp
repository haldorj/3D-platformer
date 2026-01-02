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
void Move(float dt);
void InitGame(int gameResolutionWidth, int gameResolutionHeight);
void UpdateCamera(const float dt);
void UpdateGame(const float dt);

[[nodiscard]] std::string ReadEntireFile(const std::string& path);
[[nodiscard]] Texture CreateErrorTexture();
[[nodiscard]] Texture LoadTexture(const std::string& path);
[[nodiscard]] std::unordered_map<char, FontGlyph> LoadFontGlyphs(const std::string& path);

static GameState _GameState;

constinit std::unique_ptr<Platform> _Platform;
constinit std::unique_ptr<Renderer> _Renderer;

constinit uint32_t _WindowWidth = 1280;
constinit uint32_t _WindowHeight = 720;

constinit uint32_t _GameResolutionWidth = 1280;
constinit uint32_t _GameResolutionHeight = 720;

constinit int _FPS{};
constinit bool _VSync{ true };
constinit bool _Running{};
constinit bool _EditMode{};
constinit bool _ShowCursor{ true };

static float pitch = 0.0f;
static float yaw = 180.0f;

void Init()
{
    _GameResolutionWidth = min(_WindowWidth, _GameResolutionWidth);
	_GameResolutionHeight = min(_WindowHeight, _GameResolutionHeight);

#ifdef _WIN32
    _Platform = std::make_unique<Win32Platform>();
    _Renderer = std::make_unique<D3D11Renderer>();
#endif
    assert(_Platform && _Renderer);

    _Platform->PlatformInitWindow(_WindowWidth, _WindowHeight, L"Window");
	_Platform->PlatformInitInput();

    _Renderer->InitRenderer(
        _GameResolutionHeight, _GameResolutionWidth, *_Platform, _GameState);

    InitGame(_GameResolutionWidth, _GameResolutionHeight);
}

void Run()
{
    _Running = true;
    auto previousTime = std::chrono::steady_clock::now();

    while (_Running)
    {
        static double fpsTimer{};
        static int fpsFrameCount{};
        static float deltaTime{};

        fpsTimer += deltaTime;
        fpsFrameCount++;

        if (fpsTimer >= 1.0)
        {
            _FPS = static_cast<int>(fpsFrameCount / fpsTimer);
            fpsFrameCount = 0;
            fpsTimer = 0.0;
        }

        V3 textColor = { 1.0f, 1.0f, 1.0f };
        float textScale = 0.75f;
        const std::string fpsStr = std::move(std::format("FPS: {}", _FPS));

        _Platform->PlatformUpdateWindow(_Running);
		_Platform->PlatformUpdateInput();

        Move(deltaTime);
        UpdateGame(deltaTime);

        _Renderer->RenderScene(_GameState);
        _Renderer->RenderText(_GameState,
            _GameResolutionWidth, _GameResolutionHeight,
            fpsStr, 0, 0, textScale, textColor);
        _Renderer->RenderText(_GameState,
            _GameResolutionWidth, _GameResolutionHeight,
            "www123", 0, 21, 0.4f, textColor);
        _Renderer->PresentSwapChain(_VSync);

        auto currentTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = currentTime - previousTime;
        previousTime = currentTime;
        deltaTime = static_cast<float>(elapsed.count());
    }
}

void InitGame(int gameResolutionWidth, int gameResolutionHeight)
{
    //Camera information
    _GameState.MainCamera.Position = { 0.0f, 3.0f, -8.0f };

    V3 target = { 0.0f, 0.0f, 0.0f };
	V3 direction = Normalize(target - _GameState.MainCamera.Position);

    _GameState.MainCamera.Direction = direction;
    _GameState.MainCamera.Up = { 0.0f, 1.0f, 0.0f };

    //Set the View matrix
    _GameState.MainCamera.View = MatrixLookAt(
        _GameState.MainCamera.Position, 
        target - _GameState.MainCamera.Position,
        _GameState.MainCamera.Up);

    //Set the Projection matrix
	float nearPlane = 0.1f;
	float farPlane = 1000.0f;
    _GameState.MainCamera.Projection = MatrixPerspective(
        0.4f * 3.14f, static_cast<float>(gameResolutionWidth) / gameResolutionHeight, nearPlane, farPlane);
    
    auto fontGlyphs = LoadFontGlyphs("C:/Windows/Fonts/calibri.ttf");
    _GameState.LoadedFontGlyphs = std::move(fontGlyphs);

    _GameState.GlobalDirectionalLight.Direction = { .X = -0.25f, .Y = -0.5f, .Z = -1.0f };
    _GameState.GlobalDirectionalLight.Ambient = { .X = 0.15f, .Y = 0.15f, .Z = 0.15f };
    _GameState.GlobalDirectionalLight.Diffuse = { .X = 0.8f, .Y = 0.8f, .Z = 0.8f };
}

void Move(float dt)
{
    float moveSpeed = 5.0f * dt;

    if (_Platform->IsKeyDown(KeyCode::KEY_ESCAPE))
    {
		_Running = false;
    }
    if (_Platform->IsKeyDown(KeyCode::KEY_W))
    {
        _GameState.MainCamera.Position.Z += moveSpeed;
    }
    if (_Platform->IsKeyDown(KeyCode::KEY_S))
    {
        _GameState.MainCamera.Position.Z -= moveSpeed;
	}
    if (_Platform->IsKeyDown(KeyCode::KEY_A))
    {
        _GameState.MainCamera.Position.X -= moveSpeed;
	}
    if (_Platform->IsKeyDown(KeyCode::KEY_D))
    {
        _GameState.MainCamera.Position.X += moveSpeed;
    }
    if (_Platform->IsKeyDown(KeyCode::KEY_SPACE))
    {
        _GameState.MainCamera.Position.Y += moveSpeed;
    }
    if (_Platform->IsKeyDown(KeyCode::KEY_LEFT_CTRL))
    {
        _GameState.MainCamera.Position.Y -= moveSpeed;
    }

    if (_Platform->IsKeyPressed(KeyCode::KEY_F1))
    {
        _VSync = !_VSync;
	}
    if (_Platform->IsKeyPressed(KeyCode::KEY_F2))
    {
        _EditMode = !_EditMode;
    }

    if (_EditMode)
    {
        if (_ShowCursor)
        {
            _ShowCursor = false;
			_Platform->PlatformConfineCursorToWindow(true);
            _Platform->PlatformShowCursor(false);
		}

        // _Platform->CenterMousePosition();
        UpdateCamera(dt);
    }
    else
    {
        if (!_ShowCursor)
        {
            _ShowCursor = true;
            _Platform->PlatformConfineCursorToWindow(false);
            _Platform->PlatformShowCursor(_ShowCursor);
        }
    }
}

void UpdateCamera(const float dt)
{
    V2 delta = _Platform->GetMouseDelta();
    const float sensitivity = 1.0f;

    // Adjust yaw/pitch
    yaw -= delta.X * sensitivity;
    pitch -= delta.Y * sensitivity;

    // Clamp pitch
    if (pitch > 89.0f)  pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    // Wrap yaw
    yaw = fmodf(yaw, 360.0f);
    if (yaw < 0.0f) yaw += 360.0f;

    // Convert to direction vector
    V3 direction{};
    direction.X = cosf(DegreesToRadians(yaw)) * cosf(DegreesToRadians(pitch));
    direction.Y = sinf(DegreesToRadians(pitch));
    direction.Z = sinf(DegreesToRadians(yaw)) * cosf(DegreesToRadians(pitch));
    direction = Normalize(direction);

    _GameState.MainCamera.Direction = direction;

    // LookAt matrix
    V3 target = _GameState.MainCamera.Position + direction;

    _GameState.MainCamera.View = MatrixLookAt(
        _GameState.MainCamera.Position,
        target,
        _GameState.MainCamera.Up
    );

}

void UpdateGame(const float dt)
{
    //Keep the cubes rotating
    _GameState.Rot += 0.0f * dt;  //.5f * dt;
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
