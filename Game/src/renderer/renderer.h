#pragma once

#include "application.h"

class Application;
class Platform;
struct GameState;
struct Texture;
struct V3;

class Renderer
{
public:
	virtual void* CreateTextureView(const Texture& texture) = 0;

	virtual void InitRenderer(int gameHeight, int gameWidth,
		Platform& platform, GameState& gameState, Application& app) = 0;

	virtual void InitMainRenderingPipeline() = 0;

	virtual void InitFontRenderingPipeline() = 0;

	virtual void RenderScene(GameState& gameState) = 0;

	virtual void RenderText(GameState& gameState, int w, int h, 
		const std::string_view text, float x, float y, 
		const float scale, const V3& color) = 0;

	virtual void PresentSwapChain(bool& vSync) = 0;
};
