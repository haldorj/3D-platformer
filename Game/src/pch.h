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

//////////////////////////////////////////
// Third party includes					//
//////////////////////////////////////////

#include <impl.h>
#include <tiny_gltf.h>
#include <stb_truetype.h>

//////////////////////////////////////////
// Platform includes					//
//////////////////////////////////////////

#ifdef _WIN32
// Windows specific includes
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