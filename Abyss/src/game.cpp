#include "SDL3/SDL.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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
    V2 TexCoord;
};

struct Texture
{
    int Width;
    int Height;
    unsigned char* Pixels;
};

struct cbPerObject
{
    M4  WVP;
};

constexpr int windowWidth = 1280;
constexpr int windowHeight = 720;

constexpr int GameResolutionWidth = 640;
constexpr int GameResolutionHeight = 360;

IDXGISwapChain* SwapChain;
ID3D11Device* d3d11Device;
ID3D11DeviceContext* d3d11DevCon;
ID3D11RenderTargetView* renderTargetView;
ID3D11DepthStencilView* depthStencilView;
ID3D11Texture2D* depthStencilBuffer;

ID3D11Buffer* CubesIndexBuffer;
ID3D11Buffer* CubesVertBuffer;
ID3D11VertexShader* VS;
ID3D11PixelShader* PS;
ID3D10Blob* VS_Buffer;
ID3D10Blob* PS_Buffer;
ID3D11InputLayout* vertLayout;
ID3D11Buffer* cbPerObjectBuffer;
ID3D11RasterizerState* Solid;
ID3D11RasterizerState* WireFrame;

ID3D11ShaderResourceView* CubesTextureView;
ID3D11SamplerState* CubesTexSamplerState;
ID3D11BlendState* Transparency;
ID3D11RasterizerState* CCWcullMode;
ID3D11RasterizerState* CWcullMode;
ID3D11RasterizerState* noCull;

cbPerObject cbPerObj;

M4 WVP;
M4 cube1World;
M4 cube2World;
M4 camView;
M4 camProjection;

V3 camPosition;
V3 camTarget;
V3 camUp;

DeletionQueue D3D11DeletionQueue;
    
struct SDL_Window* window{};

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

Texture CreateErrorTexture()
{
	const int width = 256;
	const int height = 256;
	unsigned char* pixels = (unsigned char*)malloc(width * height * 4);
	// Checkerboard pattern

	int checkSize = 16;
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			int index = (y * width + x) * 4;
			if (((x / checkSize) % 2) == ((y / checkSize) % 2))
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

	Texture texture;
	texture.Width = width;
	texture.Height = height;
	texture.Pixels = pixels;
	return texture;
}

