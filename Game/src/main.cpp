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
void UploadMeshesToGPU();
void UpdateGame(const float dt);
void UpdateCamera(const float dt);

[[nodiscard]] std::string ReadEntireFile(const std::string& path);
[[nodiscard]] Texture CreateErrorTexture();
[[nodiscard]] Texture LoadTextureFromFile(const std::string& path);
[[nodiscard]] std::unordered_map<char, FontGlyph> LoadFontGlyphs(const std::string& path, Renderer* renderer);

static GameState _GameState;

static std::unique_ptr<Platform> _Platform;
static std::unique_ptr<Renderer> _Renderer;

constinit uint32_t _WindowWidth = 1280;
constinit uint32_t _WindowHeight = 720;

constinit uint32_t _GameResolutionWidth = 1280;
constinit uint32_t _GameResolutionHeight = 720;

constinit bool _Running{};

constinit int _FPS{};
constinit bool _VSync{ true };
constinit bool _EditMode{};
constinit bool _ShowCursor{ true };
constinit float _MouseSensitivity = 0.1f;

void Init()
{
    _GameResolutionWidth = std::min<uint32_t>(_WindowWidth, _GameResolutionWidth);
	_GameResolutionHeight = std::min<uint32_t>(_WindowHeight, _GameResolutionHeight);

#ifdef _WIN32
    _Platform = std::make_unique<Win32Platform>();
    _Renderer = std::make_unique<D3D11Renderer>();
#endif
    Assert(_Platform && _Renderer);

    _Platform->InitWindow(_WindowWidth, _WindowHeight, L"Window");
	_Platform->InitInput();
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

        _Platform->UpdateWindow(_Running);
		_Platform->UpdateInput();

        Move(deltaTime);
        UpdateGame(deltaTime);

        _Renderer->RenderScene(_GameState);

        _Renderer->RenderText(_GameState,
            _GameResolutionWidth, _GameResolutionHeight,
            fpsStr, 0, 0, textScale, textColor);

        if (_EditMode)
        {
            _Renderer->RenderText(_GameState,
                _GameResolutionWidth, _GameResolutionHeight,
                "Edit mode", 0, 21, textScale, { 1.0f, 0.0f, 0.0f });
        }

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

    _GameState.MainCamera.Pitch = asinf(direction.Y);
    _GameState.MainCamera.Yaw = 90.0f;

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
    
    auto fontGlyphs = LoadFontGlyphs("C:/Windows/Fonts/Calibri.ttf", _Renderer.get());
    _GameState.LoadedFontGlyphs = std::move(fontGlyphs);

    _GameState.GlobalDirectionalLight.Direction = { .X = -0.25f, .Y = -0.5f, .Z = -1.0f };
    _GameState.GlobalDirectionalLight.Ambient = { .X = 0.15f, .Y = 0.15f, .Z = 0.15f };
    _GameState.GlobalDirectionalLight.Diffuse = { .X = 0.8f, .Y = 0.8f, .Z = 0.8f };

    UploadMeshesToGPU();
}

Texture LoadTextureFromFile(const std::string& path)
{
    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

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

Texture CreateErrorTexture()
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

void UploadMeshesToGPU()
{
    Assert(_Renderer);

    //Create the vertex buffer (cube)
    std::vector<Vertex> vertices =
    {
        // Front Face
        { { -1.0f, -1.0f, -1.0f }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 1.0f } },
        { { -1.0f,  1.0f, -1.0f }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 0.0f } },
        { {  1.0f,  1.0f, -1.0f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 0.0f } },
        { {  1.0f, -1.0f, -1.0f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 1.0f } },

        // Back Face
        { { -1.0f, -1.0f,  1.0f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 1.0f } },
        { {  1.0f, -1.0f,  1.0f }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 1.0f } },
        { {  1.0f,  1.0f,  1.0f }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 0.0f } },
        { { -1.0f,  1.0f,  1.0f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 0.0f } },

        // Top Face
        { { -1.0f,  1.0f, -1.0f }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 1.0f } },
        { { -1.0f,  1.0f,  1.0f }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 0.0f } },
        { {  1.0f,  1.0f,  1.0f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 0.0f } },
        { {  1.0f,  1.0f, -1.0f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 1.0f } },

        // Bottom Face
        { { -1.0f, -1.0f, -1.0f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 1.0f } },
        { {  1.0f, -1.0f, -1.0f }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 1.0f } },
        { {  1.0f, -1.0f,  1.0f }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 0.0f } },
        { { -1.0f, -1.0f,  1.0f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 0.0f } },

        // Left Face
        { { -1.0f, -1.0f,  1.0f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } },
        { { -1.0f,  1.0f,  1.0f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f } },
        { { -1.0f,  1.0f, -1.0f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f } },
        { { -1.0f, -1.0f, -1.0f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f } },

        // Right Face
        { {  1.0f, -1.0f, -1.0f }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } },
        { {  1.0f,  1.0f, -1.0f }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f } },
        { {  1.0f,  1.0f,  1.0f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f } },
        { {  1.0f, -1.0f,  1.0f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f } },
    };

    uint32_t indices[] =
    {
        // Front Face
        0,  1,  2,
        0,  2,  3,
        // Back Face
        4,  5,  6,
        4,  6,  7,
        // Top Face
        8,  9, 10,
        8, 10, 11,
        // Bottom Face
        12, 13, 14,
        12, 14, 15,
        // Left Face
        16, 17, 18,
        16, 18, 19,
        // Right Face
        20, 21, 22,
        20, 22, 23
    };

	Mesh cubeMesh{};
    Texture tex = LoadTextureFromFile("assets/textures/crate.jpg");

    cubeMesh.Vertices = std::move(vertices);
    cubeMesh.Indices = std::move(std::vector<uint32_t>(std::begin(indices), std::end(indices)));
    cubeMesh.Textures.push_back(std::move(tex));

    // Texture Loading Test
	_Renderer->UploadMeshesToGPU(cubeMesh);
    stbi_image_free(tex.Pixels.data());

	//Assign the cube mesh to all entities
    for (int i = 0; i < MAX_ENTITIES; i++)
    {
        Entity& entity = _GameState.World.Entities[i];
        entity.Mesh = cubeMesh;
	}
}

