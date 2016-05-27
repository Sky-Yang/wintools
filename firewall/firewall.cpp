// firewall.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include <windows.h>
#include <Netfw.h>

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

BOOL AddFirewallException(TCHAR *szCaption, TCHAR *szAppPath)
{
    INetFwMgr *pNetFwMgr = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_NetFwMgr, nullptr, CLSCTX_SERVER, IID_INetFwMgr, (void**)&pNetFwMgr);
    if (FAILED(hr) || (pNetFwMgr == nullptr))
    {
        return FALSE;
    }

    INetFwPolicy *pNetFwPolicy = nullptr;
    hr = pNetFwMgr->get_LocalPolicy(&pNetFwPolicy);
    if (FAILED(hr) || (pNetFwPolicy == nullptr))
    {
        pNetFwMgr->Release();
        return FALSE;
    }

    // 添加APP防火墙为允许（lambda函数）
    auto AddIt = [szCaption, szAppPath](INetFwProfile *pNetFwProfile) -> BOOL
    {
        BOOL result = FALSE;

        INetFwAuthorizedApplication *pNewFwApp = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_NetFwAuthorizedApplication, nullptr, CLSCTX_SERVER, IID_INetFwAuthorizedApplication, (void**)&pNewFwApp);
        if (SUCCEEDED(hr) && (pNetFwProfile != nullptr))
        {
            pNewFwApp->put_Name(szCaption);
            pNewFwApp->put_ProcessImageFileName(szAppPath);
            pNewFwApp->put_Scope(NET_FW_SCOPE_ALL);
            pNewFwApp->put_IpVersion(NET_FW_IP_VERSION_ANY);

            // 不知道为什么，加了这一句能添加但是那个勾没打上
            //pNewFwApp->put_Enabled(_variant_t(TRUE));

            INetFwAuthorizedApplications *pNetFwAuthorizedApps = nullptr;
            hr = pNetFwProfile->get_AuthorizedApplications(&pNetFwAuthorizedApps);
            if (SUCCEEDED(hr))
            {
                result = SUCCEEDED(pNetFwAuthorizedApps->Add(pNewFwApp));
                pNetFwAuthorizedApps->Release();
                pNetFwAuthorizedApps = nullptr;
            }

            pNewFwApp->Release();
            pNewFwApp = nullptr;
        }

        return result;
    };

    INetFwProfile *pNetFwCurrentProfile = nullptr;
    hr = pNetFwPolicy->get_CurrentProfile(&pNetFwCurrentProfile);
    if (FAILED(hr) || (pNetFwCurrentProfile == nullptr))
    {
        pNetFwPolicy->Release();
        pNetFwMgr->Release();
        return FALSE;
    }

    // 在当前Profile下添加（公用）
    BOOL result1 = AddIt(pNetFwCurrentProfile);

    INetFwProfile *pNetFwStandardProfile = nullptr;
    hr = pNetFwPolicy->GetProfileByType(NET_FW_PROFILE_STANDARD, &pNetFwStandardProfile);
    if (FAILED(hr) || (pNetFwStandardProfile == nullptr))
    {
        pNetFwPolicy->Release();
        pNetFwMgr->Release();
        return FALSE;
    }

    // 在标准Profile下添加（家庭/工作(专用)）
    BOOL result2 = AddIt(pNetFwStandardProfile);

    pNetFwCurrentProfile->Release();
    pNetFwStandardProfile->Release();
    pNetFwPolicy->Release();
    pNetFwMgr->Release();

    return (result1 || result2);
}

BOOL RemoveFirewallException(TCHAR *szAppPath)
{
    INetFwMgr *pNetFwMgr = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_NetFwMgr, nullptr, CLSCTX_SERVER, IID_INetFwMgr, (void**)&pNetFwMgr);
    if (FAILED(hr) || (pNetFwMgr == nullptr))
    {
        return FALSE;
    }

    INetFwPolicy *pNetFwPolicy = nullptr;
    hr = pNetFwMgr->get_LocalPolicy(&pNetFwPolicy);
    if (FAILED(hr) || (pNetFwPolicy == nullptr))
    {
        pNetFwMgr->Release();
        return FALSE;
    }

    auto RemoveIt = [szAppPath](INetFwProfile *pNetFwProfile) -> BOOL
    {
        BOOL result = FALSE;

        INetFwAuthorizedApplications *pNetFwAuthorizedApps = nullptr;
        HRESULT hr = pNetFwProfile->get_AuthorizedApplications(&pNetFwAuthorizedApps);
        if (SUCCEEDED(hr))
        {
            // 不知怎么搞的，szAppPath随便一个字符串，Remove也是返回S_OK
            result = SUCCEEDED(pNetFwAuthorizedApps->Remove(szAppPath));
            pNetFwAuthorizedApps->Release();
            pNetFwAuthorizedApps = nullptr;
        }

        return result;
    };

    INetFwProfile *pNetFwCurrentProfile = nullptr;
    hr = pNetFwPolicy->get_CurrentProfile(&pNetFwCurrentProfile);
    if (FAILED(hr) || (pNetFwCurrentProfile == nullptr))
    {
        pNetFwPolicy->Release();
        pNetFwMgr->Release();
        return FALSE;
    }

    // 在当前Profile下移除（公用）
    BOOL result1 = RemoveIt(pNetFwCurrentProfile);

    INetFwProfile *pNetFwStandardProfile = nullptr;
    hr = pNetFwPolicy->GetProfileByType(NET_FW_PROFILE_STANDARD, &pNetFwStandardProfile);
    if (FAILED(hr) || (pNetFwStandardProfile == nullptr))
    {
        pNetFwPolicy->Release();
        pNetFwMgr->Release();
        return FALSE;
    }

    // 在标准Profile下移除（家庭/工作(专用)）
    BOOL result2 = RemoveIt(pNetFwStandardProfile);

    pNetFwCurrentProfile->Release();
    pNetFwStandardProfile->Release();
    pNetFwPolicy->Release();
    pNetFwMgr->Release();

    return (result1 || result2);
}

int _tmain(int argc, _TCHAR* argv[])
{
    if (argc != 4 && argc != 3)
    {
        printf("Usage: firewall.exe [-a caption path]|[-r path]\n");
        printf("    -a : add to firewall while list.\n");
        printf("    -r : remove from firewall while list.\n");
        printf("    caption : the name in while list.\n");
        printf("    path : path of the application.\n");
        return 0;
    }

    HANDLE hProcess = GetCurrentProcess();
    HANDLE hToken;
    if (!OpenProcessToken(hProcess, TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken))
    {
        fwprintf(stderr, L"Can't open current process token\n");
        return 1;
    }

    if (!GetPrivilege(hToken, "SeProfileSingleProcessPrivilege", 1))
    {
        fwprintf(stderr, L"Can't get SeProfileSingleProcessPrivilege\n");
        return 1;
    }

    CloseHandle(hToken);

    int action = 0;
    if (argc == 4 && wcscmp(argv[1], L"-a") == 0)
    {
        action = 1;
    }
    else if (argc == 3 && wcscmp(argv[1], L"-r") == 0)
    {
        action = 2;
    }

    CoInitialize(NULL);

    BOOL result = FALSE;
    if (1 == action)
        result = AddFirewallException(argv[2], argv[3]);
    else if (2 == action)
        result = RemoveFirewallException(argv[2]);

    CoUninitialize();
    return result ? 0 : 1;
}

