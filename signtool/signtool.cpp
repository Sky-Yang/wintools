// signtool.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "windows.h"

// signtool sign /a /q /t http://timestamp.wosign.com/timestamp %1
int main(int argc, char *argv[])
{
    WIN32_FIND_DATAA  FindFileData;
    HANDLE hFind;
    printf("Target file is %s. \n", argv[argc-1]);
    hFind = FindFirstFileA(argv[argc-1], &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        printf("Invalid File Handle. Get Last Error reports %d ", GetLastError());
        return 1;
    }
    else
    {
        FindClose(hFind);
    }

    return  0;
}