void Move(float dt)
{
    float moveSpeed = 5.0f * dt;
    const V3& forward = Normalize(_GameState.MainCamera.Direction);
    V3 right = Normalize(Cross(forward, _GameState.MainCamera.Up));

    if (_Platform->IsKeyDown(KeyCode::KEY_ESCAPE))
    {
		_Running = false;
    }
    if (_Platform->IsKeyDown(KeyCode::KEY_W))
    {
        _GameState.MainCamera.Position += forward * moveSpeed;
    }
    if (_Platform->IsKeyDown(KeyCode::KEY_S))
    {
        _GameState.MainCamera.Position -= forward * moveSpeed;
	}
    if (_Platform->IsKeyDown(KeyCode::KEY_A))
    {
        _GameState.MainCamera.Position += right * moveSpeed;
	}
    if (_Platform->IsKeyDown(KeyCode::KEY_D))
    {
        _GameState.MainCamera.Position -= right * moveSpeed;
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
			_Platform->ConfineCursorToWindow(true);
            _Platform->SetCursorVisible(false);
		}

        UpdateCamera(dt);
    }
    else
    {
        if (!_ShowCursor)
        {
            _ShowCursor = true;
            _Platform->ConfineCursorToWindow(false);
            _Platform->SetCursorVisible(true);
        }
    }
    _Platform->SetMouseDelta({ 0.0f, 0.0f });
}

void UpdateCamera(const float dt)
{
    const V2& delta = _Platform->GetMouseDelta();
	Camera& c = _GameState.MainCamera;

    // Adjust yaw/pitch
    c.Yaw -= delta.X * _MouseSensitivity;
    c.Pitch -= delta.Y * _MouseSensitivity;

    // Clamp pitch
    if (c.Pitch > 89.0f)
        c.Pitch = 89.0f;
    if (c.Pitch < -89.0f)
        c.Pitch = -89.0f;

    // Wrap yaw
    c.Yaw = fmodf(c.Yaw, 360.0f);
    if (c.Yaw < 0.0f) c.Yaw += 360.0f;

    // Convert to direction vector
    V3 direction{};

    direction.X = 
        cosf(DegreesToRadians(c.Yaw)) * cosf(DegreesToRadians(c.Pitch));
    direction.Y = 
        sinf(DegreesToRadians(c.Pitch));
    direction.Z = 
        sinf(DegreesToRadians(c.Yaw)) * cosf(DegreesToRadians(c.Pitch));

    direction = Normalize(direction);

    c.Direction = direction;

    // LookAt matrix
    V3 target = c.Position + direction;

    c.View = MatrixLookAt(c.Position, target, c.Up);

}

void UpdateGame(const float dt)
{
    //Keep the cubes rotating
    _GameState.Rot += 0.5f * dt;
    if (_GameState.Rot > 6.28f)
        _GameState.Rot = 0.0f;

    V3 lightDir = Normalize({ 0.5f, 1.0f, 0.5f });
    _GameState.GlobalDirectionalLight.Ambient = { 0.4f, 0.4f, 0.4f };
    _GameState.GlobalDirectionalLight.Color = { 1.0f, 1.0f, 1.0f };
    _GameState.GlobalDirectionalLight.Direction = { lightDir.X, lightDir.Y, lightDir.Z, 0.0f };

    for (int i = 0; i < MAX_ENTITIES; i++)
    {
		Entity& entity = _GameState.World.Entities[i];

		entity.WorldMatrix = MatrixIdentity();

		M4 translation = MatrixTranslation(0.0f, 0.0f, i * 2.5f);
		M4 rotation = MatrixRotationY(_GameState.Rot);
		M4 scale = MatrixScaling(1.0f, 1.0f, 1.0f);

		entity.WorldMatrix = scale * translation * rotation;
    }
}

std::unordered_map<char, FontGlyph> LoadFontGlyphs(const std::string& path, Renderer* renderer)
{
    std::unordered_map<char, FontGlyph> result;

    Assert(renderer);

    std::string ttfBuffer = ReadEntireFile(path);
    Assert(ttfBuffer.size()); // Ensure the font file was read successfully

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
        glyph.TextureView = renderer->CreateTextureView(fontTexture);
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

#ifdef WIN32
// Winmain for Windows platform
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    Init();
    Run();

    return 0;
}
#endif
