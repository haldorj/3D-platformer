#include "pch.h"

#include "platform/platform.h"
#include "platform/win32_platform.h"

#include "application.h"
#include "game.h"
#include "renderer/d3d11_renderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"


static constexpr int WindowWidth = 1280;
static constexpr int WindowHeight = 720;

static constexpr int GameResolutionWidth = 640;
static constexpr int GameResolutionHeight = 360;

static bool VSync = true;

static int FPS = 0;

int main()
{
    Application app{};
    GameState gameState{};

#ifdef _WIN32
	std::unique_ptr<Platform> platform = std::make_unique<Win32Platform>();
    std::unique_ptr<Renderer> renderer = std::make_unique<D3D11Renderer>();
#endif

    platform->PlatformInitWindow(WindowWidth, WindowHeight, L"Window");

    renderer->InitRenderer(
		GameResolutionHeight, GameResolutionWidth, *platform, gameState, app);

    app.InitGame(GameResolutionWidth, GameResolutionHeight, gameState, *renderer);

    bool running = true;

    auto previousTime = std::chrono::steady_clock::now();

    while (running)
    {
        static double fpsTimer{};
        static int fpsFrameCount{};
        static float DeltaTime{};

        fpsTimer += DeltaTime;
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

        platform->PlatformUpdateWindow(running);

        app.UpdateGame(DeltaTime, gameState);

        renderer->RenderScene(gameState);
        renderer->RenderText(gameState, 
            GameResolutionWidth, GameResolutionHeight, 
            fpsStr, 0, 0, textScale, textColor);
        renderer->RenderText(gameState, 
            GameResolutionWidth, GameResolutionHeight,
            "www123", 0, 21, 0.4f, textColor);
        renderer->PresentSwapChain(VSync);

        auto currentTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = currentTime - previousTime;
        previousTime = currentTime;
        DeltaTime = static_cast<float>(elapsed.count());
    }

    return 0;
}
