#include "pch.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "game.h"
#include "platform/platform.h"

using Microsoft::WRL::ComPtr;

static DirectionalLight GlobalDirectionalLight{};

ComPtr<IDXGISwapChain1> SwapChain;
ComPtr<ID3D11Device> D3d11Device;
ComPtr<ID3D11DeviceContext> D3d11DeviceContext;

ID3D11RenderTargetView* RenderTargetView;
ID3D11DepthStencilView* DepthStencilView;
ID3D11Texture2D* DepthStencilBuffer;

ID3D11Buffer* CubesIndexBuffer;
ID3D11Buffer* CubesVertBuffer;
ID3D11VertexShader* VS;
ID3D11PixelShader* PS;
ID3D10Blob* VsBuffer;
ID3D10Blob* PsBuffer;
ID3D11InputLayout* VertLayout;

ID3D11Buffer* CbPerObjectBuffer;
ID3D11RasterizerState* Solid;
ID3D11RasterizerState* WireFrame;

ID3D11Buffer* cbPerFrameBuffer;

ID3D11Buffer* QuadIndexBuffer;
ID3D11Buffer* QuadVertBuffer;
ID3D11VertexShader* FontVS;
ID3D11PixelShader* FontPS;
ID3D10Blob* FontVsBuffer;
ID3D10Blob* FontPsBuffer;
ID3D11InputLayout* FontVertLayout;

ID3D11ShaderResourceView* CubesTextureView;
ID3D11SamplerState* CubesTexSamplerState;
ID3D11BlendState* Transparency;
ID3D11RasterizerState* CounterClockwiseCullMode;
ID3D11RasterizerState* ClockwiseCullMode;
ID3D11RasterizerState* NoCull;

CbPerObject CbPerObj;
CbPerFrame ConstBufferPerFrame;

M4 Cube1World;
M4 Cube2World;
M4 CamView;
M4 CamProjection;

V3 CamPosition;
V3 CamTarget;
V3 CamUp;

M4 Rotation;
M4 Scale;
M4 Translation;
float Rot = 0.01f;

std::unordered_map<char, FontGlyph> LoadedFontGlyphs{};

static void ExitIfFailed(const HRESULT hr)
{
    if (FAILED(hr))
    {
        const _com_error err(hr);
        const LPCTSTR errMsg = err.ErrorMessage();
        char buffer[265];
        WideCharToMultiByte(CP_ACP, 0, errMsg, -1, buffer, sizeof(buffer), nullptr, nullptr);

        std::print("HRESULT failed with error: {}", buffer);
        assert(false);
    }
}

static void VerifyShader(const HRESULT hr, ID3D10Blob* errorMessages)
{
    if (FAILED(hr) && errorMessages) 
    {
	    auto errorMsg = static_cast<const char*>(errorMessages->GetBufferPointer());
        std::print("Shader Compilation Error: {}\n", errorMsg);
        assert(false);
    }
}

static Texture CreateErrorTexture()
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

static std::string ReadEntireFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file)
    {
        std::print("Failed to open file: {}", filename);
        return {};
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string buffer(size, '\0');
    if (!file.read(buffer.data(), size))
    {
        std::print("Failed to read file: {}", filename);
        return {};
    }
    return buffer;
}

static ID3D11ShaderResourceView* CreateTextureView(const Texture& texture)
{
	ID3D11ShaderResourceView* textureView = nullptr;

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = texture.Width;
    desc.Height = texture.Height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = texture.Pixels.data();
    initData.SysMemPitch = texture.Width * 4; // 4 bytes per pixel (RGBA)

    ID3D11Texture2D* d3d11texture = nullptr;
    ExitIfFailed(D3d11Device->CreateTexture2D(&desc, &initData, &d3d11texture));

    ExitIfFailed(D3d11Device->CreateShaderResourceView(d3d11texture, nullptr, &textureView));
    d3d11texture->Release();

    return textureView;
}

static Texture LoadTexture(const std::string& filename)
{
    int width, height, channels;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);

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


