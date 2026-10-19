#include <windows.h>
#include <shellapi.h>

#include <algorithm>
#include <cwctype>
#include <map>
#include <string>
#include <vector>

namespace
{
constexpr wchar_t kManagerClass[] = L"MuOriginsWindowManager";
constexpr wchar_t kManagerMutex[] = L"Local\\MuOriginsWindowManager.Singleton";
constexpr wchar_t kClientExecutable[] = L"main.exe";
constexpr wchar_t kClientWindowClass[] = L"Dialog";
constexpr UINT kHotkeyId = 1;
constexpr UINT kDiscoveryTimerId = 1;
constexpr UINT kTrayMessage = WM_APP + 1;
constexpr UINT kTrayIconId = 1;
constexpr UINT kCommandRestore = 1001;
constexpr UINT kCommandHide = 1002;
constexpr UINT kCommandExit = 1003;
constexpr size_t kMaximumClients = 4;

struct ManagedWindow
{
    HWND window = nullptr;
    DWORD processId = 0;
    std::wstring executablePath;
    WINDOWPLACEMENT placement = {sizeof(WINDOWPLACEMENT)};
    bool hiddenByManager = false;
};

std::map<HWND, ManagedWindow> gManagedWindows;
bool gWindowsHidden = false;
bool gHotkeyRegistered = false;
NOTIFYICONDATAW gTrayIcon = {};

std::wstring ToLower(std::wstring value)
{
    std::transform(value.begin(), value.end(), value.begin(),
        [](wchar_t character) { return static_cast<wchar_t>(std::towlower(character)); });
    return value;
}

std::wstring GetProcessPath(DWORD processId)
{
    std::wstring result;
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (process == nullptr)
    {
        return result;
    }

    wchar_t path[32768] = {};
    DWORD pathLength = ARRAYSIZE(path);
    if (QueryFullProcessImageNameW(process, 0, path, &pathLength) != FALSE)
    {
        result.assign(path, pathLength);
    }

    CloseHandle(process);
    return result;
}

bool IsDirectory(const std::wstring& path)
{
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool IsClientExecutable(const std::wstring& executablePath)
{
    const size_t slash = executablePath.find_last_of(L"\\/");
    if (slash == std::wstring::npos)
    {
        return false;
    }

    if (ToLower(executablePath.substr(slash + 1)) != ToLower(kClientExecutable))
    {
        return false;
    }

    // A process named main.exe is not sufficient. A MU client installation
    // must have its Data directory beside the executable.
    return IsDirectory(executablePath.substr(0, slash) + L"\\Data");
}

bool QueryClientIdentity(HWND window, DWORD& processId, std::wstring& executablePath)
{
    if (window == nullptr || IsWindow(window) == FALSE || GetWindow(window, GW_OWNER) != nullptr)
    {
        return false;
    }

    wchar_t className[128] = {};
    if (GetClassNameW(window, className, ARRAYSIZE(className)) == 0 ||
        _wcsicmp(className, kClientWindowClass) != 0)
    {
        return false;
    }

    GetWindowThreadProcessId(window, &processId);
    if (processId == 0 || processId == GetCurrentProcessId())
    {
        return false;
    }

    executablePath = GetProcessPath(processId);
    return !executablePath.empty() && IsClientExecutable(executablePath);
}

bool IdentityStillMatches(const ManagedWindow& client)
{
    DWORD currentProcessId = 0;
    std::wstring currentPath;
    return QueryClientIdentity(client.window, currentProcessId, currentPath) &&
        currentProcessId == client.processId &&
        ToLower(currentPath) == ToLower(client.executablePath);
}

BOOL CALLBACK CollectClientWindow(HWND window, LPARAM parameter)
{
    auto* windows = reinterpret_cast<std::vector<ManagedWindow>*>(parameter);
    if (windows->size() >= kMaximumClients)
    {
        return FALSE;
    }

    DWORD processId = 0;
    std::wstring executablePath;
    if (!QueryClientIdentity(window, processId, executablePath))
    {
        return TRUE;
    }

    const auto duplicate = std::find_if(windows->begin(), windows->end(),
        [processId](const ManagedWindow& client) { return client.processId == processId; });
    if (duplicate != windows->end())
    {
        return TRUE;
    }

    ManagedWindow client;
    client.window = window;
    client.processId = processId;
    client.executablePath = executablePath;
    client.placement.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(window, &client.placement);
    windows->push_back(client);
    return TRUE;
}

void DiscoverClients()
{
    for (auto iterator = gManagedWindows.begin(); iterator != gManagedWindows.end();)
    {
        if (!IdentityStillMatches(iterator->second))
        {
            iterator = gManagedWindows.erase(iterator);
        }
        else
        {
            ++iterator;
        }
    }

    std::vector<ManagedWindow> discovered;
    EnumWindows(CollectClientWindow, reinterpret_cast<LPARAM>(&discovered));

    for (ManagedWindow& client : discovered)
    {
        if (gManagedWindows.find(client.window) != gManagedWindows.end())
        {
            continue;
        }

        if (gWindowsHidden)
        {
            client.placement.length = sizeof(WINDOWPLACEMENT);
            GetWindowPlacement(client.window, &client.placement);
			ShowWindow(client.window, SW_MINIMIZE);
            client.hiddenByManager = true;
        }

        gManagedWindows.emplace(client.window, client);
    }
}

void HideAllClients()
{
    DiscoverClients();
    for (auto& entry : gManagedWindows)
    {
        ManagedWindow& client = entry.second;
        if (!IdentityStillMatches(client) || IsWindowVisible(client.window) == FALSE)
        {
            continue;
        }

        client.placement.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(client.window, &client.placement);
		// Minimize instead of merely making the HWND invisible. This gives an
		// immediate, standard Windows transition while the game loop and the
		// server-side Helper continue running normally.
		ShowWindow(client.window, SW_MINIMIZE);
        client.hiddenByManager = true;
    }

    gWindowsHidden = true;
}

void RestoreAllClients()
{
    for (auto& entry : gManagedWindows)
    {
        ManagedWindow& client = entry.second;
        if (!client.hiddenByManager || !IdentityStillMatches(client))
        {
            continue;
        }

        client.placement.length = sizeof(WINDOWPLACEMENT);
        SetWindowPlacement(client.window, &client.placement);
		const int showCommand = (client.placement.showCmd == SW_HIDE ||
			client.placement.showCmd == SW_SHOWMINIMIZED ||
			client.placement.showCmd == SW_MINIMIZE)
            ? SW_SHOWNORMAL
            : static_cast<int>(client.placement.showCmd);
        ShowWindow(client.window, showCommand);
		SetForegroundWindow(client.window);
        client.hiddenByManager = false;
    }

    gWindowsHidden = false;
    DiscoverClients();
}

void AddTrayIcon(HWND window)
{
    gTrayIcon.cbSize = sizeof(gTrayIcon);
    gTrayIcon.hWnd = window;
    gTrayIcon.uID = kTrayIconId;
    gTrayIcon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    gTrayIcon.uCallbackMessage = kTrayMessage;
    gTrayIcon.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wcscpy_s(gTrayIcon.szTip, L"MU Origins Window Manager (F12)");
    Shell_NotifyIconW(NIM_ADD, &gTrayIcon);
    gTrayIcon.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &gTrayIcon);
}

void ShowTrayMenu(HWND window)
{
    POINT cursor = {};
    GetCursorPos(&cursor);

    HMENU menu = CreatePopupMenu();
    if (menu == nullptr)
    {
        return;
    }

    AppendMenuW(menu, MF_STRING, kCommandRestore, L"Restore All");
    AppendMenuW(menu, MF_STRING, kCommandHide, L"Hide All");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kCommandExit, L"Exit");
    SetForegroundWindow(window);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_LEFTALIGN,
        cursor.x, cursor.y, 0, window, nullptr);
    DestroyMenu(menu);
}

