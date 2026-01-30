#include "pch.h"

#include "renderer/d3d11_renderer.h"
#include "platform/platform.h"
#include "game.h"

static void ExitIfFailed(const HRESULT hr)
{
    if (FAILED(hr))
    {
        const _com_error err(hr);
        const LPCTSTR errMsg = err.ErrorMessage();
        char buffer[265];
        WideCharToMultiByte(CP_ACP, 0, errMsg, -1, buffer, sizeof(buffer), nullptr, nullptr);

        std::print("HRESULT failed with error: {}", buffer);
        Assert(false);
    }
}

static void VerifyShader(const HRESULT hr, ID3D10Blob* errorMessages)
{
    if (FAILED(hr) && errorMessages)
    {
        auto errorMsg = static_cast<const char*>(errorMessages->GetBufferPointer());
        std::print("Shader Compilation Error: {}\n", errorMsg);
        Assert(false);
    }
}

void D3D11Renderer::UploadMeshesToGPU(Mesh& mesh)
{
    // Create Vertex Buffer
    D3D11_BUFFER_DESC vertexBufferDesc = {};
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * mesh.Vertices.size());
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA vertexData = {};
    vertexData.pSysMem = mesh.Vertices.data();
    ID3D11Buffer* vertexBuffer = nullptr;
    ExitIfFailed(D3d11Device->CreateBuffer(&vertexBufferDesc, &vertexData, &vertexBuffer));

    mesh.VertexBuffer = vertexBuffer;

    // Create Index Buffer
    D3D11_BUFFER_DESC indexBufferDesc = {};
    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * mesh.Indices.size());
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA indexData = {};
    indexData.pSysMem = mesh.Indices.data();
    ID3D11Buffer* indexBuffer = nullptr;
    ExitIfFailed(D3d11Device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer));

	mesh.IndexBuffer = indexBuffer;

    for (auto& texture : mesh.Textures)
    {
        void* textureView = CreateTextureView(texture);
		mesh.TextureViews.emplace_back(textureView);
	}
}

void* D3D11Renderer::CreateTextureView(const Texture& texture)
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

void D3D11Renderer::InitMainRenderingPipeline()
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

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        //{ "BONEIDS", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        //{ "WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = ARRAYSIZE(layout);

    //Create the Input Layout
    ExitIfFailed(D3d11Device->CreateInputLayout(layout, numElements, VsBuffer->GetBufferPointer(),
        VsBuffer->GetBufferSize(), &VertLayout));
}

void D3D11Renderer::InitFontRenderingPipeline()
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

void D3D11Renderer::InitRenderer(int gameHeight, int gameWidth, Platform& platform, GameMemory* gameState)
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

    swapChainDesc.Width = gameWidth;
    swapChainDesc.Height = gameHeight;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    void* window = platform.GetWindowHandle();
    HWND hwnd = static_cast<HWND>(window);

    Assert(hwnd && "HWND is null!");

    factory2->CreateSwapChainForHwnd(D3d11Device.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &SwapChain);

    //Create our BackBuffer
    ID3D11Texture2D* backBuffer = nullptr;
    ExitIfFailed(SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer));

    //Create our Render Target
    ExitIfFailed(D3d11Device->CreateRenderTargetView(backBuffer, nullptr, &RenderTargetView));
    backBuffer->Release();

    //Describe our Depth/Stencil Buffer
    D3D11_TEXTURE2D_DESC depthStencilDesc = {};

    depthStencilDesc.Width = gameWidth;
    depthStencilDesc.Height = gameHeight;
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
    viewport.Width = static_cast<FLOAT>(gameWidth);
    viewport.Height = static_cast<FLOAT>(gameHeight);
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

    ConstBufferPerFrame.Light = gameState->World.DirectionalLight;

    D3d11DeviceContext->UpdateSubresource(cbPerFrameBuffer, 0, NULL, &ConstBufferPerFrame, 0, 0);
    D3d11DeviceContext->PSSetConstantBuffers(0, 1, &cbPerFrameBuffer);
}

