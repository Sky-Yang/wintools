// WmiAdapter.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "wmi_adapter.h"
#include "system_information.h"
#include <Iphlpapi.h>
#include <Windows.h>
#include <iostream>
#include <ctime>


void GenerateFromHW()
{
    std::wcout << L"Hardware info:" << std::endl;
    std::wstring cpu_id = SystemInfo::GetInstance()->GetCPUID();
    std::wstring mac_id = SystemInfo::GetInstance()->GetMac(0);
    std::wstring disk_id = SystemInfo::GetInstance()->GetDiskSerialAdminMode(0);
    std::wcout << L"\tcpu_id: " << cpu_id << std::endl;
    std::wcout << L"\tmac_id: " << mac_id << std::endl;
    std::wcout << L"\tdisk_id: " << disk_id << std::endl;
}

int _tmain(int argc, _TCHAR* argv[])
{
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
        endTime = clock();
        std::cout << "run time: " << (double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << std::endl;
        std::wcout << std::endl;
        GenerateFromHW();
        system("pause");
        return 0;
    } while (false);

    std::wcout << L"Failed to get from wmi" << std::endl;
    std::wcout << std::endl;

    GenerateFromHW();
    std::wcout << std::endl;
    endTime = clock();
    std::cout << "run time: " << (double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << std::endl;
    system("pause");
    return 0;
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
    return 0;   // Program successfully completed.
}