Texture LoadTexture(const char* filename)
{
    int width, height, channels;
    unsigned char* data = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha);
    if (!data)
    {
        // If loading fails, return an error texture
        return CreateErrorTexture();
    }
    Texture texture;
    texture.Width = width;
    texture.Height = height;
    texture.Pixels = data;
    return texture;
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

    //////////////////////////////////
    // Init D3D11                   //
    //////////////////////////////////

    //Describe our Buffer
    DXGI_MODE_DESC bufferDesc;
    ZeroMemory(&bufferDesc, sizeof(DXGI_MODE_DESC));

    bufferDesc.Width = GameResolutionWidth;
    bufferDesc.Height = GameResolutionHeight;
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

    //Describe our Depth/Stencil Buffer
    D3D11_TEXTURE2D_DESC depthStencilDesc;

    depthStencilDesc.Width = GameResolutionWidth;
    depthStencilDesc.Height = GameResolutionHeight;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.ArraySize = 1;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
    depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStencilDesc.CPUAccessFlags = 0;
    depthStencilDesc.MiscFlags = 0;

    //Create the Depth/Stencil View
    ExitIfFailed(d3d11Device->CreateTexture2D(&depthStencilDesc, NULL, &depthStencilBuffer));
    ExitIfFailed(d3d11Device->CreateDepthStencilView(depthStencilBuffer, NULL, &depthStencilView));

    //Set our Render Target
    d3d11DevCon->OMSetRenderTargets(1, &renderTargetView, depthStencilView);

    //////////////////////////////////
    // Init Rendering Pipeline      //
    //////////////////////////////////
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

    //Create the vertex buffer (cube)
    std::vector<Vertex> v =
    {
        // Front Face (Z = -1)
        { { -1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
        { {  1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f }, {1.0f, 1.0f} },
        { {  1.0f,  1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f }, {1.0f, 0.0f} },
        { { -1.0f,  1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f }, {0.0f, 0.0f} },

        // Back Face (Z = +1)
        { { -1.0f, -1.0f,  1.0f }, { 1.0f, 1.0f, 1.0f }, {1.0f, 1.0f} },
        { {  1.0f, -1.0f,  1.0f }, { 1.0f, 1.0f, 1.0f }, {0.0f, 1.0f} },
        { {  1.0f,  1.0f,  1.0f }, { 1.0f, 1.0f, 1.0f }, {0.0f, 0.0f} },
        { { -1.0f,  1.0f,  1.0f }, { 1.0f, 1.0f, 1.0f }, {1.0f, 0.0f} },

        // Top Face (Y = +1)
        { { -1.0f,  1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f }, {0.0f, 1.0f} },
        { { -1.0f,  1.0f,  1.0f }, { 1.0f, 1.0f, 1.0f }, {0.0f, 0.0f} },
        { {  1.0f,  1.0f,  1.0f }, { 1.0f, 1.0f, 1.0f }, {1.0f, 0.0f} },
        { {  1.0f,  1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f }, {1.0f, 1.0f} },

        // Bottom Face (Y = -1)
        { { -1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f }, {1.0f, 1.0f} },
        { {  1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f }, {0.0f, 1.0f} },
        { {  1.0f, -1.0f,  1.0f }, { 1.0f, 1.0f, 1.0f }, {0.0f, 0.0f} },
        { { -1.0f, -1.0f,  1.0f }, { 1.0f, 1.0f, 1.0f }, {1.0f, 0.0f} },

        // Left Face (X = -1)
        { { -1.0f, -1.0f,  1.0f }, { 1.0f, 1.0f, 1.0f }, {0.0f, 1.0f} },
        { { -1.0f,  1.0f,  1.0f }, { 1.0f, 1.0f, 1.0f }, {0.0f, 0.0f} },
        { { -1.0f,  1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f }, {1.0f, 0.0f} },
        { { -1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f }, {1.0f, 1.0f} },

        // Right Face (X = +1)
        { {  1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f }, {0.0f, 1.0f} },
        { {  1.0f,  1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f }, {0.0f, 0.0f} },
        { {  1.0f,  1.0f,  1.0f }, { 1.0f, 1.0f, 1.0f }, {1.0f, 0.0f} },
		{ {  1.0f, -1.0f,  1.0f }, { 1.0f, 1.0f, 1.0f }, {1.0f, 1.0f} },
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

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = ARRAYSIZE(layout);

    D3D11_BUFFER_DESC indexBufferDesc;
    ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));

    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.ByteWidth = sizeof(indices);
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = 0;
    indexBufferDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA iinitData;

    iinitData.pSysMem = indices;
    d3d11Device->CreateBuffer(&indexBufferDesc, &iinitData, &CubesIndexBuffer);

    d3d11DevCon->IASetIndexBuffer(CubesIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

    D3D11_BUFFER_DESC vertexBufferDesc;
    ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));

    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(Vertex) * static_cast<UINT>(v.size());
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;
    vertexBufferDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA vertexBufferData;

    ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
    vertexBufferData.pSysMem = v.data();
    ExitIfFailed(d3d11Device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &CubesVertBuffer));

    //Set the vertex buffer
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    d3d11DevCon->IASetVertexBuffers(0, 1, &CubesVertBuffer, &stride, &offset);

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
    viewport.Width = GameResolutionWidth;
    viewport.Height = GameResolutionHeight;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    //Set the Viewport
    d3d11DevCon->RSSetViewports(1, &viewport);

    //Create the buffer to send to the cbuffer in effect file
    D3D11_BUFFER_DESC cbbd;
    ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));

    cbbd.Usage = D3D11_USAGE_DEFAULT;
    cbbd.ByteWidth = sizeof(cbPerObject);
    cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbbd.CPUAccessFlags = 0;
    cbbd.MiscFlags = 0;

    ExitIfFailed(d3d11Device->CreateBuffer(&cbbd, NULL, &cbPerObjectBuffer));

    D3D11_SAMPLER_DESC samplerDesc;
    ZeroMemory(&samplerDesc, sizeof(samplerDesc));
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    ExitIfFailed(d3d11Device->CreateSamplerState(&samplerDesc, &CubesTexSamplerState));

	Texture tex = LoadTexture("assets/textures/cage.png");

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = tex.Width;
    desc.Height = tex.Height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = tex.Pixels;
    initData.SysMemPitch = tex.Width * 4; // 4 bytes per pixel (RGBA)

    ID3D11Texture2D* texture = nullptr;
	ExitIfFailed(d3d11Device->CreateTexture2D(&desc, &initData, &texture));

    ExitIfFailed(d3d11Device->CreateShaderResourceView(texture, nullptr, &CubesTextureView));
    texture->Release();
    stbi_image_free(tex.Pixels);

    //Define the Blending Equation
    D3D11_BLEND_DESC blendDesc;
    ZeroMemory(&blendDesc, sizeof(blendDesc));

    D3D11_RENDER_TARGET_BLEND_DESC rtbd;
    ZeroMemory(&rtbd, sizeof(rtbd));

    rtbd.BlendEnable = true;
    rtbd.SrcBlend = D3D11_BLEND_SRC_COLOR;
    rtbd.DestBlend = D3D11_BLEND_BLEND_FACTOR;
    rtbd.BlendOp = D3D11_BLEND_OP_ADD;
    rtbd.SrcBlendAlpha = D3D11_BLEND_ONE;
    rtbd.DestBlendAlpha = D3D11_BLEND_ZERO;
    rtbd.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    rtbd.RenderTargetWriteMask = D3D10_COLOR_WRITE_ENABLE_ALL;

    blendDesc.AlphaToCoverageEnable = false;
    blendDesc.RenderTarget[0] = rtbd;

    d3d11Device->CreateBlendState(&blendDesc, &Transparency);

    D3D11_RASTERIZER_DESC solidDesc;
    ZeroMemory(&solidDesc, sizeof(D3D11_RASTERIZER_DESC));
    solidDesc.FillMode = D3D11_FILL_SOLID;
	solidDesc.CullMode = D3D11_CULL_NONE; // TODO: Change to BACK
    ExitIfFailed(d3d11Device->CreateRasterizerState(&solidDesc, &Solid));

    D3D11_RASTERIZER_DESC wfdesc;
    ZeroMemory(&wfdesc, sizeof(D3D11_RASTERIZER_DESC));
    wfdesc.FillMode = D3D11_FILL_WIREFRAME;
    wfdesc.CullMode = D3D11_CULL_NONE;
    ExitIfFailed(d3d11Device->CreateRasterizerState(&wfdesc, &WireFrame));

    D3D11_RASTERIZER_DESC noCullDesc;
    ZeroMemory(&noCullDesc, sizeof(D3D11_RASTERIZER_DESC));
    noCullDesc.FillMode = D3D11_FILL_SOLID;
    noCullDesc.CullMode = D3D11_CULL_NONE;
    d3d11Device->CreateRasterizerState(&noCullDesc, &noCull);

    //Create the Counter Clockwise and Clockwise Culling States
    D3D11_RASTERIZER_DESC cmdesc;
    ZeroMemory(&cmdesc, sizeof(D3D11_RASTERIZER_DESC));
    cmdesc.FillMode = D3D11_FILL_SOLID;
    cmdesc.CullMode = D3D11_CULL_BACK;
    cmdesc.FrontCounterClockwise = true;
    ExitIfFailed(d3d11Device->CreateRasterizerState(&cmdesc, &CCWcullMode));
    cmdesc.FrontCounterClockwise = false;
    ExitIfFailed(d3d11Device->CreateRasterizerState(&cmdesc, &CWcullMode));

    D3D11DeletionQueue.PushFunction([=]() {
        SwapChain->Release();
        d3d11Device->Release();
        d3d11DevCon->Release();
        CubesVertBuffer->Release();
        CubesIndexBuffer->Release();
        VS->Release();
        PS->Release();
        VS_Buffer->Release();
        PS_Buffer->Release();
        vertLayout->Release();
        depthStencilView->Release();
        depthStencilBuffer->Release();
        cbPerObjectBuffer->Release();
        WireFrame->Release();
        Solid->Release();
        Transparency->Release();
        CCWcullMode->Release();
        CWcullMode->Release();
		CubesTextureView->Release();
		CubesTexSamplerState->Release();
        noCull->Release();
        });

    //Camera information
    camPosition = { 0.0f, 3.0f, -8.0f };
    camTarget = { 0.0f, 0.0f, 0.0f };
    camUp = { 0.0f, 1.0f, 0.0f };

    //Set the View matrix
    camView = MatrixLookAt(camPosition, camTarget, camUp);

    //Set the Projection matrix
    camProjection = MatrixPerspective(0.4f * 3.14f, (float)GameResolutionWidth / GameResolutionHeight, 1.0f, 1000.0f);
}

