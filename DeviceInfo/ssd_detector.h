#ifndef _SSD_DETECTOR_H_
#define _SSD_DETECTOR_H_

#include <string>
#include <set>

std::wstring GetPhysicalDriveString(const std::wstring& path);
bool IsKugouInstalOnSSD(const std::wstring& install_path);
bool DetectsSSD(const std::wstring& physical_drive);
void DetectsKugouIsInstallOnSSD(const std::wstring& install_path);
std::set<std::wstring> GetSSDDriveString();

#endif // _SSD_DETECTOR_H_