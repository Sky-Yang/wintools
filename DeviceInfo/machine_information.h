#ifndef __MACHINE_INFORMATION_H__
#define __MACHINE_INFORMATION_H__

#include <map>
#include <string>
#include <vector>

std::map<std::wstring, int> GetVideoMemory();
bool GetDriveGeometry(const std::wstring& drive_string, int *size);
std::vector<std::wstring> GetSSDDriveString();
std::wstring GetPocessorName();


#endif // __MACHINE_INFORMATION_H__