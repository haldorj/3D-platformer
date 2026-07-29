#pragma once

#include "renderer.h"
class VulkanRenderer : Renderer
{
public:
    void* CreateTextureView(const Texture& texture) override;
    void InitRenderer(int gameHeight, int gameWidth, Platform* platform, GameMemory* gameState) override;
    void UploadMeshesToGPU(Mesh& mesh) override;
    void RenderScene(GameMemory* gameState) override;
    void RenderDebugPrimitives(GameMemory* gameMemory, DebugPrimitives& primitives) override;
    void RenderText(std::unordered_map<char, FontGlyph>& glyphs, int w, int h, const std::string_view text, float x,
        float y, const float scale, const V3& color) override;
    void PresentSwapChain(bool& vSync) override;
    
private:
    
};

