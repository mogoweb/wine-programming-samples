#include <windows.h>
#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <memory>

// 用于读取注册表的辅助函数
std::wstring ReadRegistryValue(HKEY root, const std::wstring& subkey, const std::wstring& valueName) {
    HKEY hKey;
    if (RegOpenKeyExW(root, subkey.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return L"(无法打开注册表)";

    WCHAR buffer[256];
    DWORD bufferSize = sizeof(buffer);
    DWORD type = 0;
    if (RegQueryValueExW(hKey, valueName.c_str(), nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &bufferSize) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return L"(读取失败)";
    }
    RegCloseKey(hKey);
    return std::wstring(buffer);
}

// 获取 Win32 API 时区信息
void PrintWin32TimeZoneInfo() {
    TIME_ZONE_INFORMATION tzi;
    DWORD result = GetTimeZoneInformation(&tzi);

    std::wcout << L"\n[1] GetTimeZoneInformation()\n";
    std::wcout << L"标准时间: " << tzi.StandardName << L"\n";
    std::wcout << L"夏令时:   " << tzi.DaylightName << L"\n";
    std::wcout << L"Bias (分钟): " << tzi.Bias << L"\n";
    std::wcout << L"返回值: ";
    switch (result) {
    case TIME_ZONE_ID_STANDARD: std::wcout << L"标准时区\n"; break;
    case TIME_ZONE_ID_DAYLIGHT: std::wcout << L"夏令时区\n"; break;
    case TIME_ZONE_ID_UNKNOWN: std::wcout << L"未知或不使用夏令时\n"; break;
    default: std::wcout << L"错误\n"; break;
    }

    DYNAMIC_TIME_ZONE_INFORMATION dtzi;
    GetDynamicTimeZoneInformation(&dtzi);
    std::wcout << L"\n[2] GetDynamicTimeZoneInformation()\n";
    std::wcout << L"时区 KeyName: " << dtzi.TimeZoneKeyName << L"\n";
    std::wcout << L"动态 Bias: " << dtzi.Bias << L"\n";
}

// 使用 C++20 chrono 库
void PrintChronoTimeZoneInfo() {
#if __cpp_lib_chrono >= 201907L
    std::wcout << L"\n[3] std::chrono::current_zone()\n";
    try {
        auto* tz = std::chrono::current_zone();
        std::wcout << L"时区名称: " << tz->name() << L"\n";

        auto now = std::chrono::system_clock::now();
        auto local = tz->to_local(now);
        std::time_t t = std::chrono::system_clock::to_time_t(local);
        std::wcout << L"当前本地时间: " << std::put_time(std::localtime(&t), "%F %T") << L"\n";
    } catch (const std::exception& e) {
        std::wcout << L"std::chrono 获取失败: " << e.what() << L"\n";
    }
#else
    std::wcout << L"\n[3] 当前编译器不支持 C++20 chrono 时区功能。\n";
#endif
}

// 读取注册表
void PrintRegistryTimeZoneInfo() {
    std::wcout << L"\n[4] 读取注册表时区信息:\n";
    const std::wstring subkey = L"SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation";

    std::wcout << L"TimeZoneKeyName: " << ReadRegistryValue(HKEY_LOCAL_MACHINE, subkey, L"TimeZoneKeyName") << L"\n";
    std::wcout << L"StandardName:    " << ReadRegistryValue(HKEY_LOCAL_MACHINE, subkey, L"StandardName") << L"\n";
    std::wcout << L"DaylightName:    " << ReadRegistryValue(HKEY_LOCAL_MACHINE, subkey, L"DaylightName") << L"\n";
}

// 调用 WMIC 命令（备用方法）
void PrintWMICOutput() {
    std::wcout << L"\n[5] 调用 WMIC 命令:\n";

    FILE* pipe = _popen("wmic timezone get Caption /value", "r");
    if (!pipe) {
        std::wcout << L"无法执行 wmic 命令\n";
        return;
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        std::cout << buffer;
    }
    _pclose(pipe);
}

int main() {
    std::wcout.imbue(std::locale(""));

    std::wcout << L"===== Windows 时区信息获取 Demo =====\n";
    PrintWin32TimeZoneInfo();
    PrintChronoTimeZoneInfo();
    PrintRegistryTimeZoneInfo();
    PrintWMICOutput();

    std::wcout << L"\n================ 完成 =================\n";
    return 0;
}