static std::unordered_map<char, FontGlyph> LoadFontGlyphs(const std::string& path)
{
    std::unordered_map<char, FontGlyph> result;

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
		glyph.TextureView = CreateTextureView(fontTexture);
		glyph.Size = {.X = static_cast<float>(width), .Y = static_cast<float>(-height) };
		glyph.Bearing = {.X = static_cast<float>(xOffset), .Y = static_cast<float>(yOffset + pixelHeight) };
		glyph.Advance = scale * static_cast<float>(advance);

        result.emplace(codepoint, glyph);

        stbtt_FreeBitmap(bitmap, nullptr);
    }

    return result;
}

static void InitMainRenderingPipeline()
{
    LPCWSTR shaderPath = L"assets/shaders/shaders.hlsl";

    ID3DBlob* errorMessages = nullptr;

    //Create the Shader Objects
    HRESULT hr = D3DCompileFromFile(
        shaderPath,
        nullptr,
        nullptr,
        "VSMain",
        "vs_5_0",
        0,
        0,
        &VsBuffer,
        &errorMessages);
    VerifyShader(hr, errorMessages);

    hr = D3DCompileFromFile(
	    shaderPath,
	    nullptr,
	    nullptr,
	    "PSMain",
	    "ps_5_0",
	    0,
	    0,
	    &PsBuffer,
	    &errorMessages);
	VerifyShader(hr, errorMessages);

    ExitIfFailed(D3d11Device->CreateVertexShader(VsBuffer->GetBufferPointer(), VsBuffer->GetBufferSize(), nullptr, &VS));
    ExitIfFailed(D3d11Device->CreatePixelShader(PsBuffer->GetBufferPointer(), PsBuffer->GetBufferSize(), nullptr, &PS));

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

    D3D11_SUBRESOURCE_DATA indexInitData{};

    indexInitData.pSysMem = indices;
    ExitIfFailed(D3d11Device->CreateBuffer(&indexBufferDesc, &indexInitData, &CubesIndexBuffer));

    D3D11_BUFFER_DESC vertexBufferDesc;
    ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));

    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.ByteWidth = sizeof(Vertex) * static_cast<UINT>(vertices.size());
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;
    vertexBufferDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA vertexBufferData;

    ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
    vertexBufferData.pSysMem = vertices.data();
    ExitIfFailed(D3d11Device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &CubesVertBuffer));

    //Create the Input Layout
    ExitIfFailed(D3d11Device->CreateInputLayout(layout, numElements, VsBuffer->GetBufferPointer(),
        VsBuffer->GetBufferSize(), &VertLayout));
}

static void InitFontRenderingPipeline()
{
    LPCWSTR shaderPath = L"assets/shaders/font_shaders.hlsl";

    ID3DBlob* errorMessages;

    //Create the Shader Objects
    HRESULT hr = D3DCompileFromFile(
	    shaderPath,
	    nullptr,
	    nullptr,
	    "VSMain",
	    "vs_5_0",
	    0,
	    0,
	    &FontVsBuffer,
	    &errorMessages);

    VerifyShader(hr, errorMessages);

    hr = D3DCompileFromFile(
	    shaderPath,
	    nullptr,
	    nullptr,
	    "PSMain",
	    "ps_5_0",
	    0,
	    0,
	    &FontPsBuffer,
	    &errorMessages);

    VerifyShader(hr, errorMessages);

    ExitIfFailed(D3d11Device->CreateVertexShader(FontVsBuffer->GetBufferPointer(), FontVsBuffer->GetBufferSize(), nullptr, &FontVS));
    ExitIfFailed(D3d11Device->CreatePixelShader(FontPsBuffer->GetBufferPointer(), FontPsBuffer->GetBufferSize(), nullptr, &FontPS));

    //Create the vertex buffer (cube)
    std::vector<float> vertices =
    {
	    0.0f, 1.0f, 0.0f, 0.0f,
	    1.0f, 1.0f, 1.0f, 0.0f,
	    1.0f, 0.0f, 1.0f, 1.0f,
	    0.0f, 0.0f, 0.0f, 1.0f
    };

    uint32_t indices[] =
    {
        0, 1, 2,
		0, 2, 3
    };

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = ARRAYSIZE(layout);

    D3D11_BUFFER_DESC indexBufferDesc;
    ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));

    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.ByteWidth = sizeof(indices);
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = 0;
    indexBufferDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA indexInitData;

    indexInitData.pSysMem = indices;
    ExitIfFailed(D3d11Device->CreateBuffer(&indexBufferDesc, &indexInitData, &QuadIndexBuffer));

    D3D11_BUFFER_DESC vertexBufferDesc;
    ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));

    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.ByteWidth = static_cast<UINT>(sizeof(float) * (vertices.size()));
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;
    vertexBufferDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA vertexBufferData;

    ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
    vertexBufferData.pSysMem = vertices.data();
    ExitIfFailed(D3d11Device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &QuadVertBuffer));

    //Create the Input Layout
    ExitIfFailed(D3d11Device->CreateInputLayout(layout, numElements, FontVsBuffer->GetBufferPointer(),
        FontVsBuffer->GetBufferSize(), &FontVertLayout));
}

