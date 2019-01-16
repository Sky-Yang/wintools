// PurgeCache.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <string>

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

typedef enum _SYSTEM_INFORMATION_CLASS
{
    SystemMemoryListInformation = 80, // q: SYSTEM_MEMORY_LIST_INFORMATION; s: SYSTEM_MEMORY_LIST_COMMAND (requires SeProfileSingleProcessPrivilege) // 80
};

typedef enum _SYSTEM_MEMORY_LIST_COMMAND
{
    MemoryCaptureAccessedBits,
    MemoryCaptureAndResetAccessedBits,
    MemoryEmptyWorkingSets,
    MemoryFlushModifiedList,
    MemoryPurgeStandbyList,
    MemoryPurgeLowPriorityStandbyList,
    MemoryCommandMax
} SYSTEM_MEMORY_LIST_COMMAND;

int _tmain(int argc, _TCHAR* argv[])
{
    if (argc == 1)
    {
        printf("Usage: PurgeCache.exe -w | -m | -s | -p\n");
        printf("    -w : empty working sets.\n");
        printf("    -m : flush modified page list.\n");
        printf("    -s : purge standby list.\n");
        printf("    -p : purge priority-0 standby list.\n");
        printf("\nError: please enter the right parameter.\n");
        return 1;
    }

    HANDLE hProcess = GetCurrentProcess();
    HANDLE hToken;
    if (!OpenProcessToken(hProcess, TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken))
    {
        fwprintf(stderr, L"Error: can't open current process token\n");
        return 1;
    }

    if (!GetPrivilege(hToken, "SeProfileSingleProcessPrivilege", 1))
    {
        fwprintf(stderr, L"Error: can't get SeProfileSingleProcessPrivilege\n");
        return 1;
    }

    CloseHandle(hToken);

    HMODULE ntdll = LoadLibrary(L"ntdll.dll");
    if (!ntdll)
    {
        fwprintf(stderr, L"Error: Can't load ntdll.dll, wrong Windows version?\n");
        return 1;
    }

    typedef DWORD NTSTATUS;
    NTSTATUS(WINAPI *NtSetSystemInformation)(INT, PVOID, ULONG) =
        (NTSTATUS(WINAPI *)(INT, PVOID, ULONG))GetProcAddress(ntdll, "NtSetSystemInformation");
    NTSTATUS(WINAPI *NtQuerySystemInformation)(INT, PVOID, ULONG, PULONG) =
        (NTSTATUS(WINAPI *)(INT, PVOID, ULONG, PULONG))GetProcAddress(ntdll, "NtQuerySystemInformation");
    if (!NtSetSystemInformation || !NtQuerySystemInformation)
    {
        fwprintf(stderr, L"Error: can't get function addresses, wrong Windows version?\n");
        return 1;
    }

    SYSTEM_MEMORY_LIST_COMMAND command = MemoryPurgeStandbyList;
    if (wcscmp(argv[1], L"-w") == 0)
        command = MemoryEmptyWorkingSets;
    else if (wcscmp(argv[1], L"-m") == 0)
        command = MemoryFlushModifiedList;
    else if (wcscmp(argv[1], L"-s") == 0)
        command = MemoryPurgeStandbyList;
    else if (wcscmp(argv[1], L"-p") == 0)
        command = MemoryPurgeLowPriorityStandbyList;
    else
    {
        printf("Usage: PurgeCache.exe -w | -m | -s | -p\n");
        printf("    -w : empty working sets.\n");
        printf("    -m : flush modified page list.\n");
        printf("    -s : purge standby list.\n");
        printf("    -p : purge priority-0 standby list.\n");
        printf("\nError: wrong parameter.\n");
        return 1;
    }

    NTSTATUS status = 0;
    SetCursor(LoadCursor(NULL, IDC_WAIT));
    status = NtSetSystemInformation(
        SystemMemoryListInformation,
        &command,
        sizeof(SYSTEM_MEMORY_LIST_COMMAND)
        );
    SetCursor(LoadCursor(NULL, IDC_ARROW));
    if (status == STATUS_PRIVILEGE_NOT_HELD)
    {
        fwprintf(stderr,
                 L"Error: insufficient privileges to execute the memory list command");
    }
    else if (!NT_SUCCESS(status))
    {
        fwprintf(stderr,
                 L"Error: unable to execute the memory list command %p", status);
    }
    if (NT_SUCCESS(status))
        wprintf(L"PurgeCache.exe %s succeed.\n", argv[1]);

    return 0;
}
