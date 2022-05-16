// WmiAdapter.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "wmi_adapter.h"
#include "system_information.h"
#include <Iphlpapi.h>
#include <Windows.h>
#include <iostream>
#include <ctime>



std::wstring GenerateFromHW()
{
    std::wstring cpu_id = SystemInfo::GetInstance()->GetCPUID();
    std::wstring disk_id = SystemInfo::GetInstance()->GetDiskSerialAdminMode(0);
    std::wstring mac_id = SystemInfo::GetInstance()->GetMac(0);
    std::wstring macs = SystemInfo::GetInstance()->GetLocalMacs();
    std::wcout << L"cpu_id:" << cpu_id << std::endl;
    std::wcout << L"disk_id:" << disk_id << std::endl;
    std::wcout << L"mac_id:" << mac_id << std::endl;
    std::wcout << L"macs:" << macs << std::endl;
    return cpu_id + disk_id + mac_id;
}

int _tmain(int argc, _TCHAR* argv[])
{
    std::wstring hw_guid = GenerateFromHW();
    TCHAR *szSubKey = _T("SOFTWARE\\Microsoft\\Cryptography");
    HKEY hKey = nullptr;
    std::wstring guid_reg;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szSubKey, 0, KEY_READ|KEY_WOW64_64KEY, &hKey))
    {
        DWORD dwType = REG_SZ;
        TCHAR szBuffer[1024] = { 0 };
        DWORD dwSize = sizeof(szBuffer);
        auto status = RegQueryValueEx(hKey, _T("MachineGuid"), 0, &dwType, reinterpret_cast<LPBYTE>(szBuffer), &dwSize);
        if (ERROR_SUCCESS == status)
        {
            guid_reg = szBuffer;
            std::wcout << L"guid_reg:" << guid_reg << std::endl;
        }
        if (hKey != nullptr)
        {
            RegCloseKey(hKey);
        }
    }

    clock_t startTime, endTime;
    startTime = clock();
    std::wstring wql = L"Select * from Win32_ComputerSystemProduct";
    std::vector<std::wstring> attrs;
    //attrs.push_back(L"Caption");
    attrs.push_back(L"UUID");
    std::list<WmiAdapter::WmiResult> results;

    do 
    {
        if (!WmiAdapter::Query(wql, attrs, &results))
            break;

        std::wcout << L"Win32_ComputerSystemProduct:" << std::endl;
        for (auto iter = results.begin(); iter != results.end(); ++iter)
        {
            for (auto attr = attrs.begin(); attr != attrs.end(); ++attr)
            {
                std::wcout << L"\t" << attr->c_str() << L": " << iter->AttrsString[*attr].c_str() << std::endl;
            }
        }
    } while (false);

    attrs.clear();
    results.clear();
    return 0;

    wql = L"Select * from Win32_UserAccount";
    attrs.push_back(L"AccountType");
    attrs.push_back(L"Caption");
    attrs.push_back(L"Disabled");
    attrs.push_back(L"InstallDate");
    attrs.push_back(L"LocalAccount");
    attrs.push_back(L"Lockout");
    attrs.push_back(L"Status");

    do
    {
        if (!WmiAdapter::Query(wql, attrs, &results))
            break;

        std::wcout << L"Win32_UserAccount:" << std::endl;
        for (auto iter = results.begin(); iter != results.end(); ++iter)
        {
            for (auto attr = attrs.begin(); attr != attrs.end(); ++attr)
            {
                std::wcout << L"\t" << attr->c_str() << L": " << iter->AttrsString[*attr].c_str() << std::endl;
            }
        }
    } while (false);

    attrs.clear();
    results.clear();

    wql = L"Select * from Win32_GroupUser";
    attrs.push_back(L"GroupComponent");
    attrs.push_back(L"PartComponent");

    do
    {
        if (!WmiAdapter::Query(wql, attrs, &results))
            break;

        std::wcout << L"Win32_GroupUser:" << std::endl;
        for (auto iter = results.begin(); iter != results.end(); ++iter)
        {
            for (auto attr = attrs.begin(); attr != attrs.end(); ++attr)
            {
                std::wcout << L"\t" << attr->c_str() << L": " << iter->AttrsString[*attr].c_str() << std::endl;
            }
        }
    } while (false);

    attrs.clear();
    results.clear();

    wql = L"Select * from Win32_VideoController";
    attrs.push_back(L"Caption");
    attrs.push_back(L"AdapterRAM");

    if (!WmiAdapter::Query(wql, attrs, &results))
    {
        return 1;
    }

    std::wcout << L"Video Card:" << std::endl;
    for (auto iter = results.begin(); iter != results.end(); ++iter)
    {
        for (auto attr = attrs.begin(); attr != attrs.end(); ++attr)
        {
            if (L"AdapterRAM" == *attr)
            {
                std::wcout << L"AdapterRAM: " << iter->AttrsInt[L"AdapterRAM"] / 1024 / 1024 << L"MB" << std::endl;
            }
            else
            {
                std::wcout << attr->c_str() << L": " << iter->AttrsString[attr->c_str()] << std::endl;
            }
        }
    }

    wql = L"Select * from Win32_DiskDrive";
    attrs.clear();
    attrs.push_back(L"Caption");
    attrs.push_back(L"Size");
    results.clear();

    if (!WmiAdapter::Query(wql, attrs, &results))
    {
        return 1;
    }

    std::wcout << std::endl;
    std::wcout << L"Win32_DiskDrive:" << std::endl;
    for (auto iter = results.begin(); iter != results.end(); ++iter)
    {
        for (auto attr = attrs.begin(); attr != attrs.end(); ++attr)
        {
            if (L"Size" == *attr)
                std::wcout << attr->c_str() << L": " << _wtoi64(iter->AttrsString[attr->c_str()].c_str()) / 1000 / 1000 / 1000 << std::endl;
            else
                std::wcout << attr->c_str() << L": " << iter->AttrsString[attr->c_str()] << std::endl;
        }
    }

    wql = L"Select * from Win32_PerfFormattedData_PerfDisk_LogicalDisk";
    attrs.clear();
    attrs.push_back(L"Caption");
    attrs.push_back(L"AvgDiskQueueLength");
    results.clear();

    if (!WmiAdapter::Query(wql, attrs, &results))
    {
        return 1;
    }

    std::wcout << std::endl;
    std::wcout << L"Win32_PerfFormattedData_PerfDisk_LogicalDisk:" << std::endl;
    for (auto iter = results.begin(); iter != results.end(); ++iter)
    {
        for (auto attr = attrs.begin(); attr != attrs.end(); ++attr)
        {
            if (L"Size" == *attr)
                std::wcout << attr->c_str() << L": " << _wtoi64(iter->AttrsString[attr->c_str()].c_str()) / 1000 / 1000 / 1000 << std::endl;
            else
                std::wcout << attr->c_str() << L": " << iter->AttrsString[attr->c_str()] << std::endl;
        }
    }
    system("pause");
    endTime = clock();
    std::cout << "run time: " << (double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << std::endl;
    std::wcout << std::endl;
    return 0;   // Program successfully completed.
}

