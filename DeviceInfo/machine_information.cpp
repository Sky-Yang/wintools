#include "machine_information.h"

#define INITGUID
#include <memory>
#include <windows.h>
#include <Iphlpapi.h>
#include <Ntddscsi.h>
#include <devguid.h>    // for GUID_DEVCLASS_CDROM etc
#include <cfgmgr32.h>   // for MAX_DEVICE_ID_LEN, CM_Get_Parent and CM_Get_Device_ID
#include <SetupAPI.h>
#include <d3d9.h>
#include <ddraw.h>
#include <cassert>

#include "atlconv.h"

#include "ssd_detector.h"

#pragma comment(lib, "Setupapi.lib")

namespace
{

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 255

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=nullptr; } }
#endif

typedef HRESULT(WINAPI* LPDIRECTDRAWCREATE)(GUID FAR *lpGUID, LPDIRECTDRAW FAR *lplpDD, IUnknown FAR *pUnkOuter);

struct DDRAW_MATCH
{
    GUID guid;
    HMONITOR hMonitor;
    CHAR strDriverName[512];
    bool bFound;
};

//-----------------------------------------------------------------------------
BOOL WINAPI DDEnumCallbackEx(GUID FAR* lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext, HMONITOR hm)
{
    UNREFERENCED_PARAMETER(lpDriverDescription);

    DDRAW_MATCH* pDDMatch = (DDRAW_MATCH*)lpContext;
    if (pDDMatch->hMonitor == hm)
    {
        pDDMatch->bFound = true;
        strcpy_s(pDDMatch->strDriverName, 512, lpDriverName);
        memcpy(&pDDMatch->guid, lpGUID, sizeof(GUID));
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
HRESULT GetVideoMemoryViaDirectDraw(HMONITOR hMonitor, DWORD* pdwAvailableVidMem)
{
    LPDIRECTDRAW pDDraw = nullptr;
    LPDIRECTDRAWENUMERATEEXA pDirectDrawEnumerateEx = nullptr;
    HRESULT hr;
    bool bGotMemory = false;
    *pdwAvailableVidMem = 0;

    HINSTANCE hInstDDraw;
    LPDIRECTDRAWCREATE pDDCreate = nullptr;

    hInstDDraw = LoadLibrary(L"ddraw.dll");
    if (hInstDDraw)
    {
        DDRAW_MATCH match;
        ZeroMemory(&match, sizeof(DDRAW_MATCH));
        match.hMonitor = hMonitor;

        pDirectDrawEnumerateEx = (LPDIRECTDRAWENUMERATEEXA)GetProcAddress(hInstDDraw, "DirectDrawEnumerateExA");

        if (pDirectDrawEnumerateEx)
        {
            hr = pDirectDrawEnumerateEx(DDEnumCallbackEx, (VOID*)&match, DDENUM_ATTACHEDSECONDARYDEVICES);
        }

        pDDCreate = (LPDIRECTDRAWCREATE)GetProcAddress(hInstDDraw, "DirectDrawCreate");
        if (pDDCreate)
        {
            pDDCreate(&match.guid, &pDDraw, nullptr);

            LPDIRECTDRAW7 pDDraw7;
            if (SUCCEEDED(pDDraw->QueryInterface(IID_IDirectDraw7, (VOID**)&pDDraw7)))
            {
                DDSCAPS2 ddscaps;
                ZeroMemory(&ddscaps, sizeof(DDSCAPS2));
                ddscaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM;
                hr = pDDraw7->GetAvailableVidMem(&ddscaps, pdwAvailableVidMem, nullptr);
                if (SUCCEEDED(hr))
                    bGotMemory = true;
                pDDraw7->Release();
            }
        }
        FreeLibrary(hInstDDraw);
    }


    if (bGotMemory)
        return S_OK;
    else
        return E_FAIL;
}

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
            retCode = RegEnumValue(hKey, i, achValue, &cchValue, NULL, NULL,
                                   NULL, NULL);

            if (retCode == ERROR_SUCCESS)
            {
                if (video_device.compare(0, len, achValue, len) == 0)
                {
                    cchValue = MAX_VALUE_NAME;
                    type = 0;
                    ZeroMemory(value, MAX_VALUE_NAME);
                    //查询key下的字段为\Device\VideoX的子键，值保存到value  
                    LSTATUS lResult = RegQueryValueEx(
                        hKey, achValue, 0, &type, 
                        reinterpret_cast<LPBYTE>(value), &cchValue);
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
            reinterpret_cast<PBYTE>(szClass), sizeof(szClass),
            &dwSize))
        {
            if (wcscmp(szClass, L"Display") != 0)
                continue;
        }

        if (SetupDiGetDeviceRegistryProperty(hDevInfo, &DeviceInfoData,
            SPDRP_DEVICEDESC,
            &dwPropertyRegDataType,
            reinterpret_cast<PBYTE>(szDesc), sizeof(szDesc),
            &dwSize))
        {
            videos.push_back(szDesc);
        }
    }

    return videos;
}

