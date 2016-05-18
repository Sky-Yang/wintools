#ifndef __MACHINE_INFORMATION_H__
#define __MACHINE_INFORMATION_H__

#include <map>
#include <string>
#include <vector>

std::map<std::wstring, int> GetVideoMemory();   // in MB
bool GetDriveGeometry(const std::wstring& drive_string, int *size); // in GB

#endif // __MACHINE_INFORMATION_H__