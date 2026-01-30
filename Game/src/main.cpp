#include "pch.h"

#include <game.h>
#include <assets/model_loader.h>
#include <assets/animator.h>

#ifdef _WIN32
#include <platform/win32_platform.h>
#include <renderer/d3d11_renderer.h>
#endif

#include <stb_image.h>

void Init();
void Run();
void Shutdown();

void Move(float dt, GameMemory* gameState);
void InitGame(int gameResolutionWidth, int gameResolutionHeight, GameMemory* gameState);
void UploadMeshesToGPU(GameMemory* gameState);
void UpdateGame(const float dt, GameMemory* gameState);
void UpdateCamera(const float dt, GameMemory* gameState);

Entity LoadTerrain(const std::string& path, const V3& offset);

// TODO: Make a platform specific read file function.
std::string ReadEntireFile(const std::string& path);
std::unordered_map<char, FontGlyph> LoadFontGlyphs(const std::string& path, Renderer* renderer);

Sound GenerateSineWave(uint32_t sampleRate,
    float frequency, float durationSeconds);

static std::unique_ptr<GameMemory> _GameState;

static std::unique_ptr<Platform> _Platform;
static std::unique_ptr<Renderer> _Renderer;

static uint32_t _WindowWidth = 1280;
static uint32_t _WindowHeight = 720;

static uint32_t _GameResolutionWidth = 1280;
static uint32_t _GameResolutionHeight = 720;

static std::unordered_map<char, FontGlyph> _LoadedFontGlyphs{};

std::unordered_map<char, FontGlyph> LoadedFontGlyphs{};

static bool _Running{};

static int _FPS{};
static bool _VSync{ true };
static bool _EditMode{};
static bool _ShowCursor{ true };
static float _MouseSensitivity = 0.1f;

uint32_t _SampleRate = 44100;
Sound _SineWave{};

void Init()
{
    _GameResolutionWidth = std::min<uint32_t>(_WindowWidth, _GameResolutionWidth);
	_GameResolutionHeight = std::min<uint32_t>(_WindowHeight, _GameResolutionHeight);

#ifdef _WIN32
    _Platform = std::make_unique<Win32Platform>();
    _Renderer = std::make_unique<D3D11Renderer>();
#endif
    Assert(_Platform && _Renderer);

    _GameState = std::make_unique<GameMemory>();

    _Platform->InitWindow(_WindowWidth, _WindowHeight, L"Window");
	_Platform->InitConsole();
	_Platform->InitInput();
    _Platform->InitAudio();
    _Renderer->InitRenderer(
        _GameResolutionHeight, _GameResolutionWidth, *_Platform, _GameState.get());

    InitGame(_GameResolutionWidth, _GameResolutionHeight, _GameState.get());
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
        const std::string fpsStr = std::format("FPS: {}", _FPS);

        _Platform->UpdateWindow(_Running);
		_Platform->UpdateInput();

        Move(deltaTime, _GameState.get());
        UpdateGame(deltaTime, _GameState.get());

        _Renderer->RenderScene(_GameState.get());

        _Renderer->RenderText(_LoadedFontGlyphs,
            _GameResolutionWidth, _GameResolutionHeight,
            fpsStr, 0, 0, textScale, textColor);

        if (_EditMode)
        {
            _Renderer->RenderText(_LoadedFontGlyphs,
                _GameResolutionWidth, _GameResolutionHeight,
                "Edit mode", 0, 21, textScale, { 0.0f, 1.0f, 0.0f });

            const std::string cameraPosStr = 
                std::format("CameraPos: {:.2f} {:.2f} {:.2f}", 
                    _GameState.get()->MainCamera.Position.X,
                    _GameState.get()->MainCamera.Position.Y,
                    _GameState.get()->MainCamera.Position.Z );

            textScale = 0.6f;

            _Renderer->RenderText(_LoadedFontGlyphs,
                _GameResolutionWidth, _GameResolutionHeight,
                cameraPosStr, 0, 42, textScale, { 1.0f, 1.0f, 1.0f });
        }

        _Renderer->PresentSwapChain(_VSync);

        auto currentTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = currentTime - previousTime;
        previousTime = currentTime;
        deltaTime = static_cast<float>(elapsed.count());
    }
}

void Shutdown()
{
    _Platform->Shutdown();
}