int GetVideoMemoryViaRegistry(
    const std::vector<std::wstring>& video_cards,
    std::map<std::wstring, int>* video_memory)
{
    int card_count = 0;
    HKEY key;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"HARDWARE\\DEVICEMAP\\VIDEO", 0,
        KEY_READ, &key) != ERROR_SUCCESS)
    {
        return card_count;
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
                                          reinterpret_cast<LPBYTE>(desc), &size);
        if (lResult != ERROR_SUCCESS)
        {
            lResult = RegQueryValueEx(key, L"Device Description", NULL, &type,
                                      reinterpret_cast<LPBYTE>(desc), &size);
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
                if (lResult != ERROR_SUCCESS)
                    continue;

                switch (type)
                {
                    case REG_QWORD:
                    case REG_DWORD:
                        ++card_count;
                        if (0 == video_memory->at(*card))
                            (*video_memory)[*card] = memory_size / 1024 / 1024; // in MB

                        break;
                    case REG_BINARY: // 不同显卡情况太多不处理（默认0）
                    default:
                        break;
                }
            }
        }

        RegCloseKey(key);
    }

    return card_count;
}

int GetVideoMemoryViaDirectDraw(
    const std::vector<std::wstring>& video_cards,
    std::map<std::wstring, int>* video_memory)
{
    int card_count = 0;

    IDirect3D9* pD3D9 = nullptr;
    pD3D9 = Direct3DCreate9(D3D_SDK_VERSION);
    if (!pD3D9)
        return card_count;

    UINT dwAdapterCount = pD3D9->GetAdapterCount();
    D3DADAPTER_IDENTIFIER9 id;
    ZeroMemory(&id, sizeof(D3DADAPTER_IDENTIFIER9));
    pD3D9->GetAdapterIdentifier(0, 0, &id);

    USES_CONVERSION;
    WCHAR* desc = A2W(id.Description);
    if (video_memory->find(desc) ==
        video_memory->end() || video_memory->at(desc) != 0)
    {
        assert(false && L"can not find video card by nDescription");
        SAFE_RELEASE(pD3D9);
        return card_count;
    }

    HMONITOR hMonitor = pD3D9->GetAdapterMonitor(0);

    DWORD dwAvailableVidMem;
    if (SUCCEEDED(GetVideoMemoryViaDirectDraw(hMonitor, &dwAvailableVidMem)))
    {
        ++card_count;
        (*video_memory)[desc] = dwAvailableVidMem / 1024 / 1024;
    }
    SAFE_RELEASE(pD3D9);

    return card_count;
}

}   // namespace

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

    // GetVideoMemoryViaDirectDraw 只能获取当前显卡信息，多显卡时再从注册表获取
    if (video_cards.size() !=
        GetVideoMemoryViaDirectDraw(video_cards, &video_memory))
    {
        GetVideoMemoryViaRegistry(video_cards, &video_memory);
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

    return bResult == TRUE;
}
