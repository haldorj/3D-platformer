#pragma once

#include "platform.h"

class Win32Platform final : public Platform
{
public:
	virtual void PlatformInitWindow(int windowWidth, int windowHeight,
		const wchar_t* title) override;
	virtual void PlatformUpdateWindow(bool& running) override;
	virtual void* PlatformGetWindowHandle() override;
};