void InitGame(int gameResolutionWidth, int gameResolutionHeight, GameMemory* gameState)
{
    //Camera information
    gameState->MainCamera.Position = { 0.0f, 2.0f, -2.0f };

    V3 target = { 0.0f, 1.0f, 0.0f };
	V3 direction = Normalize(target - gameState->MainCamera.Position);

    gameState->MainCamera.Direction = direction;
    gameState->MainCamera.Up = { 0.0f, 1.0f, 0.0f };

    gameState->MainCamera.Pitch = asinf(direction.Y);
    gameState->MainCamera.Yaw = 90.0f;

    //Set the View matrix
    gameState->MainCamera.View = MatrixLookAt(
        gameState->MainCamera.Position,
        target - gameState->MainCamera.Position,
        gameState->MainCamera.Up);

    //Set the Projection matrix
	float nearPlane = 0.1f;
	float farPlane = 1000.0f;
    gameState->MainCamera.Projection = MatrixPerspective(
        0.5f * 3.14f, static_cast<float>(gameResolutionWidth) / gameResolutionHeight, nearPlane, farPlane);
    
    _LoadedFontGlyphs = LoadFontGlyphs("C:/Windows/Fonts/Calibri.ttf", _Renderer.get());

    gameState->World.DirectionalLight.Direction = { .X = -0.25f, .Y = -0.5f, .Z = -1.0f };
    gameState->World.DirectionalLight.Ambient = { .X = 0.15f, .Y = 0.15f, .Z = 0.15f };
    gameState->World.DirectionalLight.Diffuse = { .X = 0.8f, .Y = 0.8f, .Z = 0.8f };

    UploadMeshesToGPU(_GameState.get());

    //const float frequency = 440.f, duration = 0.2f;
    //_SineWave = GenerateSineWave(_SampleRate, frequency, duration);

    _SineWave = LoadWavFile("assets/audio/jump.wav");
}

void UploadMeshesToGPU(GameMemory* gameState)
{
    Assert(_Renderer);

    Model model{};
    model = ModelLoader::LoadGLTFModel("assets/models/dummy_platformer.gltf");
    for (auto& mesh : model.Meshes)
    {
        _Renderer->UploadMeshesToGPU(mesh);
	}

    Entity& entity = gameState->World.Entities[0];
	entity.Model = model;

    //PlayAnimation(entity.Model.Animator,
    //    &entity.Model.Animations[0], &entity.Model.Skeletons[0], 1.0f, true);

    gameState->World.Entities[1] = LoadTerrain("assets/textures/terrain.png", {0.f, -21.f, 0.f});
    for (auto& mesh : gameState->World.Entities[1].Model.Meshes)
    {
        _Renderer->UploadMeshesToGPU(mesh);
    }
}

