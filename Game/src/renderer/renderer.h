#pragma once

#include "game.h"

class Platform;

class Renderer
{
public:
	virtual ~Renderer() = default;

	virtual void* CreateTextureView(const Texture& texture) = 0;

	virtual void InitRenderer(int gameHeight, int gameWidth,
		Platform* platform, GameMemory* gameState) = 0;

	virtual void UploadMeshesToGPU(Mesh& mesh) = 0;

	virtual void RenderScene(GameMemory* gameState) = 0;

	// virtual void RenderPoint(const V3& position, const float scale);

	virtual void RenderText(std::unordered_map<char, FontGlyph>& glyphs,
		int w, int h, const std::string_view text, float x, float y,
		const float scale, const V3& color) = 0;

	virtual void PresentSwapChain(bool& vSync) = 0;
};
