#include <iostream>
#include <Windows.h>
#include <ctime>

HANDLE hCon;
void PrintfTime(const char* pFormat, ...)
{
    tm tTime; time_t tTemp = std::time(0);
    localtime_s(&tTime, &tTemp);
    char cTime[12];
    strftime(cTime, sizeof(cTime), "[%H:%M:%S] ", &tTime);
    SetConsoleTextAttribute(hCon, 12 /*Red*/); fputs(cTime, stdout);
    char cTemp[1024];
    char* pArgs;
    va_start(pArgs, pFormat);
    _vsnprintf_s(cTemp, sizeof(cTemp) - 1, pFormat, pArgs);
    va_end(pArgs);
    cTemp[sizeof(cTemp) - 1] = 0;
    SetConsoleTextAttribute(hCon, 15 /*White*/); puts(cTemp);
}

std::wstring wFromString(const std::string& s)
{
    int slength = int(s.length()) + 1;
    int len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
    wchar_t* buf = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
    std::wstring r(buf);
    delete[] buf;
    return r;
}

std::string sRegistry(HKEY hKey, std::string sPath, std::string sKey)
{
    HKEY hTempKey;
    if (RegOpenKeyExW(hKey, wFromString(sPath).c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &hTempKey) == ERROR_SUCCESS)
    {
        std::wstring wKey = wFromString(sKey);
        LPCWSTR lKey = wKey.c_str();
        DWORD dType, dData;
        if (RegQueryValueExW(hTempKey, lKey, 0, &dType, 0, &dData) == ERROR_SUCCESS)
        {
            std::wstring wValue(dData / sizeof(wchar_t), L'\0');
            if (RegQueryValueExW(hTempKey, lKey, 0, 0, reinterpret_cast<LPBYTE>(&wValue[0]), &dData) == ERROR_SUCCESS) 
                return std::string(wValue.begin(), wValue.end());
        }
    }
    return "";
}

void OnExit()
{
    puts(""); // Fast new line boys.
    PrintfTime("Exiting in 10 seconds...");
    Sleep(10000);
    exit(0);
}

char cCMDLine[1024];
int main()
{
    SetConsoleTitleA("VAC Error Fixerino");
    hCon = GetStdHandle(STD_OUTPUT_HANDLE);

    std::string sSteamPath = sRegistry(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", "SteamPath");
    if (sSteamPath.empty())
    {
        PrintfTime("[ERROR] Couldn't find Steam Path!");
        OnExit();
    }
    sSteamPath.pop_back(); // Remove null char.
    PrintfTime("Steam Path: %s", sSteamPath.c_str());

    bool bSteamRunning = FindWindowA("vguiPopupWindow", 0); // Faster than looping through each process.
    PrintfTime("Steam Running: %s", bSteamRunning ? "Yes" : "No");
    if (bSteamRunning)
    {
        PrintfTime("Killing steam.exe & all child processes.");
        system("taskkill /F /IM steam.exe /T >nul 2>nul"); // Kill steam.exe & all child processes (ex. CSGO)
    }

    PrintfTime("Starting Steam Service Repair");
    sprintf_s(cCMDLine, sizeof(cCMDLine), "\"%s/bin/steamservice.exe\" /repair", sSteamPath.c_str());
    STARTUPINFO sInfo = { 0 }; sInfo.cb = sizeof(sInfo); PROCESS_INFORMATION pInfo = { 0 };
    if (!CreateProcessA(0, cCMDLine, 0, 0, 0, CREATE_NEW_CONSOLE, 0, 0, &sInfo, &pInfo))
    {
        PrintfTime("[ERROR] Couldn't start Steam Service!");
        OnExit();
    }
    else
    {
        PrintfTime("Waiting for Steam Service Repair to finish.");
        WaitForSingleObject(pInfo.hProcess, INFINITE);
        CloseHandle(pInfo.hProcess);
        CloseHandle(pInfo.hThread);
    }
    PrintfTime("Steam Service Repair done!");

    if (bSteamRunning) // Restart Steam if it was running before.
    {
        sprintf_s(cCMDLine, sizeof(cCMDLine), "\"%s/steam.exe\"", sSteamPath.c_str());
        RtlZeroMemory(&sInfo, sizeof(sInfo)); sInfo.cb = sizeof(sInfo); RtlZeroMemory(&pInfo, sizeof(pInfo));
        if (CreateProcessA(0, cCMDLine, 0, 0, 0, CREATE_NEW_CONSOLE, 0, 0, &sInfo, &pInfo))
        {
            CloseHandle(pInfo.hProcess);
            CloseHandle(pInfo.hThread);
        }
        system(cCMDLine);
    }

    PrintfTime("All done!");
    OnExit();
    return 0; // Never reach. :)
}