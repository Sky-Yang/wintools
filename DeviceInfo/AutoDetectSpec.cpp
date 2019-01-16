#include "StdAfx.h"

#if defined(WIN32) || defined(WIN64)

#include <wbemidl.h>
#include <intrin.h>
#include <d3d9.h>
#include <ddraw.h>
#include <dxgi.h>
#include <d3d11.h>
#include <map>
#include <stdio.h>
#include <cassert>

#include "AutoDetectSpec.h"

#pragma warning(disable: 4244)

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=nullptr; } }
#endif

bool Win32SysInspect::IsVistaKB940105Required()
{
#if defined(WIN32) && !defined(WIN64)
	OSVERSIONINFO osv;
	memset(&osv, 0, sizeof(osv));
	osv.dwOSVersionInfoSize = sizeof(osv);
	GetVersionEx(&osv);

	if (osv.dwMajorVersion != 6 || osv.dwMinorVersion != 0 || (osv.dwBuildNumber > 6000))
	{
		// This QFE only ever applies to Windows Vista RTM. Windows Vista SP1 already has this fix,
		// and earlier versions of Windows do not implement WDDM
		return false;
	}

	//MEMORYSTATUSEX mex;
	//memset(&mex, 0, sizeof(mex));
	//mex.dwLength = sizeof(mex);
	//GlobalMemoryStatusEx(&mex);

	//if (mex.ullTotalVirtual >= 4294836224)
	//{
	//	// If there is 4 GB of VA space total for this process, then we are a
	//	// 32-bit Large Address Aware application running on a Windows 64-bit OS.

	//	// We could be a 32-bit Large Address Aware application running on a
	//	// Windows 32-bit OS and get up to 3 GB, but that has stability implications.
	//	// Therefore, we recommend the QFE for all 32-bit versions of the OS.

	//	// No need for the fix unless the game is pushing 4 GB of VA
	//	return false;
	//}

    const wchar_t* sysFile = L"dxgkrnl.sys";

	// Ensure we are checking the system copy of the file
	wchar_t sysPath[MAX_PATH];
	GetSystemDirectory(sysPath, sizeof(sysPath));

	wcsncat(sysPath, L"\\drivers\\", MAX_PATH);
    wcsncat(sysPath, sysFile, MAX_PATH);

	char buf[2048];
	if (!GetFileVersionInfo(sysPath, 0, sizeof(buf), buf))
	{
		// This should never happen, but we'll assume it's a newer .sys file since we've
		// narrowed the test to a Windows Vista RTM OS.
		return false;
	}

	VS_FIXEDFILEINFO* ver;
	UINT size;
	if (!VerQueryValue(buf, L"\\", (void**) &ver, &size) || size != sizeof(VS_FIXEDFILEINFO) || ver->dwSignature != 0xFEEF04BD)
	{
		// This should never happen, but we'll assume it's a newer .sys file since we've
		// narrowed the test to a Windows Vista RTM OS.
		return false;
	}

	// File major.minor.build.qfe version comparison
	//   WORD major = HIWORD( ver->dwFileVersionMS ); WORD minor = LOWORD( ver->dwFileVersionMS );
	//   WORD build = HIWORD( ver->dwFileVersionLS ); WORD qfe = LOWORD( ver->dwFileVersionLS );

	if (ver->dwFileVersionMS > MAKELONG(0, 6) || (ver->dwFileVersionMS == MAKELONG(0, 6) && ver->dwFileVersionLS >= MAKELONG(20648, 6000)))
	{
		// QFE fix version of dxgkrnl.sys is 6.0.6000.20648
		return false;
	}

	return true;
#else
	return false; // The QFE is not required for a 64-bit native application as it has 8 TB of VA
#endif
}


static void GetSystemMemory(ULONGLONG& totSysMem)
{
	typedef BOOL (WINAPI *FP_GlobalMemoryStatusEx)(LPMEMORYSTATUSEX);
	FP_GlobalMemoryStatusEx pgmsex((FP_GlobalMemoryStatusEx) GetProcAddress(GetModuleHandle(L"kernel32"), "GlobalMemoryStatusEx"));
	if (pgmsex)
	{
		MEMORYSTATUSEX memStats;
		memStats.dwLength = sizeof(memStats);
		if (pgmsex(&memStats))
			totSysMem = memStats.ullTotalPhys;
		else
			totSysMem = 0;
	}
	else
	{
		MEMORYSTATUS memStats;
		memStats.dwLength = sizeof(memStats);
		GlobalMemoryStatus(&memStats);
		totSysMem = memStats.dwTotalPhys;
	}
}


