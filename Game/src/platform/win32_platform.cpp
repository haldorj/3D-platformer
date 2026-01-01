#include "pch.h"
#include "platform/win32_platform.h"

#ifdef _WIN32

static inline HWND Hwnd;
static inline HINSTANCE hInstance;

static std::unordered_map<KeyCode, int> KeyMap;

static InputState _InputState{};

static POINT _MousePosition{};

static int TranslateModifierKey(WPARAM wParam, LPARAM lParam)
{
	// Distinguish between left and right modifier keys
    switch (wParam)
    {
    case VK_SHIFT:
    {
        UINT sc = (lParam & 0x00ff0000) >> 16;
        UINT vk = MapVirtualKey(sc, MAPVK_VSC_TO_VK_EX);
        return (int)vk;
    }

    case VK_CONTROL:
        return (lParam & 0x01000000) ? VK_RCONTROL : VK_LCONTROL;

    case VK_MENU:
        return (lParam & 0x01000000) ? VK_RMENU : VK_LMENU;
    }

    return (int)wParam;
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        if (!(lParam & 0x40000000))
        {
            int vk = TranslateModifierKey(wParam, lParam);
            _InputState.KeyStates.KeysDown[vk] = true;
        }

        // Eat Alt/F10 so Windows doesn't pause
        if (wParam == VK_MENU || wParam == VK_F10)
            return 0;
    }
    break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
        int vk = TranslateModifierKey(wParam, lParam);
        _InputState.KeyStates.KeysDown[vk] = false;

        if (wParam == VK_MENU || wParam == VK_F10)
            return 0;
    }
    break;

    case WM_MOUSEMOVE:
    {
        _MousePosition.x = GET_X_LPARAM(lParam);
        _MousePosition.y = GET_Y_LPARAM(lParam);

        break;
    }

    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    }

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void Win32Platform::PlatformInitWindow(int windowWidth, int windowHeight, const wchar_t* title)
{
    // Register the window class.
    const wchar_t CLASS_NAME[] = L"Window Class";

    WNDCLASS wc = { };

    hInstance = GetModuleHandle(NULL);
    int nCmdShow = SW_SHOW;

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // Adjust the window rectangle to include window decorations
    RECT rect = { 0, 0, windowWidth, windowHeight };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    int adjustedWidth = rect.right - rect.left;
    int adjustedHeight = rect.bottom - rect.top;

    // Create the window.

    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    //DWORD style = WS_OVERLAPPEDWINDOW;

    auto windowText = title;

    Hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        windowText,
        style,

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, adjustedWidth, adjustedHeight,

        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );

    assert(Hwnd);

    ShowWindow(Hwnd, nCmdShow);
}

