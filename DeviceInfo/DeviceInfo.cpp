// DisplayDevice.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <map>
#include <vector>
#include <string>

#include "machine_information.h"
#include "ssd_detector.h"

int _tmain(int argc, _TCHAR* argv[])
{
    printf("Pocessor Name: \n%ls\n", GetPocessorName().c_str());
    printf("\n");

    printf("Video Memory:\n");
    std::map<std::wstring, int> video_memory = GetVideoMemory();
    for (auto iter = video_memory.begin(); iter != video_memory.end(); ++iter)
    {
        printf("%ls(%dMB)\n", iter->first.c_str(), iter->second);
    }
    printf("\n");

    printf("SSD Geometry:\n");
    std::vector<std::wstring> ssd = GetSSDDriveString();
    for (auto iter = ssd.begin(); iter != ssd.end(); ++iter)
    {
        int size = 0;
        if (GetDriveGeometry(*iter, &size))
        {
            printf("%ls(%dGB)\n", iter->c_str(), size);
        }
    }
	return 0;
}

