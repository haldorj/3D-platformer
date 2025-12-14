#pragma once

void PlatformInitWindow(int windowWidth, int windowHeight, 
						const wchar_t* title);
void PlatformUpdateWindow(bool& running);
void* PlatformGetWindowHandle();

// Platform agnostic settings

static constexpr int WindowWidth = 1280;
static constexpr int WindowHeight = 720;

static constexpr int GameResolutionWidth = 640;
static constexpr int GameResolutionHeight = 360;

static bool VSync = true;

static int FPS = 0;
