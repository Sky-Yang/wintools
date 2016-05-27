// DisplayDevice.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#define INITGUID
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <map>
#include <vector>
#include <set>
#include <string>
#include <iostream>
#include <cassert>
//#include <DXGI.h>  

#include "machine_information.h"
#include "ssd_detector.h"

//#pragma comment(lib, "dxgi.lib")

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define STATUS_PRIVILEGE_NOT_HELD ((NTSTATUS)0xC0000061L)

BOOL GetPrivilege(HANDLE TokenHandle, LPCSTR lpName, int flags)
{
    BOOL bResult;
    DWORD dwBufferLength;
    LUID luid;
    TOKEN_PRIVILEGES tpPreviousState;
    TOKEN_PRIVILEGES tpNewState;

    dwBufferLength = 16;
    bResult = LookupPrivilegeValueA(0, lpName, &luid);
    if (bResult)
    {
        tpNewState.PrivilegeCount = 1;
        tpNewState.Privileges[0].Luid = luid;
        tpNewState.Privileges[0].Attributes = 0;
        bResult = AdjustTokenPrivileges(
            TokenHandle, 0, &tpNewState,
            (LPBYTE)&(tpNewState.Privileges[1]) - (LPBYTE)&tpNewState,
            &tpPreviousState, &dwBufferLength);
        if (bResult)
        {
            tpPreviousState.PrivilegeCount = 1;
            tpPreviousState.Privileges[0].Luid = luid;
            tpPreviousState.Privileges[0].Attributes = flags != 0 ? 2 : 0;
            bResult = AdjustTokenPrivileges(TokenHandle, 0, &tpPreviousState,
                                            dwBufferLength, 0, 0);
        }
    }
    return bResult;
}

int _tmain(int argc, _TCHAR* argv[])
{
    //HANDLE hProcess = GetCurrentProcess();
    //HANDLE hToken;
    //if (!OpenProcessToken(hProcess, TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken))
    //{
    //    fwprintf(stderr, L"Can't open current process token\n");
    //    return 1;
    //}

    //if (!GetPrivilege(hToken, "SeProfileSingleProcessPrivilege", 1))
    //{
    //    fwprintf(stderr, L"Can't get SeProfileSingleProcessPrivilege\n");
    //    return 1;
    //}

    //CloseHandle(hToken);


    ////if (HasWDDMDriver())
    //{
    //    IDXGIFactory * pFactory;
    //    IDXGIAdapter * pAdapter;
    //    std::vector <IDXGIAdapter*> vAdapters;            // 显卡  
    //    // 显卡的数量  
    //    int iAdapterNum = 0;
    //    // 创建一个DXGI工厂  
    //    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&pFactory));

    //    if (FAILED(hr))
    //        return -1;

    //    // 枚举适配器  
    //    while (pFactory->EnumAdapters(iAdapterNum, &pAdapter) != DXGI_ERROR_NOT_FOUND)
    //    {
    //        vAdapters.push_back(pAdapter);
    //        ++iAdapterNum;
    //    }
    //    // 信息输出   
    //    cout << "===============获取到" << iAdapterNum << "块显卡===============" << endl;
    //    for (size_t i = 0; i < vAdapters.size(); i++)
    //    {
    //        // 获取信息  
    //        DXGI_ADAPTER_DESC adapterDesc;
    //        vAdapters[i]->GetDesc(&adapterDesc);

    //        // 输出显卡信息  
    //        cout << "系统视频内存:" << adapterDesc.DedicatedSystemMemory / 1024 / 1024 << "M" << endl;
    //        cout << "专用视频内存:" << adapterDesc.DedicatedVideoMemory / 1024 / 1024 << "M" << endl;
    //        cout << "共享系统内存:" << adapterDesc.SharedSystemMemory / 1024 / 1024 << "M" << endl;
    //        cout << "设备描述:" << adapterDesc.Description << endl;
    //        cout << "设备ID:" << adapterDesc.DeviceId << endl;
    //        cout << "PCI ID修正版本:" << adapterDesc.Revision << endl;
    //        cout << "子系统PIC ID:" << adapterDesc.SubSysId << endl;
    //        cout << "厂商编号:" << adapterDesc.VendorId << endl;

    //        // 输出设备  
    //        IDXGIOutput * pOutput;
    //        // 输出设备数量  
    //        int iOutputNum = 0;
    //        while (vAdapters[i]->EnumOutputs(iOutputNum, &pOutput) != DXGI_ERROR_NOT_FOUND)
    //        {
    //            iOutputNum++;
    //        }

    //        cout << "-----------------------------------------" << endl;
    //        cout << "获取到" << iOutputNum << "个显示设备:" << endl;
    //        cout << endl;
    //    }
    //    vAdapters.clear();
    //}

    //printf("Pocessor Name: \n%ls\n", GetPocessorName().c_str());
    //printf("\n");

    printf("Video Memory:\n");
    std::map<std::wstring, int> video_memory = GetVideoMemory();
    for (auto iter = video_memory.begin(); iter != video_memory.end(); ++iter)
    {
        if (iter->second > 0)
            printf("%ls (%dMB)\n", iter->first.c_str(), iter->second);
        else
            printf("%ls\n", iter->first.c_str());
    }
    printf("\n");

    printf("SSD Geometry:\n");
    std::set<std::wstring> ssd = GetSSDDriveString();
    for (auto iter = ssd.begin(); iter != ssd.end(); ++iter)
    {
        int size = 0;
        if (GetDriveGeometry(*iter, &size))
        {
            printf("%ls (%dGB)\n", iter->c_str(), size);
        }
    }
    system("pause");
	return 0;
}

