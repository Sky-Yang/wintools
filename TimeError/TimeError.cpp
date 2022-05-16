// TimeError.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <Windows.h>
#include <iostream>
#include <fstream>
#include <stdio.h>

std::string GetCurSystemTime(time_t tt)
{
    struct tm t;
    localtime_s(&t, &tt);
    char char_time[50] = {};
    sprintf_s(char_time, 50, "%04d-%02d-%02d %02d:%02d:%02d", t.tm_year + 1900,
            t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
    std::string str_system_time = static_cast<std::string>(char_time);
    return str_system_time;
}

inline __int64 _GetSysTickCount64()
{
    LARGE_INTEGER TicksPerSecond = { 0 }; 
    LARGE_INTEGER Tick; 
    if (!TicksPerSecond.QuadPart)
        QueryPerformanceFrequency(&TicksPerSecond); 
    QueryPerformanceCounter(&Tick); 
    __int64 Seconds = Tick.QuadPart / TicksPerSecond.QuadPart; 
    __int64 LeftPart = Tick.QuadPart - (TicksPerSecond.QuadPart * Seconds); 
    __int64 MillSeconds = LeftPart * 1000 / TicksPerSecond.QuadPart; 
    __int64 Ret = Seconds * 1000 + MillSeconds; 
    return Ret;
};

DWORD WINAPI proc(LPVOID laparameter)
{
    char* p = new char[1024 * 1024];
    p[0] = 1;
    if (p)
    {
        while (true)
        {
            for (int i = 1; i < 1024 * 1024; i++)
                p[i] = rand() % 127;
            if (p[0] == 0)
                break;
            Sleep(10);
        }
    }
    return 0;
}

void MakeCPUHot()
{
    for (int i = 0; i < 4; ++i)
    {
        HANDLE hthread1 = 0;
        hthread1 = CreateThread(NULL, 0, proc, NULL, 0, NULL);
        if (hthread1)
            CloseHandle(hthread1);
    }
}

int main()
{
    std::ofstream outfile;
    time_t begin_timestamp = time(0);
    std::string begin_time = GetCurSystemTime(begin_timestamp);
    __int64 begin_tick = GetTickCount64();
    std::cout << "begin time:" << begin_time <<  
        " timestamp:" << begin_timestamp << 
        " GetTickCount64:" << begin_tick << std::endl << std::endl;

    outfile.open("TimeError.log", std::ios::app);
    outfile << "begin time:" << begin_time <<
        " timestamp:" << begin_timestamp <<
        " GetTickCount64:" << begin_tick << std::endl << std::endl;
    outfile.close();

    MakeCPUHot();
    do 
    {
        outfile.open("TimeError.log", std::ios::app);
        time_t tt = time(0);
        std::string cur = GetCurSystemTime(tt);

        __int64 d0 = GetTickCount64();

        std::cout << cur << " timestamp:" << tt << std::endl;
        std::cout << "Running for " << (tt - begin_timestamp) << "seconds computed by local time" << std::endl;
        std::cout << "GetTickCount64()  " << d0 << std::endl;
        std::cout << "Running for " << (d0 - begin_tick) / 1000 << "seconds computed by GetTickCount64" << std::endl << std::endl;

        outfile << cur << " timestamp:" << tt << std::endl;
        outfile << "Running for " << (tt - begin_timestamp) << "seconds computed by local time" << std::endl;
        outfile << "GetTickCount64()  " << d0 << std::endl;
        outfile << "Running for " << (d0 - begin_tick) / 1000 << "seconds computed by GetTickCount64" << std::endl << std::endl;
        outfile.close();
        Sleep(2*60*1000);
    } while (true);
    return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
