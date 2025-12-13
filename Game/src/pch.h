#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include <print>
#include <format>
#include <map>
#include <chrono>

#ifndef UNICODE
#define UNICODE
#endif

#ifdef _WIN32
// Windows specific includes
#include <windows.h>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <comdef.h>
#endif