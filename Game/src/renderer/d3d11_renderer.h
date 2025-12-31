#pragma once

#include "renderer.h"

using Microsoft::WRL::ComPtr;

struct CbPerObject
{
    M4 Projection{};
    M4 View{};
    M4 World{};
    V4 Color{};
};

struct CbPerFrame
{
    DirectionalLight Light{};
};

class D3D11Renderer final : public Renderer
{
public:
    void InitRenderer(int gameHeight, int gameWidth, 
        Platform& platform, GameState& gameState) override;

	void InitMainRenderingPipeline() override;
	void InitFontRenderingPipeline() override;
	void* CreateTextureView(const Texture& texture) override;

	void RenderScene(GameState& gameState) override;
	void RenderText(GameState& gameState, int w, int h,
        const std::string_view text, float x, float y, 
        const float scale, const V3& color) override;

	void PresentSwapChain(bool& vSync) override;

private:
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
};