LRESULT CALLBACK ManagerWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        AddTrayIcon(window);
        SetTimer(window, kDiscoveryTimerId, 1000, nullptr);
        gHotkeyRegistered = RegisterHotKey(window, kHotkeyId, MOD_NOREPEAT, VK_F12) != FALSE;
        if (!gHotkeyRegistered)
        {
            MessageBoxW(window,
                L"F12 is already registered by another application.\n"
                L"Use the tray icon to Hide All or Restore All.",
                L"MU Origins Window Manager", MB_OK | MB_ICONWARNING);
        }
        DiscoverClients();
        return 0;

    case WM_TIMER:
        if (wParam == kDiscoveryTimerId)
        {
            DiscoverClients();
        }
        return 0;

    case WM_HOTKEY:
        if (wParam == kHotkeyId)
        {
            gWindowsHidden ? RestoreAllClients() : HideAllClients();
        }
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case kCommandRestore:
            RestoreAllClients();
            return 0;
        case kCommandHide:
            HideAllClients();
            return 0;
        case kCommandExit:
            DestroyWindow(window);
            return 0;
        default:
            break;
        }
        break;

    case kTrayMessage:
        if (LOWORD(lParam) == WM_RBUTTONUP || LOWORD(lParam) == WM_CONTEXTMENU)
        {
            ShowTrayMenu(window);
        }
        else if (LOWORD(lParam) == WM_LBUTTONDBLCLK)
        {
            gWindowsHidden ? RestoreAllClients() : HideAllClients();
        }
        return 0;

    case WM_DESTROY:
        KillTimer(window, kDiscoveryTimerId);
        RestoreAllClients();
        if (gHotkeyRegistered)
        {
            UnregisterHotKey(window, kHotkeyId);
        }
        Shell_NotifyIconW(NIM_DELETE, &gTrayIcon);
        PostQuitMessage(0);
        return 0;

    default:
        break;
    }

    return DefWindowProcW(window, message, wParam, lParam);
}
} // namespace

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, wchar_t*, int)
{
    HANDLE singleton = CreateMutexW(nullptr, TRUE, kManagerMutex);
    if (singleton == nullptr)
    {
        return 1;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        MessageBoxW(nullptr, L"MU Origins Window Manager is already running.",
            L"MU Origins Window Manager", MB_OK | MB_ICONINFORMATION);
        CloseHandle(singleton);
        return 0;
    }

    WNDCLASSEXW windowClass = {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = ManagerWindowProc;
    windowClass.hInstance = instance;
    windowClass.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.lpszClassName = kManagerClass;

    if (RegisterClassExW(&windowClass) == 0)
    {
        CloseHandle(singleton);
        return 1;
    }

    HWND window = CreateWindowExW(0, kManagerClass, L"MU Origins Window Manager",
        WS_OVERLAPPED, 0, 0, 0, 0, nullptr, nullptr, instance, nullptr);
    if (window == nullptr)
    {
        CloseHandle(singleton);
        return 1;
    }

    MSG message = {};
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    CloseHandle(singleton);
    return static_cast<int>(message.wParam);
}
