#pragma once

#include "platform.h"

class Win32Platform final : public Platform
{
public:
	void PlatformInitWindow(int windowWidth, int windowHeight,
		const wchar_t* title) override;
	void PlatformUpdateWindow(bool& running) override;
	void* PlatformGetWindowHandle() override;

	void PlatformInitInput() override;
	void PlatformUpdateInput() override;

	bool IsKeyDown(KeyCode key) override;
	bool IsKeyPressed(KeyCode key) override;
	bool IsKeyReleased(KeyCode key) override;

	V2 GetMousePosition() override;
	V2 GetMouseDelta() override;
	
	void PlatformShowCursor(const bool show) override;
	void PlatformConfineCursorToWindow(const bool confine) override;
};