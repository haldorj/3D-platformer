#pragma once

#ifdef _DEBUG
#include <assert.h>
#define Assert(expr) assert(expr)
#else
#define Assert(expr) ((void)0)
#endif

#ifndef UNICODE
#define UNICODE
#endif

#define Kilobytes(number) ((number)*1024ull)
#define Megabytes(number) (Kilobytes(number) * 1024ull)
#define Gigabytes(number) (Megabytes(number) * 1024ull)
#define Terabytes(number) (Gigabytes(number) * 1024ull)

//////////////////////////////////////////
// C / C++ Standard library includes	//
//////////////////////////////////////////

#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <print>
#include <format>
#include <map>
#include <unordered_map>
#include <chrono>
#include <memory>
#include <algorithm>
#include <span>
#include <functional>

//////////////////////////////////////////
// Third party includes					//
//////////////////////////////////////////

#include <impl.h>
#include <stb_truetype.h>

//////////////////////////////////////////
// Platform includes					//
//////////////////////////////////////////

#ifdef _WIN32
// Windows specific includes
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>

#include <dxgi1_6.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <comdef.h>
#include <wrl/client.h>

#include <xaudio2.h>
#include <mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>
#endif