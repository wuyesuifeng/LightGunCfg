#include "stdio.h"
#include <Windows.h>
#include <setupapi.h>
#include <tchar.h>

#include <initguid.h> // include before devpropdef.h
#include <devpropdef.h>
#include <devpkey.h>

#pragma comment(lib, "setupapi.lib")

void PrintDevicesInfo1() {
    HDEVINFO hDevInfo = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_PRESENT | DIGCF_ALLCLASSES);
    if (hDevInfo == INVALID_HANDLE_VALUE) {
        printf("SetupDiGetClassDevs Err:%d", GetLastError());
        return;
    };

    SP_CLASSIMAGELIST_DATA sp_ImageData = {0};
    sp_ImageData.cbSize = sizeof(SP_CLASSIMAGELIST_DATA);
    SetupDiGetClassImageList(&sp_ImageData);

    SP_DEVINFO_DATA spDevInfoData = {0};
    spDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for (short wIndex = 0; SetupDiEnumDeviceInfo(hDevInfo, wIndex, &spDevInfoData); wIndex++) {
        TCHAR szBuf[MAX_PATH] = {0};
        int wImageIdx = 0;
        short wItem = 0;
        SetupDiGetDeviceRegistryProperty(hDevInfo, &spDevInfoData, SPDRP_CLASS, NULL, (PBYTE)szBuf, MAX_PATH, 0);
        SetupDiGetClassImageIndex(&sp_ImageData, &spDevInfoData.ClassGuid, &wImageIdx);
        char szName[MAX_PATH] = {0};
        DWORD dwRequireSize;
        SetupDiGetClassDescription(&spDevInfoData.ClassGuid, szBuf, MAX_PATH, &dwRequireSize);
        printf("Class:%s\r\n", szBuf);

        if (SetupDiGetDeviceRegistryProperty(hDevInfo, &spDevInfoData, SPDRP_FRIENDLYNAME, NULL, (PBYTE)szName, MAX_PATH - 1, 0)) {
            printf("name:%s\r\n\r\n", szName);
        } else {
            DEVPROPTYPE propType = 0;
            SetupDiGetDevicePropertyW(hDevInfo, &spDevInfoData, &DEVPKEY_Device_Manufacturer, &propType, (PBYTE)szName, MAX_PATH - 1, NULL, 0);
            printf("############# name: %s\r\n\r\n", szName);
        }
    }
    SetupDiDestroyClassImageList(&sp_ImageData);
}
int PrintDevicesInfo2() {

    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD i;

    // 得到所有设备 HDEVINFO
    hDevInfo = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_PRESENT | DIGCF_ALLCLASSES);

    if (hDevInfo == INVALID_HANDLE_VALUE)
        return 0;

    // 循环列举
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for (i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData); i++) {
        char szClassBuf[MAX_PATH] = {0};
        char szDescBuf[MAX_PATH] = {0};

        // 获取类名
        if (!SetupDiGetDeviceRegistryProperty(hDevInfo, &DeviceInfoData, SPDRP_CLASS, NULL, (PBYTE)szClassBuf, MAX_PATH - 1, NULL))
            continue;

        // 获取设备描述信息
        if (!SetupDiGetDeviceRegistryProperty(hDevInfo, &DeviceInfoData, SPDRP_FRIENDLYNAME, NULL, (PBYTE)szDescBuf, MAX_PATH - 1, NULL))
            continue;

        printf("Class:%s\r\nDesc:%s\r\n\r\n", szClassBuf, szDescBuf);

        if (strcmp(szClassBuf, _T("Display")) == 0) {
            printf("Class:%s\r\n  Desc:%s\r\n\r\n", szClassBuf, szDescBuf);
        }
    }

    //  释放
    SetupDiDestroyDeviceInfoList(hDevInfo);

    return 0;
}

int main() {
    PrintDevicesInfo1();
    // PrintDevicesInfo2();
    return 0;
}