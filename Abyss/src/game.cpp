#include "SDL3/SDL.h"

#include <iostream>
#include <stdint.h>

// Windows includes
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>

#include <deque>
#include <functional>
#include <ranges>

constexpr int windowWidth = 1280;
constexpr int windowHeight = 720;

constexpr int Width = 640;
constexpr int Height = 360;

IDXGISwapChain* SwapChain;
ID3D11Device* d3d11Device;
ID3D11DeviceContext* d3d11DevCon;
ID3D11RenderTargetView* renderTargetView;

struct DeletionQueue
{
    std::deque<std::function<void()>> DeletionFunctions;

    void PushFunction(std::function<void()>&& function)
    {
        DeletionFunctions.push_back(function);
    }

    void Flush()
    {
        // reverse iterate the deletion queue to execute all the functions
        for (auto& deletionFunction : std::ranges::reverse_view(DeletionFunctions))
        {
            deletionFunction(); //call functors
        }

        DeletionFunctions.clear();
    }
};

DeletionQueue D3D11DeletionQueue;
    
struct SDL_Window* window{};

float red = 0.0f;
float green = 0.0f;
float blue = 0.0f;
int colormodr = 1;
int colormodg = 1;
int colormodb = 1;

static void ExitIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
		char msg[256];
        SDL_Log("Error: %s (HRESULT: 0x%08X)", msg, hr);
        exit(EXIT_FAILURE);
    }
}

void Init()
{
    // We initialize SDL and create a window with it.
    SDL_Init(SDL_INIT_VIDEO);

    window = SDL_CreateWindow(
        " ",
        windowWidth,
        windowHeight,
        0);

    if (!window)
    {
		SDL_Log("Failed to create SDL window: %s", SDL_GetError());
        return;
    }
    
    SDL_PropertiesID props = SDL_GetWindowProperties(window);
    HWND hwnd = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);

    if (!hwnd) 
    {
        SDL_Log("Failed to get HWND: %s", SDL_GetError());
        return;
    }

    //Describe our Buffer
    DXGI_MODE_DESC bufferDesc;
    ZeroMemory(&bufferDesc, sizeof(DXGI_MODE_DESC));

    bufferDesc.Width = Width;
    bufferDesc.Height = Height;
    bufferDesc.RefreshRate.Numerator = 60;
    bufferDesc.RefreshRate.Denominator = 1;
    bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    //Describe our SwapChain
    DXGI_SWAP_CHAIN_DESC swapChainDesc;

    ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));

    swapChainDesc.BufferDesc = bufferDesc;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 1;
    swapChainDesc.OutputWindow = hwnd;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    //Create our SwapChain
    ExitIfFailed(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, NULL, NULL,
        D3D11_SDK_VERSION, &swapChainDesc, &SwapChain, &d3d11Device, NULL, &d3d11DevCon));
	D3D11DeletionQueue.PushFunction([=]() { 
        SwapChain->Release(); 
        d3d11Device->Release();
        d3d11DevCon->Release();
        });


    //Create our BackBuffer
    ID3D11Texture2D* BackBuffer;
    ExitIfFailed(SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer));

    //Create our Render Target
    ExitIfFailed(d3d11Device->CreateRenderTargetView(BackBuffer, NULL, &renderTargetView));
    BackBuffer->Release();

    //Set our Render Target
    d3d11DevCon->OMSetRenderTargets(1, &renderTargetView, NULL);
}

void Cleanup()
{
	D3D11DeletionQueue.Flush();
    SDL_DestroyWindow(window);
}

void Update()
{
    //Update the colors of our scene
    red += colormodr * 0.00005f;
    green += colormodg * 0.00002f;
    blue += colormodb * 0.00001f;

    if (red >= 1.0f || red <= 0.0f)
        colormodr *= -1;
    if (green >= 1.0f || green <= 0.0f)
        colormodg *= -1;
    if (blue >= 1.0f || blue <= 0.0f)
        colormodb *= -1;
}

void Draw()
{
    //Clear our backbuffer to the updated color
    FLOAT bgColor[] = { red, green, blue, 1.0f };

    d3d11DevCon->ClearRenderTargetView(renderTargetView, bgColor);

    //Present the backbuffer to the screen
    SwapChain->Present(0, 0);
}

void Run()
{
    SDL_Event e;
    bool bQuit = false;

    while (!bQuit) 
    {
        while (SDL_PollEvent(&e) != 0) 
        {
            if (e.type == SDL_EVENT_QUIT) 
            {
                bQuit = true;
            }
        }
        Update();
        Draw();
    }
}

int main()
{
	Init();
	Run();
	Cleanup();

    return 0;
}
