// WmiAdapter.cpp : 定义控制台应用程序的入口点。
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

    wql = L"Select * from Win32_DesktopMonitor";
    attrs.clear();
    attrs.push_back(L"ScreenWidth");
    attrs.push_back(L"ScreenHeight");
    attrs.push_back(L"PixelsPerXLogicalInch");
    attrs.push_back(L"PixelsPerYLogicalInch");
    results.clear();

    if (!WmiAdapter::Query(wql, attrs, &results))
    {
        return 1;
    }

    std::wcout << std::endl;
    std::wcout << L"Displayer:" << std::endl;
    for (auto iter = results.begin(); iter != results.end(); ++iter)
    {
        for (auto attr = attrs.begin(); attr != attrs.end(); ++attr)
        {
            std::wcout << attr->c_str() << L": " << iter->AttrsInt[attr->c_str()] << std::endl;
        }
    }
    system("pause");
    return 0;   // Program successfully completed.
}