void Cleanup()
{
	D3D11DeletionQueue.Flush();
    SDL_DestroyWindow(window);
}

M4 Rotation;
M4 Scale;
M4 Translation;
float rot = 0.01f;

void Update()
{
    //Keep the cubes rotating
    rot += .00005f;
    if (rot > 6.28f)
        rot = 0.0f;

    //Reset cube1World
    cube1World = MatrixIdentity();

    Rotation = MatrixRotationY(rot);
    Translation = MatrixTranslation(0.0f, 0.0f, 4.0f);

    //Set cube1's world space using the transformations
    cube1World = Translation * Rotation;

    //Reset cube2World
    cube2World = MatrixIdentity();

    //Define cube2's world space matrix
    Rotation = MatrixRotationY(rot);
    Scale = MatrixScaling(1.3f, 1.3f, 1.3f);

    //Set cube2's world space matrix
    cube2World = Rotation * Scale;
}

void Draw()
{
    //Clear our backbuffer (sky blue)
	float bgColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    d3d11DevCon->ClearRenderTargetView(renderTargetView, bgColor);

    //Refresh the Depth/Stencil view
    d3d11DevCon->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    ///////////////**************new**************////////////////////
    //Enable the Default Rasterizer State
    d3d11DevCon->RSSetState(NULL);
    //Draw objects that will use backface culling

    //Turn off backface culling
    d3d11DevCon->RSSetState(noCull);
    ///////////////**************new**************////////////////////

    //Set the WVP matrix and send it to the constant buffer in effect file
    WVP = cube1World * camView * camProjection;
    cbPerObj.WVP = MatrixTranspose(WVP);
    d3d11DevCon->UpdateSubresource(cbPerObjectBuffer, 0, NULL, &cbPerObj, 0, 0);
    d3d11DevCon->VSSetConstantBuffers(0, 1, &cbPerObjectBuffer);
    d3d11DevCon->PSSetShaderResources(0, 1, &CubesTextureView);
    d3d11DevCon->PSSetSamplers(0, 1, &CubesTexSamplerState);

    //Draw the first cube
    d3d11DevCon->DrawIndexed(36, 0, 0);

    WVP = cube2World * camView * camProjection;
    cbPerObj.WVP = MatrixTranspose(WVP);
    d3d11DevCon->UpdateSubresource(cbPerObjectBuffer, 0, NULL, &cbPerObj, 0, 0);
    d3d11DevCon->VSSetConstantBuffers(0, 1, &cbPerObjectBuffer);
    d3d11DevCon->PSSetShaderResources(0, 1, &CubesTextureView);
    d3d11DevCon->PSSetSamplers(0, 1, &CubesTexSamplerState);

    //Draw the second cube
    d3d11DevCon->DrawIndexed(36, 0, 0);

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
