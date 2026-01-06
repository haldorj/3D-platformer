#include "pch.h"
#include "platform/win32_platform.h"

#ifdef _WIN32

constinit HWND _Hwnd;
constinit HINSTANCE _HInstance;

static std::unordered_map<KeyCode, int> _KeyMap;

constinit Input _Input{};

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
		// Handle keyboard input
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            if (!(lParam & 0x40000000))
            {
                int vk = TranslateModifierKey(wParam, lParam);
                _Input.KeyStates.KeysDown[vk] = true;
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
            _Input.KeyStates.KeysDown[vk] = false;

            if (wParam == VK_MENU || wParam == VK_F10)
                return 0;
        }
        break;

		// Handle mouse input
        /*
        case WM_MOUSEMOVE:
        {
            _Input.MousePosition.X = static_cast<float>(GET_X_LPARAM(lParam));
            _Input.MousePosition.Y = static_cast<float>(GET_Y_LPARAM(lParam));

            break;
        }
        */

        case WM_LBUTTONDOWN:
        {
            _Input.KeyStates.KeysDown[VK_LBUTTON] = true;
            break;
	    }
        case WM_LBUTTONUP:
        {
            _Input.KeyStates.KeysDown[VK_LBUTTON] = false;
            break;
        }
        case WM_RBUTTONDOWN:
        {
            _Input.KeyStates.KeysDown[VK_RBUTTON] = true;
            break;
	    }
        case WM_RBUTTONUP:
        {
            _Input.KeyStates.KeysDown[VK_RBUTTON] = false;
            break;
	    }
        case WM_MBUTTONDOWN:
        {
            _Input.KeyStates.KeysDown[VK_MBUTTON] = true;
		    break;
	    }
        case WM_MBUTTONUP:
        {
		    _Input.KeyStates.KeysDown[VK_MBUTTON] = false;
		    break;
	    }
        case WM_XBUTTONDOWN:
        {
            int button = HIWORD(wParam);
            if (button == XBUTTON1)
                _Input.KeyStates.KeysDown[VK_XBUTTON1] = true;
            else if (button == XBUTTON2)
                _Input.KeyStates.KeysDown[VK_XBUTTON2] = true;
            break;
	    }
        case WM_XBUTTONUP:
        {
            int button = HIWORD(wParam);
            if (button == XBUTTON1)
                _Input.KeyStates.KeysDown[VK_XBUTTON1] = false;
            else if (button == XBUTTON2)
                _Input.KeyStates.KeysDown[VK_XBUTTON2] = false;
		    break;
	    }

        case WM_INPUT:
        {
            UINT dwSize = 0;
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));
            std::vector<BYTE> lpb(dwSize);
            if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb.data(), &dwSize, sizeof(RAWINPUTHEADER)) == dwSize)
            {
                RAWINPUT* raw = (RAWINPUT*)lpb.data();
                if (raw->header.dwType == RIM_TYPEMOUSE)
                {
                    LONG dx = raw->data.mouse.lLastX;
                    LONG dy = raw->data.mouse.lLastY;

					_Input.MouseDelta.X += static_cast<float>(dx);
					_Input.MouseDelta.Y += static_cast<float>(dy);
                }
            }
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

    _HInstance = GetModuleHandle(NULL);
    int nCmdShow = SW_SHOW;

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = _HInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // Adjust the window rectangle to include window decorations
    RECT rect = { 0, 0, windowWidth, windowHeight };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    int adjustedWidth = rect.right - rect.left;
    int adjustedHeight = rect.bottom - rect.top;

    // Create the window.

    //DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    DWORD style = WS_OVERLAPPEDWINDOW;

    auto windowText = title;

    _Hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        windowText,
        style,

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, adjustedWidth, adjustedHeight,

        NULL,       // Parent window    
        NULL,       // Menu
        _HInstance,  // Instance handle
        NULL        // Additional application data
    );

    Assert(_Hwnd);

    ShowWindow(_Hwnd, nCmdShow);
}

