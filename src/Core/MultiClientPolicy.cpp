#include "stdafx.h"
#include "Core/MultiClientPolicy.h"

#include <cstdint>
#include <cwctype>

namespace
{
constexpr int kDefaultMaximumClients = 4;
constexpr int kHardMaximumClients = 4;
HANDLE gClientSlot = nullptr;

uint64_t HashInstallationPath(const wchar_t* path)
{
    uint64_t hash = 1469598103934665603ULL;
    for (const wchar_t* cursor = path; *cursor != L'\0'; ++cursor)
    {
        const wchar_t normalized = static_cast<wchar_t>(std::towlower(*cursor));
        hash ^= static_cast<uint64_t>(normalized);
        hash *= 1099511628211ULL;
    }
    return hash;
}
}

bool MultiClientPolicy_AcquireSlot()
{
    if (gClientSlot != nullptr)
    {
        return true;
    }

    int maximumClients = GetPrivateProfileIntA(
        "MultiClient", "MaxInstances", kDefaultMaximumClients, ".\\Config.ini");
    if (maximumClients < 1)
    {
        maximumClients = 1;
    }
    if (maximumClients > kHardMaximumClients)
    {
        maximumClients = kHardMaximumClients;
    }

    wchar_t executablePath[32768] = {};
    const DWORD pathLength = GetModuleFileNameW(nullptr, executablePath, ARRAYSIZE(executablePath));
    if (pathLength == 0 || pathLength >= ARRAYSIZE(executablePath))
    {
        MessageBoxA(nullptr, "Unable to validate the MU client installation path.",
            "MU Origins", MB_OK | MB_ICONERROR);
        return false;
    }

    const unsigned long long installationHash = HashInstallationPath(executablePath);
    for (int slot = 1; slot <= maximumClients; ++slot)
    {
        wchar_t mutexName[128] = {};
        swprintf_s(mutexName, L"Local\\MuOrigins.Client.%016llX.Slot%d",
            installationHash, slot);

        HANDLE candidate = CreateMutexW(nullptr, FALSE, mutexName);
        if (candidate == nullptr)
        {
            continue;
        }

        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            gClientSlot = candidate;
            return true;
        }

        CloseHandle(candidate);
    }

    char message[192] = {};
    sprintf_s(message,
        "This MU Origins installation allows up to %d simultaneous clients.\n"
        "Close one client before opening another.", maximumClients);
    MessageBoxA(nullptr, message, "MU Origins", MB_OK | MB_ICONINFORMATION);
    return false;
}
