// USBCommunication.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <devguid.h>    // for GUID_DEVCLASS_CDROM etc
#include <setupapi.h>
#include <cfgmgr32.h>   // for MAX_DEVICE_ID_LEN, CM_Get_Parent and CM_Get_Device_ID
#define INITGUID
#include <tchar.h>
#include <stdio.h>
#include <hidsdi.h>
#include <initguid.h>
#include <Usbiodef.h>

#pragma comment (lib,"Setupapi.lib")
#pragma comment (lib,"Hid.lib")


#ifdef DEFINE_DEVPROPKEY
#undef DEFINE_DEVPROPKEY
#endif
#ifdef INITGUID
#define DEFINE_DEVPROPKEY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, pid) EXTERN_C const DEVPROPKEY DECLSPEC_SELECTANY name = { { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }, pid }
#else
#define DEFINE_DEVPROPKEY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, pid) EXTERN_C const DEVPROPKEY name
#endif // INITGUID

// include DEVPKEY_Device_BusReportedDeviceDesc from WinDDK\7600.16385.1\inc\api\devpkey.h
DEFINE_DEVPROPKEY(DEVPKEY_Device_BusReportedDeviceDesc, 0x540b947e, 0x8b40, 0x45bc, 0xa8, 0xa2, 0x6a, 0x0b, 0x89, 0x4c, 0xbd, 0xa2, 4);     // DEVPROP_TYPE_STRING
DEFINE_DEVPROPKEY(DEVPKEY_Device_ContainerId, 0x8c7ed206, 0x3f8a, 0x4827, 0xb3, 0xab, 0xae, 0x9e, 0x1f, 0xae, 0xfc, 0x6c, 2);     // DEVPROP_TYPE_GUID
DEFINE_DEVPROPKEY(DEVPKEY_Device_FriendlyName, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 14);    // DEVPROP_TYPE_STRING
DEFINE_DEVPROPKEY(DEVPKEY_DeviceDisplay_Category, 0x78c34fc8, 0x104a, 0x4aca, 0x9e, 0xa4, 0x52, 0x4d, 0x52, 0x99, 0x6e, 0x57, 0x5a);  // DEVPROP_TYPE_STRING_LIST
DEFINE_DEVPROPKEY(DEVPKEY_Device_LocationInfo, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 15);    // DEVPROP_TYPE_STRING
DEFINE_DEVPROPKEY(DEVPKEY_Device_Manufacturer, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 13);    // DEVPROP_TYPE_STRING
DEFINE_DEVPROPKEY(DEVPKEY_Device_SecuritySDS, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 26);    // DEVPROP_TYPE_SECURITY_DESCRIPTOR_STRING

#define ARRAY_SIZE(arr)     (sizeof(arr)/sizeof(arr[0]))

#pragma comment (lib, "setupapi.lib")

typedef BOOL(WINAPI* FN_SetupDiGetDevicePropertyW)(
    __in       HDEVINFO DeviceInfoSet,
    __in       PSP_DEVINFO_DATA DeviceInfoData,
    __in       const DEVPROPKEY* PropertyKey,
    __out      DEVPROPTYPE* PropertyType,
    __out_opt  PBYTE PropertyBuffer,
    __in       DWORD PropertyBufferSize,
    __out_opt  PDWORD RequiredSize,
    __in       DWORD Flags
    );