void Win32Platform::PlatformUpdateWindow(bool& _Running)
{
    MSG msg = { };
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
            _Running = false;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void* Win32Platform::PlatformGetWindowHandle()
{
    return static_cast<void*>(_Hwnd);
}

void Win32Platform::PlatformInitInput()
{
    RAWINPUTDEVICE rid{};
    rid.usUsagePage = 0x01;
    rid.usUsage = 0x02;
    rid.dwFlags = 0; /*RIDEV_NOLEGACY;*/
    rid.hwndTarget = _Hwnd;
    Assert(RegisterRawInputDevices(&rid, 1, sizeof(rid)));
    SetProcessDPIAware();

    _KeyMap[KeyCode::MOUSE_BUTTON_LEFT] = VK_LBUTTON;
    _KeyMap[KeyCode::MOUSE_BUTTON_RIGHT] = VK_RBUTTON;
    _KeyMap[KeyCode::MOUSE_BUTTON_MIDDLE] = VK_MBUTTON;
    _KeyMap[KeyCode::MOUSE_BUTTON_4] = VK_XBUTTON1;
    _KeyMap[KeyCode::MOUSE_BUTTON_5] = VK_XBUTTON2;

    _KeyMap[KeyCode::KEY_A] = 'A';
    _KeyMap[KeyCode::KEY_B] = 'B';
    _KeyMap[KeyCode::KEY_C] = 'C';
    _KeyMap[KeyCode::KEY_D] = 'D';
    _KeyMap[KeyCode::KEY_E] = 'E';
    _KeyMap[KeyCode::KEY_F] = 'F';
    _KeyMap[KeyCode::KEY_G] = 'G';
    _KeyMap[KeyCode::KEY_H] = 'H';
    _KeyMap[KeyCode::KEY_I] = 'I';
    _KeyMap[KeyCode::KEY_J] = 'J';
    _KeyMap[KeyCode::KEY_K] = 'K';
    _KeyMap[KeyCode::KEY_L] = 'L';
    _KeyMap[KeyCode::KEY_M] = 'M';
    _KeyMap[KeyCode::KEY_N] = 'N';
    _KeyMap[KeyCode::KEY_O] = 'O';
    _KeyMap[KeyCode::KEY_P] = 'P';
    _KeyMap[KeyCode::KEY_Q] = 'Q';
    _KeyMap[KeyCode::KEY_R] = 'R';
    _KeyMap[KeyCode::KEY_S] = 'S';
    _KeyMap[KeyCode::KEY_T] = 'T';
    _KeyMap[KeyCode::KEY_U] = 'U';
    _KeyMap[KeyCode::KEY_V] = 'V';
    _KeyMap[KeyCode::KEY_W] = 'W';
    _KeyMap[KeyCode::KEY_X] = 'X';
    _KeyMap[KeyCode::KEY_Y] = 'Y';
    _KeyMap[KeyCode::KEY_Z] = 'Z';

    _KeyMap[KeyCode::KEY_0] = '0';
    _KeyMap[KeyCode::KEY_1] = '1';
    _KeyMap[KeyCode::KEY_2] = '2';
    _KeyMap[KeyCode::KEY_3] = '3';
    _KeyMap[KeyCode::KEY_4] = '4';
    _KeyMap[KeyCode::KEY_5] = '5';
    _KeyMap[KeyCode::KEY_6] = '6';
    _KeyMap[KeyCode::KEY_7] = '7';
    _KeyMap[KeyCode::KEY_8] = '8';
    _KeyMap[KeyCode::KEY_9] = '9';

    _KeyMap[KeyCode::KEY_F1] = VK_F1;
    _KeyMap[KeyCode::KEY_F2] = VK_F2;
    _KeyMap[KeyCode::KEY_F3] = VK_F3;
    _KeyMap[KeyCode::KEY_F4] = VK_F4;
    _KeyMap[KeyCode::KEY_F5] = VK_F5;
    _KeyMap[KeyCode::KEY_F6] = VK_F6;
    _KeyMap[KeyCode::KEY_F7] = VK_F7;
    _KeyMap[KeyCode::KEY_F8] = VK_F8;
    _KeyMap[KeyCode::KEY_F9] = VK_F9;
    _KeyMap[KeyCode::KEY_F10] = VK_F10;

    _KeyMap[KeyCode::KEY_LEFT] = VK_LEFT;
    _KeyMap[KeyCode::KEY_RIGHT] = VK_RIGHT;
    _KeyMap[KeyCode::KEY_UP] = VK_UP;
    _KeyMap[KeyCode::KEY_DOWN] = VK_DOWN;

    _KeyMap[KeyCode::KEY_SPACE] = VK_SPACE;
    _KeyMap[KeyCode::KEY_ESCAPE] = VK_ESCAPE;
    _KeyMap[KeyCode::KEY_ENTER] = VK_RETURN;
    _KeyMap[KeyCode::KEY_TAB] = VK_TAB;
    _KeyMap[KeyCode::KEY_BACKSPACE] = VK_BACK;
    _KeyMap[KeyCode::KEY_CAPS_LOCK] = VK_CAPITAL;

    _KeyMap[KeyCode::KEY_LEFT_SHIFT] = VK_LSHIFT;
    _KeyMap[KeyCode::KEY_RIGHT_SHIFT] = VK_RSHIFT;
    _KeyMap[KeyCode::KEY_LEFT_CTRL] = VK_LCONTROL;
    _KeyMap[KeyCode::KEY_RIGHT_CTRL] = VK_RCONTROL;
    _KeyMap[KeyCode::KEY_LEFT_ALT] = VK_LMENU;
    _KeyMap[KeyCode::KEY_RIGHT_ALT] = VK_RMENU;

}

void Win32Platform::PlatformUpdateInput()
{
    auto& s = _Input.KeyStates;
    for (int i = 0; i < KEY_COUNT; ++i) 
    {
        s.KeysPressed[i] = !s.PrevKeysDown[i] && s.KeysDown[i];
        s.KeysReleased[i] = s.PrevKeysDown[i] && !s.KeysDown[i];
        s.PrevKeysDown[i] = s.KeysDown[i];
    }
}

bool Win32Platform::IsKeyDown(KeyCode key)
{
    Assert(static_cast<size_t>(key) < _Input.KeyStates.KeysDown.size());
	return _Input.KeyStates.KeysDown[_KeyMap[key]];
}

bool Win32Platform::IsKeyPressed(KeyCode key)
{
    Assert(static_cast<size_t>(key) < _Input.KeyStates.KeysDown.size());
	return _Input.KeyStates.KeysPressed[_KeyMap[key]];
}

bool Win32Platform::IsKeyReleased(KeyCode key)
{
    Assert(static_cast<size_t>(key) < _Input.KeyStates.KeysDown.size());
	return _Input.KeyStates.KeysReleased[_KeyMap[key]];
}

V2 Win32Platform::GetMousePosition()
{
	return V2{ 
        static_cast<float>(_Input.MousePosition.X), 
        static_cast<float>(_Input.MousePosition.Y) };
}

V2 Win32Platform::GetMouseDelta()
{
	return _Input.MouseDelta;
}

void Win32Platform::SetMouseDelta(const V2& delta)
{
    _Input.MouseDelta = delta;
}

void Win32Platform::PlatformShowCursor(const bool show)
{
	ShowCursor(show);
}

void Win32Platform::PlatformConfineCursorToWindow(const bool confine)
{
    if (confine)
    {
        RECT rect;
        GetClientRect(_Hwnd, &rect);
        MapWindowPoints(_Hwnd, nullptr, (POINT*)&rect, 2);
        ClipCursor(&rect);
    }
    else
    {
        ClipCursor(nullptr);
	}
}

#endif // _WIN32