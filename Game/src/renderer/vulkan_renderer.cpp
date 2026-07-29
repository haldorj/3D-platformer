#include "pch.h"
#include "vulkan_renderer.h"
#include <vulkan/vulkan.h>

void* VulkanRenderer::CreateTextureView(const Texture& texture)
{
    return {};
}

void VulkanRenderer::InitRenderer(int gameHeight, int gameWidth, Platform* platform, GameMemory* gameState)
{
    VkApplicationInfo appInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "How to Vulkan",
        .apiVersion = VK_API_VERSION_1_3
    };
}

void VulkanRenderer::UploadMeshesToGPU(Mesh& mesh)
{
}

void VulkanRenderer::RenderScene(GameMemory* gameState)
{
}

void VulkanRenderer::RenderDebugPrimitives(GameMemory* gameMemory, DebugPrimitives& primitives)
{
}

void VulkanRenderer::RenderText(std::unordered_map<char, FontGlyph>& glyphs, int w, int h, const std::string_view text,
    float x, float y, const float scale, const V3& color)
{
}

void VulkanRenderer::PresentSwapChain(bool& vSync)
{
}
