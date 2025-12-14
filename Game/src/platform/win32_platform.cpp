#include "pch.h"
#include "platform/platform.h"

#ifdef _WIN32

static inline HWND Hwnd;
static inline HINSTANCE hInstance;
static int nCmdShow;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void PlatformInitWindow(int windowWidth, int windowHeight, const wchar_t* title)
{
    // Register the window class.
    const wchar_t CLASS_NAME[] = L"Window Class";

    WNDCLASS wc = { };

	hInstance = GetModuleHandle(NULL);
    nCmdShow = SW_SHOW;

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

void PlatformUpdateWindow(bool& running)
{
	MSG msg = { };
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
            running = false;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void* PlatformGetWindowHandle()
{
	return static_cast<void*>(Hwnd);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

        EndPaint(hwnd, &ps);
    }
    return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

#endif // _WIN32