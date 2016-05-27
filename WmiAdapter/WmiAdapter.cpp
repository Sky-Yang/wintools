// WmiAdapter.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "wmi_adapter.h"
#include <iostream>

int _tmain(int argc, _TCHAR* argv[])
{
    std::wstring wql = L"Select * from Win32_VideoController";
    std::vector<std::wstring> attrs;
    attrs.push_back(L"Caption");
    attrs.push_back(L"AdapterRAM");
    std::list<WmiAdapter::WmiResult> results;

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
            {
                std::wcout << attr->c_str() << L": " << _wtoi64(iter->AttrsString[attr->c_str()].c_str()) / 1000/1000/1000 << std::endl;                
            }
            else
                std::wcout << attr->c_str() << L": " << iter->AttrsString[attr->c_str()] << std::endl;
        }
    }
    system("pause");
    return 0;   // Program successfully completed.
}
