#include "SDL3/SDL.h"

#include "handmade_math.h"

#include <iostream>
#include <stdint.h>
#include <deque>
#include <functional>
#include <ranges>

// Windows includes
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <comdef.h>

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

struct Vertex
{
	V3 Position;
	V3 Color;
};

constexpr int windowWidth = 1280;
constexpr int windowHeight = 720;

constexpr int Width = 640;
constexpr int Height = 360;

IDXGISwapChain* SwapChain;
ID3D11Device* d3d11Device;
ID3D11DeviceContext* d3d11DevCon;
ID3D11RenderTargetView* renderTargetView;

ID3D11Buffer* squareIndexBuffer;
ID3D11Buffer* squareVertBuffer;
ID3D11VertexShader* VS;
ID3D11PixelShader* PS;
ID3D10Blob* VS_Buffer;
ID3D10Blob* PS_Buffer;
ID3D11InputLayout* vertLayout;

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
        _com_error err(hr);
		LPCTSTR errMsg = err.ErrorMessage();
		char buffer[265];
		WideCharToMultiByte(CP_ACP, 0, errMsg, -1, buffer, sizeof(buffer), NULL, NULL);

        std::printf("HRESULT failed with error: %s", buffer);
        exit(-1);
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
        std::printf("Failed to create SDL window: %s", SDL_GetError());
        return;
    }
    
    SDL_PropertiesID props = SDL_GetWindowProperties(window);
    HWND hwnd = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);

    if (!hwnd) 
    {
        std::printf("Failed to get HWND: %s", SDL_GetError());
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

    UINT createDeviceFlags = 0;
#if defined(_DEBUG)
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    //Create our SwapChain
    ExitIfFailed(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, NULL, NULL,
        D3D11_SDK_VERSION, &swapChainDesc, &SwapChain, &d3d11Device, NULL, &d3d11DevCon));

    //Create our BackBuffer
    ID3D11Texture2D* BackBuffer;
    ExitIfFailed(SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer));

    //Create our Render Target
    ExitIfFailed(d3d11Device->CreateRenderTargetView(BackBuffer, NULL, &renderTargetView));
    BackBuffer->Release();

    //Set our Render Target
    d3d11DevCon->OMSetRenderTargets(1, &renderTargetView, NULL);

    LPCWSTR shaderPath = L"assets/shaders/shaders.hlsl";

    //Create the Shader Objects
    ExitIfFailed(D3DCompileFromFile(
        shaderPath,
        nullptr,
        nullptr,
        "VSMain",
        "vs_5_0",
        0,
        0,
        &VS_Buffer,
        nullptr));

    ExitIfFailed(D3DCompileFromFile(
        shaderPath,
        nullptr,
        nullptr,
        "PSMain",
        "ps_5_0",
        0,
        0,
        &PS_Buffer,
        nullptr));

    ExitIfFailed(d3d11Device->CreateVertexShader(VS_Buffer->GetBufferPointer(), VS_Buffer->GetBufferSize(), NULL, &VS));
    ExitIfFailed(d3d11Device->CreatePixelShader(PS_Buffer->GetBufferPointer(), PS_Buffer->GetBufferSize(), NULL, &PS));

    //Set Initial Vertex and Pixel Shaders
    d3d11DevCon->VSSetShader(VS, 0, 0);
    d3d11DevCon->PSSetShader(PS, 0, 0);

    //Create the vertex buffer (square)
    Vertex v[] =
    {
        {{-0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f} },
        {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f} },
        {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f} },
        {{-0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 0.0f} },
    };

    uint32_t indices[] =
    {
        0, 1, 2,
        2, 3, 0
	};

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = ARRAYSIZE(layout);

    D3D11_BUFFER_DESC indexBufferDesc;
    ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));

    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.ByteWidth = sizeof(DWORD) * 2 * 3;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = 0;
    indexBufferDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA iinitData;

    iinitData.pSysMem = indices;
    d3d11Device->CreateBuffer(&indexBufferDesc, &iinitData, &squareIndexBuffer);

    d3d11DevCon->IASetIndexBuffer(squareIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

    D3D11_BUFFER_DESC vertexBufferDesc;
    ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));

    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.ByteWidth = sizeof(Vertex) * 4;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;
    vertexBufferDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA vertexBufferData;

    ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
    vertexBufferData.pSysMem = v;
    ExitIfFailed(d3d11Device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &squareVertBuffer));

    //Set the vertex buffer
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    d3d11DevCon->IASetVertexBuffers(0, 1, &squareVertBuffer, &stride, &offset);

    //Create the Input Layout
    d3d11Device->CreateInputLayout(layout, numElements, VS_Buffer->GetBufferPointer(),
        VS_Buffer->GetBufferSize(), &vertLayout);

    //Set the Input Layout
    d3d11DevCon->IASetInputLayout(vertLayout);

    //Set Primitive Topology
    d3d11DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    //Create the Viewport
    D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = Width;
    viewport.Height = Height;

    //Set the Viewport
    d3d11DevCon->RSSetViewports(1, &viewport);

    D3D11DeletionQueue.PushFunction([=]() {
        SwapChain->Release();
        d3d11Device->Release();
        d3d11DevCon->Release();
        squareVertBuffer->Release();
        squareIndexBuffer->Release();
        VS->Release();
        PS->Release();
        VS_Buffer->Release();
        PS_Buffer->Release();
        vertLayout->Release();
        });
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
    //Clear our backbuffer
    float bgColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    d3d11DevCon->ClearRenderTargetView(renderTargetView, bgColor);

    //Draw the square
    d3d11DevCon->DrawIndexed(6, 0, 0);

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