void Win32Platform::PlatformUpdateWindow(bool& Running)
{
    MSG msg = { };
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
            Running = false;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void* Win32Platform::PlatformGetWindowHandle()
{
    return static_cast<void*>(Hwnd);
}

void Win32Platform::PlatformInitInput()
{
    KeyMap[KeyCode::MOUSE_BUTTON_LEFT] = VK_LBUTTON;
    KeyMap[KeyCode::MOUSE_BUTTON_RIGHT] = VK_RBUTTON;
    KeyMap[KeyCode::MOUSE_BUTTON_MIDDLE] = VK_MBUTTON;
    KeyMap[KeyCode::MOUSE_BUTTON_4] = VK_XBUTTON1;
    KeyMap[KeyCode::MOUSE_BUTTON_5] = VK_XBUTTON2;

    KeyMap[KeyCode::KEY_A] = 'A';
    KeyMap[KeyCode::KEY_B] = 'B';
    KeyMap[KeyCode::KEY_C] = 'C';
    KeyMap[KeyCode::KEY_D] = 'D';
    KeyMap[KeyCode::KEY_E] = 'E';
    KeyMap[KeyCode::KEY_F] = 'F';
    KeyMap[KeyCode::KEY_G] = 'G';
    KeyMap[KeyCode::KEY_H] = 'H';
    KeyMap[KeyCode::KEY_I] = 'I';
    KeyMap[KeyCode::KEY_J] = 'J';
    KeyMap[KeyCode::KEY_K] = 'K';
    KeyMap[KeyCode::KEY_L] = 'L';
    KeyMap[KeyCode::KEY_M] = 'M';
    KeyMap[KeyCode::KEY_N] = 'N';
    KeyMap[KeyCode::KEY_O] = 'O';
    KeyMap[KeyCode::KEY_P] = 'P';
    KeyMap[KeyCode::KEY_Q] = 'Q';
    KeyMap[KeyCode::KEY_R] = 'R';
    KeyMap[KeyCode::KEY_S] = 'S';
    KeyMap[KeyCode::KEY_T] = 'T';
    KeyMap[KeyCode::KEY_U] = 'U';
    KeyMap[KeyCode::KEY_V] = 'V';
    KeyMap[KeyCode::KEY_W] = 'W';
    KeyMap[KeyCode::KEY_X] = 'X';
    KeyMap[KeyCode::KEY_Y] = 'Y';
    KeyMap[KeyCode::KEY_Z] = 'Z';

    KeyMap[KeyCode::KEY_0] = '0';
    KeyMap[KeyCode::KEY_1] = '1';
    KeyMap[KeyCode::KEY_2] = '2';
    KeyMap[KeyCode::KEY_3] = '3';
    KeyMap[KeyCode::KEY_4] = '4';
    KeyMap[KeyCode::KEY_5] = '5';
    KeyMap[KeyCode::KEY_6] = '6';
    KeyMap[KeyCode::KEY_7] = '7';
    KeyMap[KeyCode::KEY_8] = '8';
    KeyMap[KeyCode::KEY_9] = '9';

    KeyMap[KeyCode::KEY_F1] = VK_F1;
    KeyMap[KeyCode::KEY_F2] = VK_F2;
    KeyMap[KeyCode::KEY_F3] = VK_F3;
    KeyMap[KeyCode::KEY_F4] = VK_F4;
    KeyMap[KeyCode::KEY_F5] = VK_F5;
    KeyMap[KeyCode::KEY_F6] = VK_F6;
    KeyMap[KeyCode::KEY_F7] = VK_F7;
    KeyMap[KeyCode::KEY_F8] = VK_F8;
    KeyMap[KeyCode::KEY_F9] = VK_F9;
    KeyMap[KeyCode::KEY_F10] = VK_F10;

    KeyMap[KeyCode::KEY_LEFT] = VK_LEFT;
    KeyMap[KeyCode::KEY_RIGHT] = VK_RIGHT;
    KeyMap[KeyCode::KEY_UP] = VK_UP;
    KeyMap[KeyCode::KEY_DOWN] = VK_DOWN;

    KeyMap[KeyCode::KEY_SPACE] = VK_SPACE;
    KeyMap[KeyCode::KEY_ESCAPE] = VK_ESCAPE;
    KeyMap[KeyCode::KEY_ENTER] = VK_RETURN;
    KeyMap[KeyCode::KEY_TAB] = VK_TAB;
    KeyMap[KeyCode::KEY_BACKSPACE] = VK_BACK;
    KeyMap[KeyCode::KEY_CAPS_LOCK] = VK_CAPITAL;

    KeyMap[KeyCode::KEY_LEFT_SHIFT] = VK_LSHIFT;
    KeyMap[KeyCode::KEY_RIGHT_SHIFT] = VK_RSHIFT;
    KeyMap[KeyCode::KEY_LEFT_CTRL] = VK_LCONTROL;
    KeyMap[KeyCode::KEY_RIGHT_CTRL] = VK_RCONTROL;
    KeyMap[KeyCode::KEY_LEFT_ALT] = VK_LMENU;
    KeyMap[KeyCode::KEY_RIGHT_ALT] = VK_RMENU;

}

void Win32Platform::PlatformUpdateInput()
{
    auto& s = _InputState.KeyStates;
    for (int i = 0; i < KEY_COUNT; ++i) 
    {
        s.KeysPressed[i] = !s.PrevKeysDown[i] && s.KeysDown[i];
        s.KeysReleased[i] = s.PrevKeysDown[i] && !s.KeysDown[i];
        s.PrevKeysDown[i] = s.KeysDown[i];
    }
}

bool Win32Platform::IsKeyDown(KeyCode key)
{
	if (_InputState.KeyStates.KeysDown.size() <= static_cast<size_t>(key))
		return false;

	return _InputState.KeyStates.KeysDown[KeyMap[key]];
}

bool Win32Platform::IsKeyPressed(KeyCode key)
{
	if (_InputState.KeyStates.KeysPressed.size() <= static_cast<size_t>(key))
		return false;

	return _InputState.KeyStates.KeysPressed[KeyMap[key]];
}

bool Win32Platform::IsKeyReleased(KeyCode key)
{
	if (_InputState.KeyStates.KeysReleased.size() <= static_cast<size_t>(key))
		return false;

	return _InputState.KeyStates.KeysReleased[KeyMap[key]];
}

V2 Win32Platform::GetMousePosition()
{
	return V2{ static_cast<float>(_MousePosition.x), static_cast<float>(_MousePosition.y) };
}

#endif // _WIN32