static bool IsVistaOrAbove()
{
	typedef BOOL (WINAPI* FP_VerifyVersionInfo) (LPOSVERSIONINFOEX, DWORD, DWORDLONG);
    FP_VerifyVersionInfo pvvi((FP_VerifyVersionInfo)GetProcAddress(GetModuleHandle(L"kernel32"), "VerifyVersionInfoA"));

	if (pvvi)
	{
		typedef ULONGLONG (WINAPI* FP_VerSetConditionMask) (ULONGLONG, DWORD, BYTE);
        FP_VerSetConditionMask pvscm((FP_VerSetConditionMask)GetProcAddress(GetModuleHandle(L"kernel32"), "VerSetConditionMask"));
		assert(pvscm);

		OSVERSIONINFOEX osvi;
		memset(&osvi, 0, sizeof(osvi));
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
		osvi.dwMajorVersion = 6;
		osvi.dwMinorVersion = 0;
		osvi.wServicePackMajor = 0;
		osvi.wServicePackMinor = 0;

		ULONGLONG mask(0);
		mask = pvscm(mask, VER_MAJORVERSION, VER_GREATER_EQUAL);
		mask = pvscm(mask, VER_MINORVERSION, VER_GREATER_EQUAL);
		mask = pvscm(mask, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
		mask = pvscm(mask, VER_SERVICEPACKMINOR, VER_GREATER_EQUAL);

		if (pvvi(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR, mask))
			return true;
	}

	return false;
}

static Win32SysInspect::DXFeatureLevel GetFeatureLevel(D3D_FEATURE_LEVEL featureLevel)
{
	switch (featureLevel)
	{
	case D3D_FEATURE_LEVEL_9_1:
		return Win32SysInspect::DXFL_9_1;
	case D3D_FEATURE_LEVEL_9_2:
		return Win32SysInspect::DXFL_9_2;
	case D3D_FEATURE_LEVEL_9_3:
		return Win32SysInspect::DXFL_9_3;
	case D3D_FEATURE_LEVEL_10_0:
		return Win32SysInspect::DXFL_10_0;
	case D3D_FEATURE_LEVEL_10_1:
		return Win32SysInspect::DXFL_10_1;
	case D3D_FEATURE_LEVEL_11_0:
	default:
		return Win32SysInspect::DXFL_11_0;
	}
}


static int GetDXGIAdapterOverride()
{
#if defined(WIN32) || defined(WIN64)
    return -1;
	//ICVar* pCVar = gEnv->pConsole ? gEnv->pConsole->GetCVar("r_overrideDXGIAdapter") : 0;
	//return pCVar ? pCVar->GetIVal() : -1;
#else
	return -1;
#endif
}


static void LogDeviceInfo(unsigned int adapterIdx, const DXGI_ADAPTER_DESC1& ad, Win32SysInspect::DXFeatureLevel fl, bool displaysConnected)
{
	const bool suitableDevice = fl >= Win32SysInspect::DXFL_11_0 && displaysConnected;

	printf("- %ls (vendor = 0x%.4x, device = 0x%.4x)", ad.Description, ad.VendorId, ad.DeviceId);
	printf("  - Adapter index: %d", adapterIdx);
	printf("  - Dedicated video memory: %d MB", ad.DedicatedVideoMemory >> 20);
	printf("  - Feature level: %s", GetFeatureLevelAsString(fl));
	printf("  - Displays connected: %s", displaysConnected ? "yes" : "no");
	printf("  - Suitable rendering device: %s", suitableDevice ? "yes" : "no");
}


static bool FindGPU(DXGI_ADAPTER_DESC1& adapterDesc, Win32SysInspect::DXFeatureLevel& featureLevel)
{
	memset(&adapterDesc, 0, sizeof(adapterDesc));
	featureLevel = Win32SysInspect::DXFL_Undefined;

	if (!IsVistaOrAbove())
		return false;

	typedef HRESULT (WINAPI *FP_CreateDXGIFactory1)(REFIID, void**);
	FP_CreateDXGIFactory1 pCDXGIF = (FP_CreateDXGIFactory1) GetProcAddress(LoadLibrary(L"dxgi.dll"), "CreateDXGIFactory1");

	IDXGIFactory1* pFactory = 0;
	if (pCDXGIF && SUCCEEDED(pCDXGIF(__uuidof(IDXGIFactory1), (void**) &pFactory)) && pFactory)
	{
		typedef HRESULT (WINAPI *FP_D3D11CreateDevice)(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT, CONST D3D_FEATURE_LEVEL*, UINT, UINT, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**);
        FP_D3D11CreateDevice pD3D11CD = (FP_D3D11CreateDevice)GetProcAddress(LoadLibrary(L"d3d11.dll"), "D3D11CreateDevice");

		if (pD3D11CD)
		{
			const int r_overrideDXGIAdapter = GetDXGIAdapterOverride();
			const bool logDeviceInfo = !gEnv->pRenderer && r_overrideDXGIAdapter < 0;

			if (logDeviceInfo)
				printf("Logging video adapters:");

			unsigned int nAdapter = r_overrideDXGIAdapter >= 0 ? r_overrideDXGIAdapter : 0;
			IDXGIAdapter1* pAdapter = 0;
			while (pFactory->EnumAdapters1(nAdapter, &pAdapter) != DXGI_ERROR_NOT_FOUND)
			{
				if (pAdapter)
				{
					ID3D11Device* pDevice = 0;
					D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_9_3, D3D_FEATURE_LEVEL_9_2, D3D_FEATURE_LEVEL_9_1};
					D3D_FEATURE_LEVEL deviceFeatureLevel = D3D_FEATURE_LEVEL_9_1;
					HRESULT hr = pD3D11CD(pAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, levels, sizeof(levels) / sizeof(levels[0]), D3D11_SDK_VERSION, &pDevice, &deviceFeatureLevel, NULL);
					if (SUCCEEDED(hr) && pDevice)
					{
						IDXGIOutput* pOutput = 0;
						const bool displaysConnected = SUCCEEDED(pAdapter->EnumOutputs(0, &pOutput)) && pOutput;
						SAFE_RELEASE(pOutput);

						DXGI_ADAPTER_DESC1 ad;
						pAdapter->GetDesc1(&ad);

						const Win32SysInspect::DXFeatureLevel fl = GetFeatureLevel(deviceFeatureLevel);

						if (logDeviceInfo)
							LogDeviceInfo(nAdapter, ad, fl, displaysConnected);

						if (featureLevel < fl && displaysConnected)
						{
							adapterDesc = ad;
							featureLevel = fl;
						}
						else if (r_overrideDXGIAdapter >= 0)
							printf("No display connected to DXGI adapter override %d. Adapter cannot be used for rendering.", r_overrideDXGIAdapter);
					}

					SAFE_RELEASE(pDevice);
					SAFE_RELEASE(pAdapter);
				}
				if (r_overrideDXGIAdapter >= 0)
					break;
				++nAdapter;
			}
		}
	}
	SAFE_RELEASE(pFactory);
	return featureLevel != Win32SysInspect::DXFL_Undefined;
}


