#ifndef _AUTODETECTSPEC_
#define _AUTODETECTSPEC_

#pragma once


#if defined(WIN32) || defined(WIN64)

// exposed AutoDetectSpec() helper functions for reuse in CrySystem
namespace Win32SysInspect
{
	enum DXFeatureLevel
	{
		DXFL_Undefined,
		DXFL_9_1,
		DXFL_9_2,
		DXFL_9_3,
		DXFL_10_0,
		DXFL_10_1,
		DXFL_11_0
	};

	static const char* GetFeatureLevelAsString(DXFeatureLevel featureLevel)
	{
		switch (featureLevel)
		{
		case Win32SysInspect::DXFL_Undefined:
			return "unknown";
		case Win32SysInspect::DXFL_9_1:
			return "DX9 (SM 2.0)";
		case Win32SysInspect::DXFL_9_2:
			return "DX9 (SM 2.0)";
		case Win32SysInspect::DXFL_9_3:
			return "DX9 (SM 2.x)";
		case Win32SysInspect::DXFL_10_0:
			return "DX10 (SM 4.0)";
		case Win32SysInspect::DXFL_10_1:
			return "DX10.1 (SM 4.x)";
		case Win32SysInspect::DXFL_11_0:
		default:
			return "DX11 (SM 5.0)";
		}
	}

	bool IsDX11Supported();
	bool GetGPUInfo(char* pName, size_t bufferSize, unsigned int& vendorID, unsigned int& deviceID, unsigned int& totLocalVidMem, DXFeatureLevel& featureLevel);
	bool IsVistaKB940105Required();

	inline size_t SafeMemoryThreshold(size_t memMB)
	{
		return (memMB * 8) / 10;
	}
}

#endif // #if defined(WIN32) || defined(WIN64)


#endif // #ifndef _AUTODETECTSPEC_