// hello-time.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>

#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <chrono>
#include <iomanip>

void wprintf_gbk(const wchar_t* wstr)
{
    char buf[256];
    WideCharToMultiByte(936, 0, wstr, -1, buf, sizeof(buf), NULL, NULL);
    printf("%s", buf);
}

void print_win32_time()
{
    SYSTEMTIME localTime, systemTime;
    GetLocalTime(&localTime);
    GetSystemTime(&systemTime);

    printf("=== Win32 API time ===\n");
    printf("LocalTime: %04d-%02d-%02d %02d:%02d:%02d.%03d\n",
        localTime.wYear, localTime.wMonth, localTime.wDay,
        localTime.wHour, localTime.wMinute, localTime.wSecond, localTime.wMilliseconds);

    printf("SystemTime (UTC): %04d-%02d-%02d %02d:%02d:%02d.%03d\n\n",
        systemTime.wYear, systemTime.wMonth, systemTime.wDay,
        systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds);
}

void print_filetime()
{
    FILETIME ft;
    SYSTEMTIME stUTC;
    GetSystemTimeAsFileTime(&ft);
    FileTimeToSystemTime(&ft, &stUTC);

    printf("=== FILETIME ===\n");
    printf("SystemTimeAsFileTime: %04d-%02d-%02d %02d:%02d:%02d.%03d (UTC)\n\n",
        stUTC.wYear, stUTC.wMonth, stUTC.wDay,
        stUTC.wHour, stUTC.wMinute, stUTC.wSecond, stUTC.wMilliseconds);
}

void print_timezone_info()
{
    TIME_ZONE_INFORMATION tzi;
    DWORD res = GetTimeZoneInformation(&tzi);

    printf("=== GetTimeZoneInformation ===\n");
    printf("Bias (min): %ld\n", tzi.Bias);
    printf("StandardName: ");
    wprintf_gbk(tzi.StandardName);
    printf("\n");
    printf("DaylightName: ");
    wprintf_gbk(tzi.DaylightName);
    printf("\n");

    if (res == TIME_ZONE_ID_STANDARD)
        printf("Current time zone: Standard time\n\n");
    else if (res == TIME_ZONE_ID_DAYLIGHT)
        printf("Current time zone: Daylight saving time\n\n");
    else
        printf("Current time zone: Unknown\n\n");
}

void print_dynamic_timezone()
{
    DYNAMIC_TIME_ZONE_INFORMATION dtzi;
    GetDynamicTimeZoneInformation(&dtzi);

    printf("=== GetDynamicTimeZoneInformation ===\n");
    printf("TimeZoneKeyName: ");
    wprintf_gbk(dtzi.TimeZoneKeyName);
    printf("\n");
    printf("StandardName: ");
    wprintf_gbk(dtzi.StandardName);
    printf("\n");
    printf("DaylightName: ");
    wprintf_gbk(dtzi.DaylightName);
    printf("\n");
    printf("Bias (min): %ld\n\n", dtzi.Bias);
}

void print_cruntime_time()
{
    time_t now = time(NULL);
    struct tm local_tm, utc_tm;
    localtime_s(&local_tm, &now);
    gmtime_s(&utc_tm, &now);

    printf("=== C runtime (time.h) ===\n");
    printf("Local time: %04d-%02d-%02d %02d:%02d:%02d\n",
        local_tm.tm_year + 1900, local_tm.tm_mon + 1, local_tm.tm_mday,
        local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec);

    printf("UTC time: %04d-%02d-%02d %02d:%02d:%02d\n\n",
        utc_tm.tm_year + 1900, utc_tm.tm_mon + 1, utc_tm.tm_mday,
        utc_tm.tm_hour, utc_tm.tm_min, utc_tm.tm_sec);
}

void print_chrono_time()
{
    using namespace std::chrono;
    auto now = system_clock::now();
    auto t_c = system_clock::to_time_t(now);
    std::tm local_tm, utc_tm;
    localtime_s(&local_tm, &t_c);
    gmtime_s(&utc_tm, &t_c);

    std::cout << "=== C++ <chrono> ===\n";
    std::cout << "Local: " << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S") << "\n";
    std::cout << "UTC:   " << std::put_time(&utc_tm, "%Y-%m-%d %H:%M:%S") << "\n\n";
}



int main()
{
    print_win32_time();
    print_filetime();
    print_timezone_info();
    print_dynamic_timezone();
    print_cruntime_time();
    print_chrono_time();
    return 0;
}
