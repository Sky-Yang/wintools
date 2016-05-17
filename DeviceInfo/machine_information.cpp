#include "machine_information.h"

#include <memory>
#include <windows.h>
#include <WinIoCtl.h>
#include <Iphlpapi.h>
#include <Ntddscsi.h>
#include <devguid.h>    // for GUID_DEVCLASS_CDROM etc
#include <cfgmgr32.h>   // for MAX_DEVICE_ID_LEN, CM_Get_Parent and CM_Get_Device_ID
#include <SetupAPI.h>

#include "ssd_detector.h"

#pragma comment(lib, "Setupapi.lib")

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 255

std::vector<std::wstring> QueryVideoDevice(HKEY hKey)
{
    TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name 
    DWORD    cchClassName = MAX_PATH;  // size of class string 
    DWORD    cSubKeys = 0;         // number of subkeys 
    DWORD    cbMaxSubKey;          // longest subkey size 
    DWORD    cchMaxClass;          // longest class string 
    DWORD    cValues;              // number of values for key 
    DWORD    cchMaxValue;          // longest value name 
    DWORD    cbMaxValueData;       // longest value data 
    DWORD    cbSecurityDescriptor; // size of security descriptor 
    FILETIME ftLastWriteTime;      // last write time 

    DWORD i, retCode;

    TCHAR achValue[MAX_VALUE_NAME];
    DWORD cchValue = MAX_VALUE_NAME;

    // Get the class name and the value count. 
    retCode = RegQueryInfoKey(
        hKey,                    // key handle 
        achClass,                // buffer for class name 
        &cchClassName,           // size of class string 
        NULL,                    // reserved 
        &cSubKeys,               // number of subkeys 
        &cbMaxSubKey,            // longest subkey size 
        &cchMaxClass,            // longest class string 
        &cValues,                // number of values for this key 
        &cchMaxValue,            // longest value name 
        &cbMaxValueData,         // longest value data 
        &cbSecurityDescriptor,   // security descriptor 
        &ftLastWriteTime);       // last write time 

    // Enumerate the key values. 
    std::vector<std::wstring> device_locates;
    if (cValues)
    {
        const std::wstring video_device(L"\\Device\\Video");
        const int len = video_device.length();
        TCHAR value[MAX_VALUE_NAME] = { 0 };
        DWORD type = 0;
        for (i = 0, retCode = ERROR_SUCCESS; i < cValues; i++)
        {
            cchValue = MAX_VALUE_NAME;
            achValue[0] = L'\0';
            retCode = RegEnumValue(hKey, i,
                                    achValue,
                                    &cchValue,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL);

            if (retCode == ERROR_SUCCESS)
            {
                if (video_device.compare(0, len, achValue, len) == 0)
                {
                    cchValue = MAX_VALUE_NAME;
                    type = 0;
                    ZeroMemory(value, MAX_VALUE_NAME);
                    //查询key下的字段为\Device\VideoX的子键，值保存到value  
                    LSTATUS lResult = RegQueryValueEx(hKey, achValue, 0, &type,
                                                        (LPBYTE)value, &cchValue);
                    if (lResult == ERROR_SUCCESS && REG_SZ == type)
                    {
                        if (wcslen(value) > 18) // 删除 "\Registry\Machine\"
                            device_locates.push_back(std::wstring(&(value[18])));
                    }
                }
            }
        }
    }

    return device_locates;
}

// enum all devices with some additional information
std::vector<std::wstring> GetVideoDevices(CONST GUID *pClassGuid,
                                            LPCTSTR pszEnumerator)
{
    std::vector<std::wstring> videos;
    // List all connected devices
    HDEVINFO hDevInfo = SetupDiGetClassDevs(
        pClassGuid, pszEnumerator, NULL,
        pClassGuid != NULL ? DIGCF_PRESENT : DIGCF_ALLCLASSES | DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE)
        return videos;

    DWORD dwSize, dwPropertyRegDataType;
    SP_DEVINFO_DATA DeviceInfoData;
    TCHAR szDesc[1024], szClass[1024];
    // Find the ones that are driverless
    for (unsigned i = 0;; i++)
    {
        ZeroMemory(szDesc, 1024);
        ZeroMemory(szClass, 1024);
        DeviceInfoData.cbSize = sizeof(DeviceInfoData);
        if (!SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData))
            break;

        if (SetupDiGetDeviceRegistryProperty(hDevInfo, &DeviceInfoData,
            SPDRP_CLASS, &dwPropertyRegDataType,
            (BYTE*)szClass, sizeof(szClass),
            &dwSize))
        {
            if (wcscmp(szClass, L"Display") != 0)
                continue;
        }

        if (SetupDiGetDeviceRegistryProperty(hDevInfo, &DeviceInfoData,
            SPDRP_DEVICEDESC,
            &dwPropertyRegDataType,
            (BYTE*)szDesc, sizeof(szDesc),
            &dwSize))
        {
            videos.push_back(szDesc);
        }
    }

    return videos;
}

int RegBinaryToInt(BYTE* bytes, int size)
{
    int addr = 0;
    for (int i = size - 1; i >= 0; --i)
    {
        addr |= (bytes[i] & 0xFF) << (8 * (size - i - 1));
    }
    return addr;
}

