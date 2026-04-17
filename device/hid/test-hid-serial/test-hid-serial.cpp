// test-hid-serial.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <windows.h>
extern "C" {
#include <setupapi.h>
#include <hidsdi.h>
}
#include <stdio.h>
#include <tchar.h>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "hid.lib")

int main()
{
    GUID hidGuid;
    HDEVINFO deviceInfoSet;
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA detailData = NULL;
    DWORD requiredSize = 0;
    HANDLE deviceHandle;
    WCHAR serialNumber[256];
    DWORD index = 0;

    HidD_GetHidGuid(&hidGuid);
    deviceInfoSet = SetupDiGetClassDevs(&hidGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        printf("Failed to get HID device list.\n");
        return 1;
    }

    printf("Enumerating HID devices and reading serial numbers:\n");

    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    while (SetupDiEnumDeviceInterfaces(deviceInfoSet, NULL, &hidGuid, index, &deviceInterfaceData)) {
        index++;
        printf("Device %d:\n", index);

        // 获取路径长度
        SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, NULL, 0, &requiredSize, NULL);
        detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(requiredSize);
        detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        if (!SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, detailData, requiredSize, NULL, NULL)) {
            printf("  Failed to get device detail.\n");
            free(detailData);
            continue;
        }

        _tprintf(TEXT("\nDevice path: %s\n"), detailData->DevicePath);

        // 打开设备
        deviceHandle = CreateFile(detailData->DevicePath,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);

        if (deviceHandle == INVALID_HANDLE_VALUE) {
            printf("  Cannot open device.\n");
            free(detailData);
            continue;
        }

        // 读取序列号
        if (HidD_GetSerialNumberString(deviceHandle, serialNumber, sizeof(serialNumber))) {
            wprintf(L"  Serial: %s\n", serialNumber);
        }
        else {
            printf("  Serial: <not available>\n");
        }

        CloseHandle(deviceHandle);
        free(detailData);
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return 0;
}