void D3D11Renderer::RenderScene(GameMemory* gameState)
{
    //Clear our back buffer (sky blue)
    constexpr float bgColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    D3d11DeviceContext->ClearRenderTargetView(RenderTargetView, bgColor);

    //Refresh the Depth/Stencil view
    D3d11DeviceContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    //Set Initial Vertex and Pixel Shaders
    D3d11DeviceContext->VSSetShader(VS, nullptr, 0);
    D3d11DeviceContext->PSSetShader(PS, nullptr, 0);

    //Set the vertex buffer
    UINT stride = sizeof(Vertex);
    UINT offset = 0;

	for (auto& entity : gameState->World.Entities)
    {
        for (auto& mesh : entity.Model.Meshes)
        {
            D3d11DeviceContext->IASetIndexBuffer(static_cast<ID3D11Buffer*>(mesh.IndexBuffer), DXGI_FORMAT_R32_UINT, 0);
            D3d11DeviceContext->IASetVertexBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&mesh.VertexBuffer), &stride, &offset);
            D3d11DeviceContext->IASetInputLayout(VertLayout);
            D3d11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


            //Enable the Default Rasterizer State
            D3d11DeviceContext->RSSetState(nullptr);
            //Turn off backface culling
            //D3d11DeviceContext->RSSetState(NoCull);

            ConstBufferPerFrame.Light = gameState->World.DirectionalLight;
            D3d11DeviceContext->UpdateSubresource(cbPerFrameBuffer, 0, NULL, &ConstBufferPerFrame, 0, 0);
            D3d11DeviceContext->PSSetConstantBuffers(0, 1, &cbPerFrameBuffer);

            CbPerObj = {};

            CbPerObj.Projection = gameState->MainCamera.Projection;
            CbPerObj.View = gameState->MainCamera.View;
            CbPerObj.World = entity.WorldMatrix;

            CbPerObj.FinalBoneTransforms = entity.Model.Animator.FinalBoneTransforms;

            D3d11DeviceContext->UpdateSubresource(CbPerObjectBuffer, 0, nullptr, &CbPerObj, 0, 0);
            D3d11DeviceContext->VSSetConstantBuffers(0, 1, &CbPerObjectBuffer);
            ID3D11ShaderResourceView* textureView = static_cast<ID3D11ShaderResourceView*>(mesh.TextureViews[0]);
            D3d11DeviceContext->PSSetShaderResources(0, 1, &textureView);
            D3d11DeviceContext->PSSetSamplers(0, 1, &CubesTexSamplerState);
            //Draw the mesh
            D3d11DeviceContext->DrawIndexed(static_cast<UINT>(mesh.Indices.size()), 0, 0);
        }
    }
}

void D3D11Renderer::RenderText(std::unordered_map<char, FontGlyph>& glyphs,
    int w, int h, const std::string_view text, float x, float y,
    const float scale, const V3& color)
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
        static_cast<float>(w), static_cast<float>(h), 0.0f, 1.0f);

    for (char c : text)
    {
        auto it = glyphs.find(c);
        if (it == glyphs.end())
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

        ID3D11ShaderResourceView* view = static_cast<ID3D11ShaderResourceView*>(glyph.TextureView);
        D3d11DeviceContext->PSSetShaderResources(0, 1, &view);

        D3d11DeviceContext->DrawIndexed(6, 0, 0);

        x += glyph.Advance * scale;
    }
}

void D3D11Renderer::PresentSwapChain(bool& vSync)
{
    UINT PresentFlags = 0;
    DXGI_PRESENT_PARAMETERS presentParams = {};

#ifdef _DEBUG
    ExitIfFailed(SwapChain->Present1(vSync, PresentFlags, &presentParams));
#else
    SwapChain->Present1(vSync, PresentFlags, &presentParams);
#endif
    
}