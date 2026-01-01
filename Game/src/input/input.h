#pragma once

#include "key_codes.h"

struct KeyStates
{
	std::array<bool, KEY_COUNT> KeysDown{};
	std::array<bool, KEY_COUNT> KeysPressed{};
	std::array<bool, KEY_COUNT> KeysReleased{};
	std::array<bool, KEY_COUNT> PrevKeysDown{};
};

struct InputState
{
	KeyStates KeyStates{};
	V2 MousePosition{};
};