// List all USB devices with some additional information
void ListDevices(CONST GUID* pClassGuid, LPCTSTR pszEnumerator)
{
    unsigned i, j;
    DWORD dwSize, dwPropertyRegDataType;
    DEVPROPTYPE ulPropertyType;
    CONFIGRET status;
    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA DeviceInfoData;
    const static LPCTSTR arPrefix[3] = { TEXT("VID_"), TEXT("PID_"), TEXT("MI_") };
    TCHAR szDeviceInstanceID[MAX_DEVICE_ID_LEN];
    TCHAR szDesc[1024], szHardwareIDs[4096];
    WCHAR szBuffer[4096];
    LPTSTR pszToken, pszNextToken;
    TCHAR szVid[MAX_DEVICE_ID_LEN], szPid[MAX_DEVICE_ID_LEN], szMi[MAX_DEVICE_ID_LEN];
  

    // List all connected USB devices
    hDevInfo = SetupDiGetClassDevs(pClassGuid, pszEnumerator, NULL, pClassGuid != NULL ? DIGCF_PRESENT : DIGCF_ALLCLASSES | DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE)
        return;


        DWORD requiredSize = 0;
    SP_DEVICE_INTERFACE_DATA			devInterfceData;


    i = 0;


    

    // Find the ones that are driverless
    for (i = 0; ; i++) 
    {
        DeviceInfoData.cbSize = sizeof(DeviceInfoData);
        if (!SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData))
            break;

        status = CM_Get_Device_ID(DeviceInfoData.DevInst, szDeviceInstanceID, MAX_PATH, 0);
        if (status != CR_SUCCESS)
            continue;
        // Display device instance ID
        _tprintf(TEXT("%s\n"), szDeviceInstanceID);

        if (SetupDiGetDeviceRegistryProperty(hDevInfo, &DeviceInfoData, SPDRP_DEVICEDESC,
            &dwPropertyRegDataType, (BYTE*)szDesc,
            sizeof(szDesc),   // The size, in bytes
            &dwSize))
            _tprintf(TEXT("    Device Description: \"%s\"\n"), szDesc);

        if (SetupDiGetDeviceRegistryProperty(hDevInfo, &DeviceInfoData, SPDRP_HARDWAREID,
            &dwPropertyRegDataType, (BYTE*)szHardwareIDs,
            sizeof(szHardwareIDs),    // The size, in bytes
            &dwSize)) {
            LPCTSTR pszId;
            _tprintf(TEXT("    Hardware IDs:\n"));
            for (pszId = szHardwareIDs;
                *pszId != TEXT('\0') && pszId + dwSize / sizeof(TCHAR) <= szHardwareIDs + ARRAYSIZE(szHardwareIDs);
                pszId += lstrlen(pszId) + 1) {

                _tprintf(TEXT("        \"%s\"\n"), pszId);
            }
        }
      
        // Retreive the device description as reported by the device itself
        // On Vista and earlier, we can use only SPDRP_DEVICEDESC
        // On Windows 7, the information we want ("Bus reported device description") is
        // accessed through DEVPKEY_Device_BusReportedDeviceDesc
        if (SetupDiGetDeviceProperty(hDevInfo, &DeviceInfoData, &DEVPKEY_Device_BusReportedDeviceDesc,
            &ulPropertyType, (BYTE*)szBuffer, sizeof(szBuffer), &dwSize, 0)) 
        {

            if (SetupDiGetDeviceProperty(hDevInfo, &DeviceInfoData, &DEVPKEY_Device_BusReportedDeviceDesc,
                &ulPropertyType, (BYTE*)szBuffer, sizeof(szBuffer), &dwSize, 0))
            {
                _tprintf(TEXT("    Bus Reported Device Description: \"%ls\"\n"), szBuffer);
            }

            if (SetupDiGetDeviceProperty(hDevInfo, &DeviceInfoData, &DEVPKEY_Device_Manufacturer,
                &ulPropertyType, (BYTE*)szBuffer, sizeof(szBuffer), &dwSize, 0)) 
            {
                _tprintf(TEXT("    Device Manufacturer: \"%ls\"\n"), szBuffer);
            }

            if (SetupDiGetDeviceProperty(hDevInfo, &DeviceInfoData, &DEVPKEY_Device_FriendlyName,
                &ulPropertyType, (BYTE*)szBuffer, sizeof(szBuffer), &dwSize, 0)) 
            {
                _tprintf(TEXT("    Device Friendly Name: \"%ls\"\n"), szBuffer);
            }

            if (SetupDiGetDeviceProperty(hDevInfo, &DeviceInfoData, &DEVPKEY_Device_LocationInfo,
                &ulPropertyType, (BYTE*)szBuffer, sizeof(szBuffer), &dwSize, 0)) 
            {
                _tprintf(TEXT("    Device Location Info: \"%ls\"\n"), szBuffer);
            }

            if (SetupDiGetDeviceProperty(hDevInfo, &DeviceInfoData, &DEVPKEY_Device_SecuritySDS,
                &ulPropertyType, (BYTE*)szBuffer, sizeof(szBuffer), &dwSize, 0)) 
            {
                // See Security Descriptor Definition Language on MSDN
                // (http://msdn.microsoft.com/en-us/library/windows/desktop/aa379567(v=vs.85).aspx)
                _tprintf(TEXT("    Device Security Descriptor String: \"%ls\"\n"), szBuffer);
            }

            if (SetupDiGetDeviceProperty(hDevInfo, &DeviceInfoData, &DEVPKEY_Device_ContainerId,
                &ulPropertyType, (BYTE*)szDesc, sizeof(szDesc), &dwSize, 0)) 
            {
                StringFromGUID2((REFGUID)szDesc, szBuffer, ARRAY_SIZE(szBuffer));
                _tprintf(TEXT("    ContainerId: \"%ls\"\n"), szBuffer);
            }

            if (SetupDiGetDeviceProperty(hDevInfo, &DeviceInfoData, &DEVPKEY_DeviceDisplay_Category,
                &ulPropertyType, (BYTE*)szBuffer, sizeof(szBuffer), &dwSize, 0))
            {
                _tprintf(TEXT("    Device Display Category: \"%ls\"\n"), szBuffer);
            }
        }

        pszToken = _tcstok_s(szDeviceInstanceID, TEXT("\\#&"), &pszNextToken);
        while (pszToken != NULL) 
        {
            szVid[0] = TEXT('\0');
            szPid[0] = TEXT('\0');
            szMi[0] = TEXT('\0');
            for (j = 0; j < 3; j++) 
            {
                if (_tcsncmp(pszToken, arPrefix[j], lstrlen(arPrefix[j])) == 0) 
                {
                    switch (j) 
                    {
                        case 0:
                            _tcscpy_s(szVid, ARRAY_SIZE(szVid), pszToken);
                            break;
                        case 1:
                            _tcscpy_s(szPid, ARRAY_SIZE(szPid), pszToken);
                            break;
                        case 2:
                            _tcscpy_s(szMi, ARRAY_SIZE(szMi), pszToken);
                            break;
                        default:
                            break;
                    }
                }
            }
            if (szVid[0] != TEXT('\0'))
                _tprintf(TEXT("    vid: \"%s\"\n"), szVid);
            if (szPid[0] != TEXT('\0'))
                _tprintf(TEXT("    pid: \"%s\"\n"), szPid);
            if (szMi[0] != TEXT('\0'))
                _tprintf(TEXT("    mi: \"%s\"\n"), szMi);
            pszToken = _tcstok_s(NULL, TEXT("\\#&"), &pszNextToken);
        }
    }

    return;
}