bool Win32SysInspect::IsDX11Supported()
{
	DXGI_ADAPTER_DESC1 adapterDesc = {0};
	DXFeatureLevel featureLevel = Win32SysInspect::DXFL_Undefined;
	return FindGPU(adapterDesc, featureLevel) && featureLevel >= DXFL_11_0;
}


bool Win32SysInspect::GetGPUInfo(char* pName, size_t bufferSize, unsigned int& vendorID, unsigned int& deviceID, unsigned int& totLocalVidMem, DXFeatureLevel& featureLevel)
{
	if (pName && bufferSize)
		pName[0] = '\0';

	vendorID = 0;
	deviceID = 0;
	totLocalVidMem = 0;
	featureLevel = Win32SysInspect::DXFL_Undefined;

	DXGI_ADAPTER_DESC1 adapterDesc = {0};
	const bool gpuFound = FindGPU(adapterDesc, featureLevel);
	if (gpuFound)
	{
		vendorID = adapterDesc.VendorId;
		deviceID = adapterDesc.DeviceId;

		if (pName && bufferSize)
			sprintf_s(pName, bufferSize, "%ls", adapterDesc.Description);

		totLocalVidMem = adapterDesc.DedicatedVideoMemory;
	}

	return gpuFound;
}


class CGPURating
{
public:
	CGPURating();
	~CGPURating();

	int GetRating(unsigned int vendorId, unsigned int deviceId) const;

private:
	struct SGPUID
	{
		SGPUID(unsigned int vendorId, unsigned int deviceId)
		: vendor(vendorId)
		, device(deviceId)
		{
		}

		bool operator < (const SGPUID& rhs) const
		{
			if (vendor == rhs.vendor)
				return device < rhs.device;
			else
				return vendor < rhs.vendor;
		}

		unsigned int vendor;
		unsigned int device;
	};

	typedef std::map<SGPUID, int> GPURatingMap;

private:
	GPURatingMap m_gpuRatingMap;

};


#else

#include "System.h"

void CSystem::AutoDetectSpec(const bool detectResolution)
{
}


#endif
