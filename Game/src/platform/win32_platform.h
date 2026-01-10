#pragma once

#include "platform.h"

class Win32Platform final : public Platform
{
public:
	void InitWindow(int windowWidth, int windowHeight,
		const wchar_t* title) override;
	void UpdateWindow(bool& running) override;
	void* GetWindowHandle() override;

	void InitConsole() override;
	void Shutdown() override;

	void InitInput() override;
	void UpdateInput() override;

	bool IsKeyDown(KeyCode key) override;
	bool IsKeyPressed(KeyCode key) override;
	bool IsKeyReleased(KeyCode key) override;

	V2 GetMousePosition() override;
	V2 GetMouseDelta() override;
	void SetMouseDelta(const V2& delta) override;
	
	void SetCursorVisible(const bool show) override;
	void ConfineCursorToWindow(const bool confine) override;

	void InitAudio() override;
};