void Init()
{
    //////////////////////////////////
    // Init D3D11                   //
    //////////////////////////////////

    UINT createDeviceFlags = 0;
#if defined(_DEBUG)
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    ExitIfFailed(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, nullptr, NULL,
        D3D11_SDK_VERSION, &D3d11Device, nullptr, &D3d11DeviceContext));

    ComPtr<IDXGIFactory2> factory2;
    CreateDXGIFactory1(IID_PPV_ARGS(&factory2));

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC1));

    swapChainDesc.Width = GameResolutionWidth;
    swapChainDesc.Height = GameResolutionHeight;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

	void* window = PlatformGetWindowHandle();
	HWND hwnd = static_cast<HWND>(window);

	assert(hwnd && "HWND is null!");
    
    factory2->CreateSwapChainForHwnd(D3d11Device.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &SwapChain);

    //Create our BackBuffer
    ID3D11Texture2D* backBuffer = nullptr;
    ExitIfFailed(SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer));

    //Create our Render Target
    ExitIfFailed(D3d11Device->CreateRenderTargetView(backBuffer, nullptr, &RenderTargetView));
    backBuffer->Release();

    //Describe our Depth/Stencil Buffer
    D3D11_TEXTURE2D_DESC depthStencilDesc = {};

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
    ExitIfFailed(D3d11Device->CreateTexture2D(&depthStencilDesc, nullptr, &DepthStencilBuffer));
    ExitIfFailed(D3d11Device->CreateDepthStencilView(DepthStencilBuffer, nullptr, &DepthStencilView));

    //Set our Render Target and Depth/Stencil Views
    D3d11DeviceContext->OMSetRenderTargets(1, &RenderTargetView, DepthStencilView);

    //Create the Viewport
    D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = static_cast<FLOAT>(GameResolutionWidth);
    viewport.Height = static_cast<FLOAT>(GameResolutionHeight);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    //Set the Viewport
    D3d11DeviceContext->RSSetViewports(1, &viewport);

    InitMainRenderingPipeline();
	InitFontRenderingPipeline();

    //Create the buffer to send to the constant buffer in effect file
    D3D11_BUFFER_DESC constantBufferDesc;
    ZeroMemory(&constantBufferDesc, sizeof(D3D11_BUFFER_DESC));

    constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    constantBufferDesc.ByteWidth = sizeof(CbPerObject);
    constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantBufferDesc.CPUAccessFlags = 0;
    constantBufferDesc.MiscFlags = 0;

    ExitIfFailed(D3d11Device->CreateBuffer(&constantBufferDesc, nullptr, &CbPerObjectBuffer));

	// Texture Loading Test
    Texture tex = LoadTexture("assets/textures/cat.jpg");
	CubesTextureView = CreateTextureView(tex);

    //stbi_image_free(tex.Pixels.data());

    D3D11_SAMPLER_DESC samplerDesc;
    ZeroMemory(&samplerDesc, sizeof(samplerDesc));
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    ExitIfFailed(D3d11Device->CreateSamplerState(&samplerDesc, &CubesTexSamplerState));

    // Rasterizer States 

    //Define the Blending Equation
    D3D11_BLEND_DESC blendDesc;
    ZeroMemory(&blendDesc, sizeof(blendDesc));

    D3D11_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc;
    ZeroMemory(&renderTargetBlendDesc, sizeof(renderTargetBlendDesc));

    renderTargetBlendDesc.BlendEnable = true;
    renderTargetBlendDesc.SrcBlend = D3D11_BLEND_SRC_COLOR;
    renderTargetBlendDesc.DestBlend = D3D11_BLEND_BLEND_FACTOR;
    renderTargetBlendDesc.BlendOp = D3D11_BLEND_OP_ADD;
    renderTargetBlendDesc.SrcBlendAlpha = D3D11_BLEND_ONE;
    renderTargetBlendDesc.DestBlendAlpha = D3D11_BLEND_ZERO;
    renderTargetBlendDesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    renderTargetBlendDesc.RenderTargetWriteMask = D3D10_COLOR_WRITE_ENABLE_ALL;

    blendDesc.AlphaToCoverageEnable = false;
    blendDesc.RenderTarget[0] = renderTargetBlendDesc;

    ExitIfFailed(D3d11Device->CreateBlendState(&blendDesc, &Transparency));

    D3D11_RASTERIZER_DESC solidDesc;
    ZeroMemory(&solidDesc, sizeof(D3D11_RASTERIZER_DESC));
    solidDesc.FillMode = D3D11_FILL_SOLID;
    solidDesc.CullMode = D3D11_CULL_NONE; // TODO: Change to BACK
    ExitIfFailed(D3d11Device->CreateRasterizerState(&solidDesc, &Solid));

    D3D11_RASTERIZER_DESC wireFrameDesc;
    ZeroMemory(&wireFrameDesc, sizeof(D3D11_RASTERIZER_DESC));
    wireFrameDesc.FillMode = D3D11_FILL_WIREFRAME;
    wireFrameDesc.CullMode = D3D11_CULL_NONE;
    ExitIfFailed(D3d11Device->CreateRasterizerState(&wireFrameDesc, &WireFrame));

    D3D11_RASTERIZER_DESC noCullDesc;
    ZeroMemory(&noCullDesc, sizeof(D3D11_RASTERIZER_DESC));
    noCullDesc.FillMode = D3D11_FILL_SOLID;
    noCullDesc.CullMode = D3D11_CULL_NONE;
    ExitIfFailed(D3d11Device->CreateRasterizerState(&noCullDesc, &NoCull));

    //Create the Counterclockwise and Clockwise Culling States
    D3D11_RASTERIZER_DESC rasterizerDesc;
    ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_BACK;
    rasterizerDesc.FrontCounterClockwise = true;
    ExitIfFailed(D3d11Device->CreateRasterizerState(&rasterizerDesc, &CounterClockwiseCullMode));
    rasterizerDesc.FrontCounterClockwise = false;
    ExitIfFailed(D3d11Device->CreateRasterizerState(&rasterizerDesc, &ClockwiseCullMode));


    ZeroMemory(&constantBufferDesc, sizeof(D3D11_BUFFER_DESC));

    constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    constantBufferDesc.ByteWidth = sizeof(CbPerFrame);
    constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantBufferDesc.CPUAccessFlags = 0;
    constantBufferDesc.MiscFlags = 0;

    ExitIfFailed(D3d11Device->CreateBuffer(&constantBufferDesc, nullptr, &cbPerFrameBuffer));

    //Camera information
    CamPosition = { 0.0f, 3.0f, -8.0f };
    CamTarget = { 0.0f, 0.0f, 0.0f };
    CamUp = { 0.0f, 1.0f, 0.0f };

    //Set the View matrix
    CamView = MatrixLookAt(CamPosition, CamTarget, CamUp);

    //Set the Projection matrix
    CamProjection = MatrixPerspective(
        0.4f * 3.14f, static_cast<float>(GameResolutionWidth) / GameResolutionHeight, 1.0f, 1000.0f);

    LoadedFontGlyphs = std::move(LoadFontGlyphs("C:/Windows/Fonts/calibri.ttf"));

	GlobalDirectionalLight.Direction = {.X = -0.25f, .Y = -0.5f, .Z = -1.0f };
	GlobalDirectionalLight.Ambient = {.X = 0.15f, .Y = 0.15f, .Z = 0.15f };
	GlobalDirectionalLight.Diffuse = {.X = 0.8f, .Y = 0.8f, .Z = 0.8f };

    ConstBufferPerFrame.Light = GlobalDirectionalLight;

    D3d11DeviceContext->UpdateSubresource(cbPerFrameBuffer, 0, NULL, &ConstBufferPerFrame, 0, 0);
    D3d11DeviceContext->PSSetConstantBuffers(0, 1, &cbPerFrameBuffer);
}

