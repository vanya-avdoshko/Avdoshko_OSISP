#include <windows.h>
#include <iostream>
#include <string>
#include <iphlpapi.h>
#include <vector>
#pragma comment(lib, "iphlpapi.lib")

void PrintSystemInfo();
void PrintRegistryInfo();
void PrintDiskInfo();
void PrintNetworkInfo();
void PrintMemoryInfo();
void PrintUserInfo();

int main() {
    std::cout << "System Information Utility\n";
    std::cout << "--------------------------\n";

    PrintSystemInfo();
    PrintRegistryInfo();
    PrintDiskInfo();
    PrintNetworkInfo();
    PrintMemoryInfo();
    PrintUserInfo();

    std::cout << "\nPress any key to exit...";
    std::cin.get();
    return 0;
}

void PrintSystemInfo() {
    // Получение версии ОС через реестр
    HKEY hKey;  //получаем дескриптор открытого ключа
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD dwType = 0;
        TCHAR szValue[512];
        DWORD dwSize = sizeof(szValue);
        //используем дескриптор открытого ключа в качестве параметра для считывания значений подключа с помощью ф-ии RegQueryValueEx
        if (RegQueryValueEx(hKey, L"ProductName", NULL, &dwType, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS) {
            std::wcout << "Operating System: " << szValue << std::endl;
        }
        if (RegQueryValueEx(hKey, L"CurrentBuild", NULL, &dwType, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS) {
            std::wcout << "Build Number: " << szValue << std::endl;
        }
        if (RegQueryValueEx(hKey, L"EditionID", NULL, &dwType, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS) {
            std::wcout << "Edition: " << szValue << std::endl;
        }
        if (RegQueryValueEx(hKey, L"ReleaseId", NULL, &dwType, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS) {
            std::wcout << "Release ID: " << szValue << std::endl;
        }

        RegCloseKey(hKey);
    }
    else {
        std::cerr << "Failed to open registry key for OS info.\n";
    }
}

void PrintRegistryInfo() {
    // Получение информации о Windows через реестр
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD dwType = 0;
        TCHAR szValue[512];
        DWORD dwSize = sizeof(szValue);

        if (RegQueryValueEx(hKey, L"ProgramFilesDir", NULL, &dwType, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS) {
            std::wcout << "Program Files Directory: " << szValue << std::endl;
        }
        if (RegQueryValueEx(hKey, L"CommonFilesDir", NULL, &dwType, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS) {
            std::wcout << "Common Files Directory: " << szValue << std::endl;
        }
        if (RegQueryValueEx(hKey, L"CommonFilesDir (x86)", NULL, &dwType, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS) {
            std::wcout << "Common Files Directory (x86): " << szValue << std::endl;
        }

        RegCloseKey(hKey);
    }
    else {
        std::cerr << "Failed to open registry key for program files directory.\n";
    }
}

void PrintDiskInfo() {
    std::cout << "\nDisk Information:\n";

    char driveStrings[256];
    DWORD driveLength = GetLogicalDriveStringsA(sizeof(driveStrings), driveStrings);
    char* drive = driveStrings;

    while (*drive) {
        ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
        if (GetDiskFreeSpaceExA(drive, &freeBytesAvailable, &totalBytes, &totalFreeBytes)) {
            std::cout << "  Drive: " << drive << "\n";
            std::cout << "    Total Space: " << totalBytes.QuadPart / (1024 * 1024) << " MB\n";
            std::cout << "    Free Space: " << totalFreeBytes.QuadPart / (1024 * 1024) << " MB\n";
        }
        drive += strlen(drive) + 1;
    }
}

void PrintNetworkInfo() {
    std::cout << "\nNetwork Information:\n";

    ULONG bufferSize = 0;
    GetAdaptersInfo(nullptr, &bufferSize);
    std::vector<BYTE> buffer(bufferSize);
    IP_ADAPTER_INFO* adapterInfo = reinterpret_cast<IP_ADAPTER_INFO*>(buffer.data());

    if (GetAdaptersInfo(adapterInfo, &bufferSize) == ERROR_SUCCESS) {
        while (adapterInfo) {
            std::cout << "  Adapter Name: " << adapterInfo->AdapterName << "\n";
            std::cout << "  Description: " << adapterInfo->Description << "\n";
            std::cout << "  IP Address: " << adapterInfo->IpAddressList.IpAddress.String << "\n";
            std::cout << "  Subnet Mask: " << adapterInfo->IpAddressList.IpMask.String << "\n";
            std::cout << "  Gateway: " << adapterInfo->GatewayList.IpAddress.String << "\n";
            adapterInfo = adapterInfo->Next;
        }
    }
    else {
        std::cerr << "Failed to retrieve network information.\n";
    }
}

void PrintMemoryInfo() {
    MEMORYSTATUSEX memInfo = { sizeof(MEMORYSTATUSEX) };
    GlobalMemoryStatusEx(&memInfo);

    std::cout << "\nMemory Information:\n";
    std::cout << "  Total Physical Memory: " << memInfo.ullTotalPhys / (1024 * 1024) << " MB\n";
    std::cout << "  Available Physical Memory: " << memInfo.ullAvailPhys / (1024 * 1024) << " MB\n";
}

void PrintUserInfo() {
    char username[256];
    DWORD username_len = sizeof(username);
    if (GetUserNameA(username, &username_len)) {
        std::cout << "\nUser Information:\n";
        std::cout << "  Logged In User: " << username << "\n";
    }
    else {
        std::cerr << "Failed to get username.\n";
    }
}
