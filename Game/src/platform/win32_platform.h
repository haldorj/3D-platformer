#pragma once

#include "platform.h"

class Win32Platform final : public Platform
{
public:
	void PlatformInitWindow(int windowWidth, int windowHeight,
		const wchar_t* title) override;
	void PlatformUpdateWindow(bool& running) override;
	void* PlatformGetWindowHandle() override;
};