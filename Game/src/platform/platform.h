#pragma once
#include <math/handmade_math.h>
#include <input/input.h>

class Platform
{
public:
	virtual ~Platform() = default;

	// Window management
	virtual void PlatformInitWindow(int windowWidth, int windowHeight,
		const wchar_t* title) = 0;
	virtual void PlatformUpdateWindow(bool& running) = 0;
	virtual void* PlatformGetWindowHandle() = 0;

	// Input management
	virtual void PlatformInitInput() = 0;
	virtual void PlatformUpdateInput() = 0;

	virtual bool IsKeyDown(KeyCode key) = 0;
	virtual bool IsKeyPressed(KeyCode key) = 0;
	virtual bool IsKeyReleased(KeyCode key) = 0;

	virtual V2 GetMousePosition() = 0;
	virtual V2 GetMouseDelta() = 0;
	virtual void SetMouseDelta(const V2& delta) = 0;

	virtual void PlatformShowCursor(const bool show) = 0;
	virtual void PlatformConfineCursorToWindow(const bool confine) = 0;
};
