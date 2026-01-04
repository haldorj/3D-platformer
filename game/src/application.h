#pragma once

#include <game.h>
#include <platform/platform.h>
#include <renderer/renderer.h>

class Application
{
public:
	void Init();
	void Run();
private:
	void Move(float dt);
	void InitGame(int gameResolutionWidth, int gameResolutionHeight);
	void UploadMeshesToGPU();
	void UpdateGame(const float dt);
	void UpdateCamera(const float dt);

	[[nodiscard]] std::string ReadEntireFile(const std::string& path);
	[[nodiscard]] Texture CreateErrorTexture();
	[[nodiscard]] Texture LoadTextureFromFile(const std::string& path);
	[[nodiscard]] std::unordered_map<char, FontGlyph> LoadFontGlyphs(const std::string& path, Renderer* renderer);

private:
	GameState _GameState;

	std::unique_ptr<class Platform> _Platform;
	std::unique_ptr<class Renderer> _Renderer;

	uint32_t _WindowWidth = 1280;
	uint32_t _WindowHeight = 720;

	uint32_t _GameResolutionWidth = 1280;
	uint32_t _GameResolutionHeight = 720;

	bool _Running{};

	int _FPS{};
	bool _VSync{ true };
	bool _EditMode{};
	bool _ShowCursor{ true };
};