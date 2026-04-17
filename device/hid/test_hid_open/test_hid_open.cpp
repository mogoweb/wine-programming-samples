#include <iostream>
#include <iomanip>

#include <windows.h>
extern "C" {
#include <setupapi.h>
#include <hidsdi.h>
}

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "hid.lib")

#include <windows.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <iostream>
#include <string>

#pragma comment(lib, "setupapi.lib")

int find_hid()
{
    const std::wstring target_vidpid = L"VID_163C&PID_0A12";

    std::wcout << "find HID " << target_vidpid << std::endl;

    HDEVINFO device_info = SetupDiGetClassDevs(
        NULL,                       // all classes
        L"HID",                     // enumerator (可以换成 NULL 以包含更多设备)
        NULL,
        DIGCF_PRESENT | DIGCF_ALLCLASSES);

    if (device_info == INVALID_HANDLE_VALUE)
    {
        std::wcerr << L"SetupDiGetClassDevs failed." << std::endl;
        return 1;
    }

    SP_DEVINFO_DATA dev_info_data;
    dev_info_data.cbSize = sizeof(SP_DEVINFO_DATA);

    DWORD index = 0;
    while (SetupDiEnumDeviceInfo(device_info, index, &dev_info_data))
    {
        index++;

        WCHAR instance_id[512];
        if (!SetupDiGetDeviceInstanceIdW(device_info, &dev_info_data, instance_id, ARRAYSIZE(instance_id), NULL))
            continue;

        std::wstring id(instance_id);

        if (id.find(target_vidpid) != std::wstring::npos)
        {
            std::wcout << L"Found HID device node: " << id << std::endl;

            // 获取设备描述
            WCHAR desc[256];
            if (SetupDiGetDeviceRegistryPropertyW(device_info, &dev_info_data,
                SPDRP_DEVICEDESC, NULL,
                (PBYTE)desc, sizeof(desc), NULL))
            {
                std::wcout << L"  Description: " << desc << std::endl;
            }

            // 获取驱动程序键名
            WCHAR driver_key[256];
            if (SetupDiGetDeviceRegistryPropertyW(device_info, &dev_info_data,
                SPDRP_DRIVER, NULL,
                (PBYTE)driver_key, sizeof(driver_key), NULL))
            {
                std::wcout << L"  Driver key: " << driver_key << std::endl;
            }

            // 获取服务名（驱动绑定）
            WCHAR service[256];
            if (SetupDiGetDeviceRegistryPropertyW(device_info, &dev_info_data,
                SPDRP_SERVICE, NULL,
                (PBYTE)service, sizeof(service), NULL))
            {
                std::wcout << L"  Service: " << service << std::endl;
            }

            std::wcout << L"--------------------------------------------" << std::endl;
        }
    }

    SetupDiDestroyDeviceInfoList(device_info);
    std::wcout << L"Done." << std::endl;
    return 0;
}

int main()
{
    GUID hidGuid;
    HDEVINFO deviceInfoSet;
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA detailData = NULL;
    DWORD index = 0;
    DWORD requiredSize = 0;
    HANDLE deviceHandle;
    HIDD_ATTRIBUTES attributes;
    BOOL result;

    // 1. 获取 HID 设备的 GUID
    HidD_GetHidGuid(&hidGuid);

    // 2. 枚举系统中所有 HID 接口设备
    deviceInfoSet = SetupDiGetClassDevs(&hidGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (deviceInfoSet == INVALID_HANDLE_VALUE)
    {
        std::cout << "SetupDiGetClassDevs failed." << std::endl;
        return 1;
    }

    std::cout << "Enumerating HID devices..." << std::endl;

    // 3. 枚举设备接口
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    while (SetupDiEnumDeviceInterfaces(deviceInfoSet, NULL, &hidGuid, index, &deviceInterfaceData))
    {
        index++;
        std::cout << "  -> Enumerating device " << index << "..." << std::endl;
        // 4. 获取设备路径
        SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, NULL, 0, &requiredSize, NULL);
        detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(requiredSize);
        detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        if (!SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, detailData, requiredSize, NULL, NULL))
        {
            std::cout << "SetupDiGetDeviceInterfaceDetail failed." << std::endl;
            free(detailData);
            continue;
        }

        std::wcout << L"Device " << index << L" path: " << detailData->DevicePath << std::endl;

        // 5. 打开设备
        deviceHandle = CreateFile(
            detailData->DevicePath,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (deviceHandle == INVALID_HANDLE_VALUE)
        {
            std::cout << "  -> Failed to open device (error " << GetLastError() << ")." << std::endl;
            free(detailData);
            continue;
        }

        // 6. 获取设备属性 (VID / PID)
        attributes.Size = sizeof(attributes);
        result = HidD_GetAttributes(deviceHandle, &attributes);
        if (result)
        {
            std::cout << "  -> Opened HID device: VID = 0x" << std::hex << std::setw(4) << std::setfill('0') << attributes.VendorID
                      << ", PID = 0x" << std::hex << std::setw(4) << std::setfill('0') << attributes.ProductID << std::endl;
        }
        else
        {
            std::cout << "  -> HidD_GetAttributes failed." << std::endl;
        }

        // 7. 关闭设备
        CloseHandle(deviceHandle);
        free(detailData);

        // 如果只想测试第一个设备，可以在这里 break
        break;
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    std::cout << "Done." << std::endl;

    find_hid();
    return 0;
}
