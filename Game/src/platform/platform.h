#pragma once
#include <math/handmade_math.h>
#include <input/input.h>
#include <assets/sound.h>

// TODO: Consider splitting platform layer into smaller subsystems.
class Platform
{
public:
	virtual ~Platform() = default;

	// Window management
	virtual void InitWindow(int windowWidth, int windowHeight,
		const wchar_t* title) = 0;
	virtual void UpdateWindow(bool& running) = 0;
	virtual void* GetWindowHandle() = 0;

	// Console management
	virtual void InitConsole() = 0;
	virtual void Shutdown() = 0;

	// Input management
	virtual void InitInput() = 0;
	virtual void UpdateInput() = 0;

	virtual bool IsKeyDown(KeyCode key) = 0;
	virtual bool IsKeyPressed(KeyCode key) = 0;
	virtual bool IsKeyReleased(KeyCode key) = 0;

	virtual V2 GetMousePosition() = 0;
	virtual V2 GetMouseDelta() = 0;
	virtual void SetMouseDelta(const V2& delta) = 0;

	virtual void SetCursorVisible(const bool show) = 0;
	virtual void ConfineCursorToWindow(const bool confine) = 0;

	// Audio management
	virtual void InitAudio() = 0;
	virtual void PlayAudio(Sound& sound, float volume) = 0;

	// Memory management
	virtual void* AllocateMemory(size_t capacity) = 0;
	virtual void FreeMemory(void* memory) = 0;
};