void Move(float dt, GameMemory* gameState)
{
    float moveSpeed = 5.0f * dt;
    const V3& forward = Normalize(gameState->MainCamera.Direction);
    V3 right = Normalize(Cross(forward, gameState->MainCamera.Up));

    if (_Platform->IsKeyDown(KeyCode::KEY_ESCAPE))
    {
		_Running = false;
    }
    if (_Platform->IsKeyDown(KeyCode::KEY_W))
    {
        gameState->MainCamera.Position += forward * moveSpeed;
    }
    if (_Platform->IsKeyDown(KeyCode::KEY_S))
    {
        gameState->MainCamera.Position -= forward * moveSpeed;
	}
    if (_Platform->IsKeyDown(KeyCode::KEY_A))
    {
        gameState->MainCamera.Position += right * moveSpeed;
	}
    if (_Platform->IsKeyDown(KeyCode::KEY_D))
    {
        gameState->MainCamera.Position -= right * moveSpeed;
    }
    if (_Platform->IsKeyDown(KeyCode::KEY_SPACE))
    {
        gameState->MainCamera.Position.Y += moveSpeed;
    }
    if (_Platform->IsKeyDown(KeyCode::KEY_LEFT_CTRL))
    {
        gameState->MainCamera.Position.Y -= moveSpeed;
    }
    if (_Platform->IsKeyPressed(KeyCode::KEY_Q))
    {
        _Platform->PlayAudio(_SineWave ,0.2f);
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

        UpdateCamera(dt, _GameState.get());
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

void UpdateCamera(const float dt, GameMemory* gameState)
{
    const V2& delta = _Platform->GetMouseDelta();
	Camera& c = gameState->MainCamera;

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

Entity LoadTerrain(const std::string& path, const V3& offset)
{
    Entity result{};

    int width, height, nChannels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nChannels, 0);
    if (!data)
    {
        std::println("Failed to load heightmap: {}", path);
        return result;
    }

    const float yScale = 0.25f;
    const float yShift = 16.0f;

    int rez = 1;
    if (rez > std::min<int>(width, height)) rez = std::min<int>(width, height);

    const int wSteps = width / rez;
    const int hSteps = height / rez;

    std::vector<Vertex> vertices;
    vertices.reserve(static_cast<size_t>(wSteps * hSteps));

    for (int z = 0; z < height; z += rez)
    {
        for (int x = 0; x < width; x += rez)
        {
            unsigned char* pixel = data + (x + width * z) * nChannels;
            float heightValue = static_cast<float>(pixel[0]) * yScale - yShift;

            Vertex v{};
            v.Position.X = (-wSteps / 2.0f + (x / (float)rez)) + offset.X;
            v.Position.Y = heightValue + offset.Y;
            v.Position.Z = (-hSteps / 2.0f + (z / (float)rez)) + offset.Z;

            v.TexCoord.X = static_cast<float>(x) / (width - 1);
            v.TexCoord.Y = static_cast<float>(z) / (height - 1);

            vertices.emplace_back(v);
        }
    }

    std::println("Loaded {} vertices (rez = {}).", vertices.size(), rez);

    std::vector<uint32_t> indices;
    indices.reserve(static_cast<size_t>((wSteps - 1) * (hSteps - 1) * 6));

    for (int z = 0; z < hSteps - 1; z++)
    {
        for (int x = 0; x < wSteps - 1; x++)
        {
            uint32_t topLeft = x + wSteps * z;
            uint32_t topRight = (x + 1) + wSteps * z;
            uint32_t bottomLeft = x + wSteps * (z + 1);
            uint32_t bottomRight = (x + 1) + wSteps * (z + 1);

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    std::println("Loaded {} indices.", indices.size());

    std::vector<V3> normals(vertices.size(), { 0, 0, 0 });

    for (size_t i = 0; i < indices.size(); i += 3)
    {
        uint32_t i0 = indices[i];
        uint32_t i1 = indices[i + 1];
        uint32_t i2 = indices[i + 2];

        const V3& v0 = vertices[i0].Position;
        const V3& v1 = vertices[i1].Position;
        const V3& v2 = vertices[i2].Position;

        V3 e1 = v1 - v0;
        V3 e2 = v2 - v0;
        V3 normal = Normalize(Cross(e1, e2));

        normals[i0] += normal;
        normals[i1] += normal;
        normals[i2] += normal;
    }

    for (size_t i = 0; i < vertices.size(); i++)
        vertices[i].Normal = Normalize(normals[i]);

    Texture tex{};
    tex.Width = width;
    tex.Height = height;
    tex.Pixels.assign(data, data + (width * height * nChannels));

    stbi_image_free(data);

    Mesh mesh{};
    mesh.Vertices = std::move(vertices);
    mesh.Indices = std::move(indices);
    mesh.Textures.emplace_back(std::move(tex));

    Model model{};
    model.Meshes.emplace_back(std::move(mesh));

    result.Model = std::move(model);

    return result;
}

Sound GenerateSineWave(uint32_t sampleRate, 
    float frequency, float durationSeconds)
{
    Assert(sampleRate > 0);

    Sound result{};

    size_t samples = static_cast<size_t>(sampleRate * durationSeconds);
    std::vector<float> buffer(samples * 2);

    for (size_t i = 0; i < samples; ++i)
    {
        float t = static_cast<float>(i) / sampleRate;
        float value = sinf(2.0f * 3.14159f * frequency * t);

        buffer[i * 2 + 0] = value; // Left
        buffer[i * 2 + 1] = value; // Right
    }

    result.AudioBuffer = buffer;

    return result;
}

void UpdateGame(const float dt, GameMemory* gameState)
{
    //Keep the cubes rotating
    static float angle = {};
    angle += 0.5f * dt;
    if (angle > 6.28f)
        angle = 0.0f;

    V3 lightDir = Normalize({ 0.5f, 1.0f, 0.5f });
    gameState->World.DirectionalLight.Ambient = { 0.4f, 0.4f, 0.4f };
    gameState->World.DirectionalLight.Color = { 1.0f, 1.0f, 1.0f };
    gameState->World.DirectionalLight.Direction = { lightDir.X, lightDir.Y, lightDir.Z, 0.0f };

    for (int i = 0; i < MAX_ENTITIES; i++)
    {
		Entity& entity = gameState->World.Entities[i];

        //UpdateAnimator(entity.Model.Animator, dt);

		M4 translation = MatrixTranslation(0.0f, 0.0f, i * 2.5f);
        M4 rotation = MatrixIdentity();
        if (i == 0)
		    rotation = MatrixRotationY(angle);
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
                const size_t dst = static_cast<size_t>(y * width + x) * 4;
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
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    Init();
    Run();
	Shutdown();

    return 0;
}
#endif
