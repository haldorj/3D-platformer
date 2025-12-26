#pragma once

class Platform
{
public:
	virtual void PlatformInitWindow(int windowWidth, int windowHeight,
		const wchar_t* title) = 0;
	virtual void PlatformUpdateWindow(bool& running) = 0;
	virtual void* PlatformGetWindowHandle() = 0;
};
