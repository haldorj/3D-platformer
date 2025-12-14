#include "pch.h"

#include "platform/platform.h"

#include "game.h"

int main()
{
    PlatformInitWindow(WindowWidth, WindowHeight, L"Window");
    Init();

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

        const std::string fpsStr = std::format("FPS: {}", FPS);

        PlatformUpdateWindow(running);

        UpdateGame(DeltaTime);

        RenderScene();
        RenderText(fpsStr, 0, 0, 1.0f, { 1.0f, 1.0f, 1.0f });
        PresentSwapChain();

        auto currentTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = currentTime - previousTime;
        previousTime = currentTime;
        DeltaTime = static_cast<float>(elapsed.count());
    }

    return 0;
}