void UpdateGame(const float dt)
{
    //Keep the cubes rotating
    Rot += .5f * dt;
    if (Rot > 6.28f)
        Rot = 0.0f;

    GlobalDirectionalLight.Ambient = { 0.4f, 0.4f, 0.4f };
    GlobalDirectionalLight.Color = { 1.0f, 1.0f, 1.0f };
	V3 lightDir = Normalize({ 0.5f, 1.0f, 0.5f });
    GlobalDirectionalLight.Direction = { lightDir.X, lightDir.Y, lightDir.Z, 0.0f };

    //Reset cube1World
    Cube1World = MatrixIdentity();

    Rotation = MatrixRotationY(Rot);
    Translation = MatrixTranslation(0.0f, 0.0f, 4.0f);

    //Set cube1's world space using the transformations
    Cube1World = Translation * Rotation;

    //Reset cube2World
    Cube2World = MatrixIdentity();

    //Define cube2's world space matrix
    Rotation = MatrixRotationY(Rot);
    Scale = MatrixScaling(1.3f, 1.3f, 1.3f);

    //Set cube2's world space matrix
    Cube2World = Rotation * Scale;
}

void RenderScene()
{
    //Clear our back buffer (sky blue)
    constexpr float bgColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    D3d11DeviceContext->ClearRenderTargetView(RenderTargetView, bgColor);

    //Set Initial Vertex and Pixel Shaders
    D3d11DeviceContext->VSSetShader(VS, nullptr, 0);
    D3d11DeviceContext->PSSetShader(PS, nullptr, 0);

    //Set the vertex buffer
    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    D3d11DeviceContext->IASetIndexBuffer(CubesIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    D3d11DeviceContext->IASetVertexBuffers(0, 1, &CubesVertBuffer, &stride, &offset);
    D3d11DeviceContext->IASetInputLayout(VertLayout);
    D3d11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    //Refresh the Depth/Stencil view
    D3d11DeviceContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    //Enable the Default Rasterizer State
    D3d11DeviceContext->RSSetState(nullptr);
    //Draw objects that will use backface culling

    //Turn off backface culling
    D3d11DeviceContext->RSSetState(NoCull);

    ConstBufferPerFrame.Light = GlobalDirectionalLight;
    D3d11DeviceContext->UpdateSubresource(cbPerFrameBuffer, 0, NULL, &ConstBufferPerFrame, 0, 0);
    D3d11DeviceContext->PSSetConstantBuffers(0, 1, &cbPerFrameBuffer);

    CbPerObj = {};

    CbPerObj.Projection = CamProjection;
    CbPerObj.View = CamView;
    CbPerObj.World = Cube1World;

    D3d11DeviceContext->UpdateSubresource(CbPerObjectBuffer, 0, nullptr, &CbPerObj, 0, 0);
    D3d11DeviceContext->VSSetConstantBuffers(0, 1, &CbPerObjectBuffer);
    D3d11DeviceContext->PSSetShaderResources(0, 1, &CubesTextureView);
    D3d11DeviceContext->PSSetSamplers(0, 1, &CubesTexSamplerState);

    //Draw the first cube
    D3d11DeviceContext->DrawIndexed(36, 0, 0);

    CbPerObj.Projection = CamProjection;
    CbPerObj.View = CamView;
    CbPerObj.World = Cube2World;

    D3d11DeviceContext->UpdateSubresource(CbPerObjectBuffer, 0, nullptr, &CbPerObj, 0, 0);
    D3d11DeviceContext->VSSetConstantBuffers(0, 1, &CbPerObjectBuffer);
    D3d11DeviceContext->PSSetShaderResources(0, 1, &CubesTextureView);
    D3d11DeviceContext->PSSetSamplers(0, 1, &CubesTexSamplerState);

    //Draw the second cube
    D3d11DeviceContext->DrawIndexed(36, 0, 0);
}

void RenderText(const std::string_view text, 
	float x, float y, const float scale, const V3& color)
{
    // Switch to font rendering pipeline
    D3d11DeviceContext->VSSetShader(FontVS, nullptr, 0);
    D3d11DeviceContext->PSSetShader(FontPS, nullptr, 0);

    //Set the vertex buffer
    constexpr UINT stride = sizeof(float) * 4;
    constexpr UINT offset = 0;
    D3d11DeviceContext->IASetVertexBuffers(0, 1, &QuadVertBuffer, &stride, &offset);
    D3d11DeviceContext->IASetIndexBuffer(QuadIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    D3d11DeviceContext->IASetInputLayout(FontVertLayout);
    D3d11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3d11DeviceContext->RSSetState(NoCull);

	const M4 projection = MatrixOrthographicTL(
        static_cast<float>(GameResolutionWidth), static_cast<float>(GameResolutionHeight), 0.0f, 1.0f);

    for (char c : text)
    {
        auto it = LoadedFontGlyphs.find(c);
        if (it == LoadedFontGlyphs.end())
        {
            x += 8.f * scale;
	        continue;
        };
        FontGlyph& glyph = it->second;

        const float xPos = x + glyph.Bearing.X * scale;
        const float yPos = y - (glyph.Size.Y - glyph.Bearing.Y) * scale;

        const M4 scaling = MatrixScaling(glyph.Size.X * scale, glyph.Size.Y * scale, 1.0f); // scale unit quad to pixel size
        const M4 translation = MatrixTranslation(xPos, yPos, 0.0f);
        const M4 model = scaling * translation;

        CbPerObj = {};
        CbPerObj.Projection = projection;
        CbPerObj.View = MatrixIdentity();
        CbPerObj.World = model;

        CbPerObj.Color = { color.X, color.Y, color.Z, 1.0f };

        D3d11DeviceContext->UpdateSubresource(CbPerObjectBuffer, 0, nullptr, &CbPerObj, 0, 0);
        D3d11DeviceContext->VSSetConstantBuffers(0, 1, &CbPerObjectBuffer);
        D3d11DeviceContext->PSSetConstantBuffers(0, 1, &CbPerObjectBuffer);

        D3d11DeviceContext->PSSetShaderResources(0, 1, &glyph.TextureView);

        D3d11DeviceContext->DrawIndexed(6, 0, 0);

        x += glyph.Advance * scale;
    }
}

 void PresentSwapChain()
{
    UINT PresentFlags = 0;
	DXGI_PRESENT_PARAMETERS presentParams = {};

#ifdef _DEBUG
    ExitIfFailed(SwapChain->Present1(VSync, PresentFlags, &presentParams));
#elif
    SwapChain->Present1(VSync, PresentFlags, &presentParams);
#endif
}