bool OpenUSBinterface() {

    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA DeviceInfoData;
    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
    struct _GUID GUID;
    HidD_GetHidGuid(&GUID);
    DWORD i;
    DWORD InterfaceNumber = 0;

    // Create a HDEVINFO with all present devices.
    hDevInfo = SetupDiGetClassDevs(&GUID,
        0, // Enumerator
        0,
        DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);

    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        // Insert error handling here.
        return 1;
    }

    // Enumerate through all devices in Set.

    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for (i = 0; SetupDiEnumDeviceInfo(hDevInfo, i,
        &DeviceInfoData); i++)
    {
        DWORD DataT;
        LPTSTR buffer = NULL;
        DWORD buffersize = 0;

        //
        // Call function with null to begin with,
        // then use the returned buffer size
        // to Alloc the buffer. Keep calling until
        // success or an unknown failure.
        //
        while (!SetupDiGetDeviceRegistryProperty(
            hDevInfo,
            &DeviceInfoData,
            SPDRP_DEVICEDESC,
            &DataT,
            (PBYTE)buffer,
            buffersize,
            &buffersize))
        {
            if (GetLastError() ==
                ERROR_INSUFFICIENT_BUFFER)
            {
                // Change the buffer size.
                if (buffer) LocalFree(buffer);
                buffer = (LPTSTR)LocalAlloc(LPTR, buffersize);
            }
            else
            {
                // Insert error handling here.
                break;
            }
        }

        _tprintf(TEXT("Device Number %i is: %s\n"), i, buffer);

        if (buffer) LocalFree(buffer);
    }

    if (GetLastError() != NO_ERROR &&
        GetLastError() != ERROR_NO_MORE_ITEMS)
    {
        // Insert error handling here.
        return false;
    }
    InterfaceNumber = 0;  // this just returns the first one, you can iterate on this
    DeviceInterfaceData.cbSize = sizeof(DeviceInterfaceData);
    if (SetupDiEnumDeviceInterfaces(
        hDevInfo,
        NULL,
        &GUID,
        InterfaceNumber,
        &DeviceInterfaceData))

        printf("\nGot interface");
    else
    {
        printf("\nNo interface");
        //ErrorExit((LPTSTR) "SetupDiEnumDeviceInterfaces");
        if (GetLastError() == ERROR_NO_MORE_ITEMS) printf(", since there are no more items found.");
        else printf(", unknown reason.");
    }






    //  Cleanup
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return true;
}