std::map<std::wstring, int> GetVideoMemory()
{
    std::vector<std::wstring>&& video_cards =
        GetVideoDevices(&GUID_DEVCLASS_DISPLAY, NULL);
    std::map<std::wstring, int> video_memory;

    if (video_cards.size() == 0)
        return video_memory;

    for (auto card = video_cards.begin(); card != video_cards.end(); ++card)
    {
        video_memory[*card] = 0;
    }

    HKEY key;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"HARDWARE\\DEVICEMAP\\VIDEO", 0,
        KEY_READ, &key) != ERROR_SUCCESS)
    {
        return video_memory;
    }

    std::vector<std::wstring>&& device_locates = QueryVideoDevice(key);
    RegCloseKey(key);

    DWORD size = MAX_VALUE_NAME;
    TCHAR desc[MAX_VALUE_NAME] = { 0 };
    DWORD type = 0;
    for (auto iter = device_locates.begin(); iter != device_locates.end();
         ++iter)
    {
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, iter->c_str(), 0, KEY_READ,
            &key) != ERROR_SUCCESS)
        {
            continue;
        }

        size = MAX_VALUE_NAME;
        ZeroMemory(desc, MAX_VALUE_NAME);
        type = 0;
        //查询key下的字段为DriverDesc的子键，值保存到value  
        LSTATUS lResult = RegQueryValueEx(key, L"DriverDesc", NULL, &type,
                                          (LPBYTE)desc, &size);
        if (lResult != ERROR_SUCCESS)
        {
            lResult = RegQueryValueEx(key, L"Device Description", NULL, &type,
                                      (LPBYTE)desc, &size);
            if (lResult != ERROR_SUCCESS)
            {
                RegCloseKey(key);
                continue;
            }
        }

        if (REG_SZ != type)
        {
            RegCloseKey(key);
            continue;
        }

        DWORD memory_size = 0;
        size = 4;
        for (auto card = video_cards.begin(); card != video_cards.end(); ++card)
        {
            memory_size = 0;
            size = 4;
            if (card->compare(desc) == 0)
            {
                lResult = RegQueryValueEx(key, L"HardwareInformation.MemorySize",
                                          NULL, &type, 
                                          reinterpret_cast<LPBYTE>(&memory_size),
                                          &size);
                if (lResult == ERROR_SUCCESS)
                {
                    switch (type)
                    {
                        case REG_DWORD:
                            video_memory[*card] = static_cast<int>(memory_size / 1024 / 1024); // in MB
                            break;
                        case REG_BINARY:
                            video_memory[*card] = static_cast<int>(RegBinaryToInt(reinterpret_cast<LPBYTE>(&memory_size), size)); // in MB
                            break;
                        default:
                            video_memory[*card] = 0;
                            break;
                    }
                }
            }
        }

        RegCloseKey(key);
    }

    return video_memory;
}

bool GetDriveGeometry(const std::wstring& drive_string, int *size)
{
    HANDLE hDevice;               // handle to the drive to be examined 
    BOOL bResult;                 // results flag
    DWORD junk;                   // discard results

    hDevice = CreateFile(drive_string.c_str(),  // drive 
                            0,                // no access to the drive
                            FILE_SHARE_READ | // share mode
                            FILE_SHARE_WRITE,
                            NULL,             // default security attributes
                            OPEN_EXISTING,    // disposition
                            0,                // file attributes
                            NULL);            // do not copy file attributes

    if (hDevice == INVALID_HANDLE_VALUE) // cannot open the drive
    {
        return false;
    }

    DISK_GEOMETRY disk_geometry;
    bResult = DeviceIoControl(hDevice,  // device to be queried
                                IOCTL_DISK_GET_DRIVE_GEOMETRY,  // operation to perform
                                NULL, 0, // no input buffer
                                &disk_geometry, sizeof(disk_geometry),  // output buffer
                                &junk,                 // # bytes returned
                                (LPOVERLAPPED)NULL);  // synchronous I/O

    CloseHandle(hDevice);

    *size = static_cast<int>(disk_geometry.Cylinders.QuadPart * 
                            (ULONG)disk_geometry.TracksPerCylinder *
                            (ULONG)disk_geometry.SectorsPerTrack * 
                            (ULONG)disk_geometry.BytesPerSector / 
                            (1000 * 1000 * 1000));
    //printf("Disk size = %I64d (Bytes) = %I64d (Gb)\n", size,
    //       size / (1024 * 1024 * 1024));

    return bResult == TRUE;
}

std::vector<std::wstring> GetSSDDriveString()
{
    DWORD len = GetLogicalDriveStrings(0, nullptr);
    std::wstring drive_string(L"");
    drive_string.resize(len+1);
    len = GetLogicalDriveStrings(len, &drive_string.front());

    bool found = false;
    std::vector<std::wstring> drives;
    std::wstring name(L"");
    for (int i = 0; i < static_cast<int>(len); i++)
    {
        if (drive_string[i] == L'\0')
        {
            continue;
        }
        else
        {
            name = drive_string[i];
            name += L":";
            i += 2;
        }

        switch (GetDriveType(name.c_str()))
        {
            case DRIVE_FIXED:
            case DRIVE_REMOVABLE:
                break;
            default:
                continue;
                break;
        }

        name = GetPhysicalDriveString(name);
        if (!DetectsSSD(name))
            continue;

        drives.push_back(name);
        name = L"";
    }

    return drives;
}

std::wstring GetPocessorName()
{
    std::wstring processorName_(L"");
    HKEY key;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0,
        KEY_READ, &key) != ERROR_SUCCESS)
    {
        return processorName_;
    }

    processorName_ = L"Unknown";

    // get CPU name
    DWORD size = MAX_VALUE_NAME;
    TCHAR desc[MAX_VALUE_NAME] = { 0 };
    DWORD type = 0;
    if (ERROR_SUCCESS == RegQueryValueEx(key, L"ProcessorNameString", NULL, &type,
        (LPBYTE)desc, &size))
    {
        processorName_ = desc;
    }

    return processorName_;
}
