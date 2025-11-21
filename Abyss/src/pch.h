#pragma once

#include "SDL3/SDL.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include <print>
#include <format>
#include <map>
#include <chrono>

#ifdef _WIN32
// Windows specific includes
#include <windows.h>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <comdef.h>
#endif