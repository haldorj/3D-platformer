#pragma once

#include "SDL3/SDL.h"

#include <fstream>
#include <string>
#include <iostream>
#include <cstdint>
#include <deque>
#include <functional>
#include <ranges>
#include <print>
#include <math.h>

#ifdef __linux__ 

#elif _WIN32
#include <windows.h>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <comdef.h>
#endif
