// ShellTray.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "ShellTray.h"

// Global Variables:
HANDLE m_singleInstanceMutex;
constexpr WCHAR const szWindowMutex[] = L"com_xellanix_pickcolor_b1e55a6b-280a-5a89-ab79-1816125185c5";

static constexpr auto WM_TRAY = WM_USER + 1;
bool isKeyboardActivation = true;

constexpr std::wstring_view EyedropToolKey()
{
    return L"e2b7a873-937e-5b53-8242-c7b07ba6134f";
}

void LaunchApp(std::wstring_view const args = {})
{
    ShellExecuteW(NULL, NULL, (fs::path(Xellanix::Utilities::AppDir) / L"PickColor.exe").wstring().c_str(), 
                  args.data(), NULL, SW_SHOWNORMAL);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_TRAY:
        {
            if (lParam == WM_RBUTTONUP)
            {
                POINT p;
                GetCursorPos(&p);

                HMENU hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDC_SHELLTRAY));
                if (hMenu)
                {
                    HMENU hSubMenu = GetSubMenu(hMenu, 0);
                    if (hSubMenu)
                    {
                        // our window must be foreground before calling TrackPopupMenu or the menu will not disappear when the user clicks away
                        SetForegroundWindow(hwnd);

                        // If the menu item has checked last time set its state to checked before the menu window shows up.
                        if (isKeyboardActivation)
                        {
                            MENUITEMINFO mi = { 0 };
                            mi.cbSize = sizeof(MENUITEMINFO);
                            mi.fMask = MIIM_STATE;
                            mi.fState = MF_CHECKED;
                            SetMenuItemInfo(hSubMenu, IDM_KEYBOARDACTIVATION, FALSE, &mi);
                        }

                        // respect menu drop alignment
                        UINT uFlags = TPM_RETURNCMD | TPM_NONOTIFY;
                        if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0)
                        {
                            uFlags |= TPM_RIGHTALIGN;
                        }
                        else
                        {
                            uFlags |= TPM_LEFTALIGN;
                        }

                        auto cmd = TrackPopupMenuEx(hSubMenu, uFlags, p.x, p.y, hwnd, NULL);
                        SendMessage(hwnd, WM_COMMAND, cmd, 0);
                    }
                    DestroyMenu(hMenu);
                }
                return 0;
            }
            else if (lParam == WM_LBUTTONUP)
            {
                LaunchApp();
            }
            break;
        }
        case WM_COMMAND:
        {
            int const wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
                case IDM_SHOW:
                {
                    LaunchApp();
                    break;
                }
                case IDM_PICKCOLOR:
                {
                    LaunchApp(EyedropToolKey());
                    break;
                }
                case IDM_KEYBOARDACTIVATION:
                {
                    isKeyboardActivation = !isKeyboardActivation;

                    break;
                }
                case IDM_EXIT:
                {
                    PostQuitMessage(0);
                    break;
                }
                default:
                    return DefWindowProc(hwnd, msg, wParam, lParam);
            }
            break;
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

std::pair<NOTIFYICONDATA, HWND> SetTray(std::wstring const& identifier, HICON icon, std::wstring const& tip)
{
    WNDCLASSEX windowClass;
    memset(&windowClass, 0, sizeof(windowClass));
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = WndProc;
    windowClass.lpszClassName = identifier.c_str();
    windowClass.hInstance = GetModuleHandle(nullptr);

    if (RegisterClassEx(&windowClass) == 0)
    {
        throw std::runtime_error("Failed to register class");
    }

    // NOLINTNEXTLINE
    HWND hwnd = CreateWindow(identifier.c_str(), nullptr, 0, 0, 0, 0, 0, nullptr, nullptr, windowClass.hInstance,
                             nullptr);
    if (hwnd == nullptr)
    {
        throw std::runtime_error("Failed to create window");
    }

    if (UpdateWindow(hwnd) == 0)
    {
        throw std::runtime_error("Failed to update window");
    }

    NOTIFYICONDATA notifyData;
    memset(&notifyData, 0, sizeof(NOTIFYICONDATA));
    notifyData.cbSize = sizeof(NOTIFYICONDATA);
    notifyData.hWnd = hwnd;
    notifyData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
    notifyData.uCallbackMessage = WM_TRAY;
    notifyData.hIcon = icon;
    notifyData.uVersion = NOTIFYICON_VERSION_4;
    wcscpy_s(notifyData.szTip, sizeof(notifyData.szTip), tip.c_str());

    if (Shell_NotifyIcon(NIM_ADD, &notifyData) == FALSE)
    {
        throw std::runtime_error("Failed to register tray icon");
    }

    return std::make_pair(notifyData, hwnd);
}

void DeleteTray(std::wstring const& identifier, NOTIFYICONDATA notifyData, HWND hwnd)
{
    Shell_NotifyIcon(NIM_DELETE, &notifyData);
    DestroyIcon(notifyData.hIcon);

    UnregisterClass(identifier.c_str(), GetModuleHandle(nullptr));
    PostMessage(hwnd, WM_QUIT, 0, 0);

    DestroyIcon(notifyData.hIcon);
}

bool IsKeyDown(int key)
{
    return (GetAsyncKeyState(key) & 0x8000) != 0;
}

void MainWork();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.
    
    m_singleInstanceMutex = CreateMutex(NULL, TRUE, szWindowMutex);
    if (m_singleInstanceMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS)
    {
        ReleaseMutex(m_singleInstanceMutex);
        return FALSE; // Exit the app.
    }

    auto&& [data, hwnd] = SetTray(szWindowMutex, LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SHELLTRAY)), L"Xellanix PickColor");

    MainWork();

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    ReleaseMutex(m_singleInstanceMutex);
    DeleteTray(szWindowMutex, data, hwnd);
    return (int)msg.wParam;
}

void MainWork()
{
    std::thread([]()
    {
        using namespace std::chrono_literals;

        while (true)
        {
            if (!isKeyboardActivation)
            {
                std::this_thread::sleep_for(1s);
                continue;
            }

            if ((IsKeyDown(VK_LWIN) || IsKeyDown(VK_RWIN)) &&
                IsKeyDown(VK_MENU) &&
                IsKeyDown(0x49))
            {
                LaunchApp(EyedropToolKey());
            }

            std::this_thread::sleep_for(10ms);
        }
    }).detach();
}