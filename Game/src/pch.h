#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include <print>
#include <format>
#include <map>
#include <unordered_map>
#include <chrono>
#include <memory>

#ifndef UNICODE
#define UNICODE
#endif

#ifdef _WIN32
// Windows specific includes
#include <windows.h>

#include <dxgi1_6.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <comdef.h>
#include <wrl/client.h>
#endif