BOOL DevicePath(_In_ LPCGUID interfaceGuid, DWORD instance)
{
    BOOL result = FALSE;
    PSP_DEVICE_INTERFACE_DETAIL_DATA_W devInterfaceDetailData = NULL;
    HANDLE hHCDev = NULL;
    UCHAR buffer[65536];
    DWORD dwLen = 0;

    HDEVINFO hDevInfo = SetupDiGetClassDevsW(interfaceGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (hDevInfo == INVALID_HANDLE_VALUE)
        goto cleanup;

    SP_DEVICE_INTERFACE_DATA devInterfaceData;
    devInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    if (!SetupDiEnumDeviceInterfaces(hDevInfo, NULL, interfaceGuid, instance, &devInterfaceData))
        goto cleanup;

    // Call this with an empty buffer to the required size of the structure.
    ULONG requiredSize;
    SetupDiGetDeviceInterfaceDetailW(hDevInfo, &devInterfaceData, NULL, 0, &requiredSize, NULL);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        goto cleanup;

    devInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)malloc(requiredSize);
    if (!devInterfaceDetailData)
        goto cleanup;

    devInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
    if (!SetupDiGetDeviceInterfaceDetailW(hDevInfo, &devInterfaceData, devInterfaceDetailData, requiredSize, &requiredSize, NULL))
        goto cleanup;

    wprintf(L"%s\n", devInterfaceDetailData->DevicePath);
    result = TRUE;
    hHCDev = CreateFile(devInterfaceDetailData->DevicePath,
        GENERIC_WRITE,
        FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

   

    if (ReadFile(hHCDev, buffer, sizeof(buffer), &dwLen, NULL))
    {
        printf("%s", buffer);
    }
cleanup:
    if (hDevInfo != INVALID_HANDLE_VALUE)
        SetupDiDestroyDeviceInfoList(hDevInfo);
    free(devInterfaceDetailData);
    CloseHandle(hHCDev);
    return result;
}

int _tmain()
{
    GUID g = GUID_DEVINTERFACE_USB_DEVICE;
    DevicePath(&g,0);
    // List all connected USB devices
  /*  _tprintf(TEXT("---------------\n"));
    _tprintf(TEXT("- USB devices -\n"));
    _tprintf(TEXT("---------------\n"));
    ListDevices(NULL, TEXT("USB"));

    _tprintf(TEXT("\n"));
    _tprintf(TEXT("-------------------\n"));
    _tprintf(TEXT("- USBSTOR devices -\n"));
    _tprintf(TEXT("-------------------\n"));
    ListDevices(NULL, TEXT("USBSTOR"));

      _tprintf(TEXT("\n"));
    _tprintf(TEXT("--------------\n"));
    _tprintf(TEXT("- SD devices -\n"));
    _tprintf(TEXT("--------------\n"));
    ListDevices(NULL, TEXT("SD"));
    */
    _tprintf (TEXT("\n"));
    ListDevices(&GUID_DEVCLASS_USB, NULL);
    _tprintf (TEXT("\n"));

  //  _tprintf(TEXT("\n"));
  //  _tprintf(TEXT("-----------\n"));
   // _tprintf(TEXT("- Volumes -\n"));
  //  _tprintf(TEXT("-----------\n"));
    //ListDevices(NULL, TEXT("STORAGE\\VOLUME"));
    //_tprintf (TEXT("\n"));
   // ListDevices(&GUID_DEVCLASS_VOLUME, NULL);
/*
    _tprintf(TEXT("\n"));
    _tprintf(TEXT("----------------------------\n"));
    _tprintf(TEXT("- devices with disk drives -\n"));
    _tprintf(TEXT("----------------------------\n"));
    ListDevices(&GUID_DEVCLASS_DISKDRIVE, NULL);*/
    
    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
