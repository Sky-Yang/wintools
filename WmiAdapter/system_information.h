#ifndef _SYSTEM_INFORMATION_H_
#define _SYSTEM_INFORMATION_H_

#include <map>
#include <vector>
#include <memory>
#include <functional>
#include <Windows.h>

class SystemInfo
{
public:
    static SystemInfo* GetInstance();
    ~SystemInfo();

    int GetCores();
    std::wstring GetCPUID();
    double GetTotalPhysicalMemory(); // 系统总的物理内存大小，单位为M
    double GetAvailPhysicalMemory(); // 系统可用的物理内存大小，单位为M
    std::wstring GetDiskSerialAdminMode(int diskIndex);
    std::wstring GetMac(int macIndex);
    std::wstring GetLocalMacs();

private:
    SystemInfo();

    bool ReadPhysicalDriveInNTWithAdminRights(std::wstring* serial,
                                              int diskIndex);
    bool ReadIdeDriveAsScsiDriveInNT(std::wstring* serial, int diskIndex);
    bool ReadPhysicalDriveInNTWithZeroRights(
        std::wstring* serial, int diskIndex,
        const std::function<bool(char*, std::wstring*)>& callback);
    std::wstring ConvertToString(const std::vector<WORD>& serial,
                                 bool bigEndian);

    int cores_;
    int speed_;
    double totalPhysicalMemory_;
    std::wstring coreId_;
    std::wstring processorName_;
    std::map<int, std::wstring> diskIds_;
    std::map<int, std::wstring> diskIdxEx_;
    std::vector<std::wstring> macs_;
    std::wstring localMacs_;
};

#endif // _SYSTEM_INFORMATION_H_
