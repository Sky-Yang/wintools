#include "system_information.h"

#include <cassert>
#include <WinIoCtl.h>
#include <Iphlpapi.h>
#include <Ntddscsi.h>
#include <cctype>
#include <clocale>
#include <string>

using std::wstring;

namespace
{
#define CPU_REG_KEY     HKEY_LOCAL_MACHINE
#define SWAP(a) ((((a) << 8) & 0xFF00) | (((a) >> 8) & 0x00FF))

const wchar_t* CpuRegSubkey = L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0";
const wchar_t* CpuSpeed = L"~MHz";
const wchar_t* CpuName = L"ProcessorNameString";
const wchar_t* cUnknownString = L"Unknown";

// disk
const int IdentifyBufferSize = 512;

//IOCTL commands
const uint32_t GetDfpVersion = 0x00074080;
const uint32_t SendDfpDriveCommand = 0x0007c084;
const uint32_t ReceiveDfpDriveData = 0x0007c088;
const uint32_t FileDeviceScsi = 0x0000001b;
const uint32_t IoScsiMiniportIdentify = (FileDeviceScsi << 16) + 0x0501;

//Valid values for the bCommandReg member of IDEREGS.
#define  IDE_ATAPI_IDENTIFY  0xA1  //  Returns ID sector for ATAPI.
#define  IDE_ATA_IDENTIFY    0xEC  //  Returns ID sector for ATA.

#define SENDIDLENGTH    sizeof(SENDCMDOUTPARAMS) + IdentifyBufferSize

//The following struct defines the interesting part of the IDENTIFY buffer:
typedef struct _IDSECTOR
{
    USHORT  wGenConfig;
    USHORT  wNumCyls;
    USHORT  wReserved;
    USHORT  wNumHeads;
    USHORT  wBytesPerTrack;
    USHORT  wBytesPerSector;
    USHORT  wSectorsPerTrack;
    USHORT  wVendorUnique[3];
    CHAR    sSerialNumber[20];
    USHORT  wBufferType;
    USHORT  wBufferSize;
    USHORT  wECCSize;
    CHAR    sFirmwareRev[8];
    CHAR    sModelNumber[40];
    USHORT  wMoreVendorUnique;
    USHORT  wDoubleWordIO;
    USHORT  wCapabilities;
    USHORT  wReserved1;
    USHORT  wPIOTiming;
    USHORT  wDMATiming;
    USHORT  wBS;
    USHORT  wNumCurrentCyls;
    USHORT  wNumCurrentHeads;
    USHORT  wNumCurrentSectorsPerTrack;
    ULONG   ulCurrentSectorCapacity;
    USHORT  wMultSectorStuff;
    ULONG   ulTotalAddressableSectors;
    USHORT  wSingleWordDMA;
    USHORT  wMultiWordDMA;
    BYTE    bReserved[128];
} IDSECTOR, *PIDSECTOR;

#define IOCTL_STORAGE_QUERY_PROPERTY   CTL_CODE(IOCTL_STORAGE_BASE, 0x0500, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STORAGE_GET_MEDIA_SERIAL_NUMBER CTL_CODE(IOCTL_STORAGE_BASE, 0x0304, METHOD_BUFFERED, FILE_ANY_ACCESS)

// disk end

bool isspace(char c)
{
    return (c == ' ' || c == '\t' || c == '\n');
}

std::string& TrimString(std::string &s)
{
    if (s.empty())
    {
        return s;
    }
    s.erase(0, s.find_first_not_of(" "));
    s.erase(s.find_last_not_of(" ") + 1);
    return s;
}

std::wstring SysMultiByteToWide(const std::string& mb, uint32_t code_page) {
    if (mb.empty())
        return std::wstring();

    int mb_length = static_cast<int>(mb.length());
    // Compute the length of the buffer.
    int charcount = MultiByteToWideChar(code_page, 0,
                                        mb.data(), mb_length, NULL, 0);
    if (charcount == 0)
        return std::wstring();

    std::wstring wide;
    wide.resize(charcount);
    MultiByteToWideChar(code_page, 0, mb.data(), mb_length, &wide[0], charcount);

    return wide;
}
std::wstring SysUTF8ToWide(const std::string& utf8) {
    return SysMultiByteToWide(utf8, CP_UTF8);
}
bool IsWin8OrLater()
{
    OSVERSIONINFO osvi;
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (!GetVersionEx(&osvi))
        return false;

    return (osvi.dwMajorVersion > 6) || ((osvi.dwMajorVersion == 6) && (osvi.dwMinorVersion >= 2));
}

void ChangeByteOrder(BYTE* data, int size)
{
    for (int i = 0; i < size - 1; i += 2)
    {
        WORD* p = reinterpret_cast<WORD*>(&data[i]);
        *p = SWAP(*p);
    }
}

int FlipAndCodeBytes(const char* str, int pos, int flip, char* buf)
{
    int i = 0;
    int j = 0;
    int cursor = 0;

    buf[0] = '\0';
    if (pos <= 0)
        return 0;

    int offset = 0;

    // First try to gather all characters representing hex digits only.
    j = 1;
    cursor = 0;
    buf[cursor] = 0;
    for (i = pos; j && str[i] != '\0'; ++i)
    {
        char c = std::tolower(str[i]);

        if (isspace(c))
            c = '0';

        ++offset;
        buf[cursor] <<= 4;
        if (c >= '0' && c <= '9')
        {
            buf[cursor] |= (unsigned char)(c - '0');
        }
        else if (c >= 'a' && c <= 'f')
        {
            buf[cursor] |= (unsigned char)(c - 'a' + 10);
        }
        else
        {
            j = 0;
            break;
        }

        if (offset == 2)
        {
            if (buf[cursor] != '\0' && !std::isprint(buf[cursor]))
            {
                j = 0;
                break;
            }

            ++cursor;
            offset = 0;
            buf[cursor] = 0;
        }
    }

    if (!j)
    {
        // There are non-digit characters, gather them as is.
        j = 1;
        cursor = 0;
        for (i = pos; j && str[i] != '\0'; ++i)
        {
            char c = str[i];
            if (!isprint(c))
            {
                j = 0;
                break;
            }

            buf[cursor++] = c;
        }
    }

    if (!j)
    {
        // The characters are not there or are not printable.
        cursor = 0;
    }

    buf[cursor] = '\0';

    if (flip)
    {
        // Flip adjacent characters
        for (j = 0; j < cursor; j += 2)
        {
            char t = buf[j];
            buf[j] = buf[j + 1];
            buf[j + 1] = t;
        }
    }

    // Trim any beginning and end space
    i = -1;
    j = -1;
    for (cursor = 0; buf[cursor] != '\0'; ++cursor)
    {
        if (!isspace(buf[cursor]))
        {
            if (i < 0)
                i = cursor;

            j = cursor;
        }
    }

    if ((i >= 0) && (j >= 0))
    {
        for (cursor = i; (cursor <= j) && (buf[cursor] != '\0'); ++cursor)
            buf[cursor - i] = buf[cursor];

        buf[cursor - i] = '\0';
    }

    return 1;
}

void AsmGetCPUID(wchar_t CPUID[36])
{
    DWORD x[4] = {};

    // cpuid
    _asm
    {
        push ebx
        mov eax, 1
        cpuid
        mov dword ptr[x], eax
        mov dword ptr[x + 4], ebx
        mov dword ptr[x + 4 * 2], ecx
        mov dword ptr[x + 4 * 3], edx
        pop ebx
    }

    swprintf_s(CPUID, 36, L"%08X-%08X-%08X-%08X", x[0], x[1], x[2], x[3]);
}

}

SystemInfo* SystemInfo::GetInstance()
{
    static SystemInfo* info = nullptr;
    if (info == nullptr)
        info = new SystemInfo;

    return info;
}

SystemInfo::SystemInfo()
    : cores_(0)
    , coreId_()
    , processorName_()
    , speed_(0)
    , totalPhysicalMemory_(0)
    , diskIds_()
    , macs_()
{
}

SystemInfo::~SystemInfo()
{

}

int SystemInfo::GetCores()
{
    SYSTEM_INFO sysInfo;
    GetSystemInfo (&sysInfo);
    cores_ = sysInfo.dwNumberOfProcessors;
    return cores_;
}

std::wstring SystemInfo::GetCPUID()
{
    if (coreId_.empty())
    {
        HANDLE currentProcess = ::GetCurrentProcess();
        DWORD initialProcessAffinityMask = 0;
        DWORD systemAffinityMask = 0;
        ::GetProcessAffinityMask(currentProcess, &initialProcessAffinityMask,
                                 &systemAffinityMask);
        if (initialProcessAffinityMask != 0)
        {
            // 保证代码只在一个核上运行
            DWORD replaceProcessAffinityMask = 1;
            while (!(replaceProcessAffinityMask & initialProcessAffinityMask))
                replaceProcessAffinityMask <<= 1;

            ::SetProcessAffinityMask(currentProcess,
                                     replaceProcessAffinityMask);
            wchar_t ids[36] = { 0 };
            AsmGetCPUID(ids);
            ::SetProcessAffinityMask(currentProcess,
                                     initialProcessAffinityMask);
            coreId_ = std::wstring(ids);
        }
        else
        {
            coreId_ = cUnknownString;
        }
    }

    return coreId_;
}


double SystemInfo::GetTotalPhysicalMemory()
{
    if (0 == totalPhysicalMemory_)
    {
        MEMORYSTATUSEX stat = { 0 };
        stat.dwLength = sizeof(stat);
        ::GlobalMemoryStatusEx(&stat);
        totalPhysicalMemory_ = stat.ullTotalPhys / 1024.0 / 1024.0;
    }
    return totalPhysicalMemory_;
}

double SystemInfo::GetAvailPhysicalMemory()
{
    MEMORYSTATUSEX stat = { 0 };
    stat.dwLength = sizeof(stat);
    ::GlobalMemoryStatusEx(&stat);
    return stat.ullAvailPhys / 1024.0 / 1024.0;
}

std::wstring SystemInfo::GetDiskSerialAdminMode(int diskIndex)
{
    auto it = diskIdxEx_.find(diskIndex);
    if (it == diskIdxEx_.end())
    {
        auto function = [](char* buffer, std::wstring* serial) -> bool
        {
            STORAGE_DEVICE_DESCRIPTOR* descrip = 
                reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR*>(buffer);

            char data[1024] = { 0 };
            int flip = IsWin8OrLater() ? 1 : 0;
            FlipAndCodeBytes(reinterpret_cast<char*>(buffer),
                             descrip->SerialNumberOffset, flip, data);
            ChangeByteOrder(reinterpret_cast<BYTE*>(data), lstrlenA(data));

            std::string tempSerial(data);
            tempSerial = TrimString(tempSerial);

            *serial = SysUTF8ToWide(tempSerial);
            return true;
        };

        std::wstring strSerial(L"");
        do
        {
            if (ReadPhysicalDriveInNTWithAdminRights(&strSerial, diskIndex))
                break;

            if (ReadIdeDriveAsScsiDriveInNT(&strSerial, diskIndex))
                break;

            ReadPhysicalDriveInNTWithZeroRights(&strSerial, diskIndex, function);
        } while (0);

        diskIdxEx_[diskIndex] = strSerial;
        return strSerial;
    }
    else
    {
        return it->second;
    }
}

std::wstring SystemInfo::ConvertToString(const std::vector<WORD>& serial,
                                         bool bigEndian)
{
    std::wstring str;
    for (auto it = serial.begin(); it != serial.end(); ++it)
    {
        BYTE low = bigEndian ? static_cast<BYTE>(((*it) >> 8)) : 
            static_cast<BYTE>(*it);
        if (low != 0 && low != 0x20)  // 不接受空格和0
            str.push_back(static_cast<wchar_t>(low));
        BYTE high = bigEndian ? static_cast<BYTE>(*it) : 
            static_cast<BYTE>(((*it) >> 8));
        if (high != 0 && high != 0x20) // 不接受空格和0
            str.push_back(static_cast<wchar_t>(high));
    }

    return str;
}

bool SystemInfo::ReadPhysicalDriveInNTWithAdminRights(std::wstring* serial,
                                                      int diskIndex)
{
    if (!serial)
        return false;

    assert(diskIndex >= 0);

    bool done = false;
    std::wstring driveName = L"\\\\.\\PhysicalDrive" + std::to_wstring(diskIndex);
    HANDLE driveHandle = ::CreateFileW(driveName.c_str(),
                                       GENERIC_READ | GENERIC_WRITE,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       NULL, OPEN_EXISTING, 0, NULL);
    if (driveHandle && INVALID_HANDLE_VALUE	!= driveHandle)
    {
        GETVERSIONINPARAMS versionParams = { 0 };
        DWORD cbBytesReturned = 0;

        if (::DeviceIoControl(driveHandle, GetDfpVersion, NULL, 0,
            &versionParams, sizeof(versionParams), &cbBytesReturned, NULL) &&
            versionParams.bIDEDeviceMap > 0)
        {
            // Now, get the ID sector for all IDE devices in the system.
            // If the device is ATAPI use the IDE_ATAPI_IDENTIFY command,
            // otherwise use the IDE_ATA_IDENTIFY command
            const int outCmdLen = 
                sizeof(SENDCMDOUTPARAMS) + IdentifyBufferSize - 1;
            std::vector<uint8_t> idOutCmd(outCmdLen);
            BYTE deviceCmdId = (versionParams.bIDEDeviceMap >> diskIndex & 0x10) ? 
                IDE_ATAPI_IDENTIFY : IDE_ATA_IDENTIFY;
            SENDCMDINPARAMS scip = { 0 };

            scip.cBufferSize = IdentifyBufferSize;
            scip.irDriveRegs.bFeaturesReg = 0;
            scip.irDriveRegs.bSectorCountReg = 1;
            scip.irDriveRegs.bCylLowReg = 0;
            scip.irDriveRegs.bCylHighReg = 0;
            scip.irDriveRegs.bDriveHeadReg = 0xA0 | ((diskIndex&1) << 4);
            scip.irDriveRegs.bCommandReg = deviceCmdId;
            scip.bDriveNumber = diskIndex;
            scip.cBufferSize = IdentifyBufferSize;

            if(::DeviceIoControl(
                driveHandle, ReceiveDfpDriveData,
                reinterpret_cast<LPVOID>(&scip), sizeof(SENDCMDINPARAMS) - 1,
                reinterpret_cast<LPVOID>(&idOutCmd.front()),
                sizeof(SENDCMDOUTPARAMS) + IdentifyBufferSize - 1,
                &cbBytesReturned, NULL))
            {
                //process the information for this buffer
                SENDCMDOUTPARAMS* outParams = 
                    reinterpret_cast<SENDCMDOUTPARAMS*>(&idOutCmd.front());
                USHORT* processInfo = 
                    reinterpret_cast<USHORT*>(outParams->bBuffer);

                std::vector<WORD> diskInfo;
                for(int j = 10; j < 20; ++j) 
                    diskInfo.push_back(processInfo[j]);

                *serial = ConvertToString(diskInfo, true);
                done = true;
            }
        }
        ::CloseHandle(driveHandle);
    }

    return done;
}

bool SystemInfo::ReadIdeDriveAsScsiDriveInNT(std::wstring* serial,
                                             int diskIndex)
{
    if (!serial)
        return false;

    bool done = false;
    std::wstring driveName = L"\\\\.\\Scsi" + std::to_wstring(diskIndex);
    HANDLE driveHandle = ::CreateFileW(driveName.c_str(),
                                       GENERIC_READ | GENERIC_WRITE,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       NULL, OPEN_EXISTING, 0, NULL);
    if (driveHandle && INVALID_HANDLE_VALUE != driveHandle)
    {
        for (int j = 0; j < 2; ++j)
        {
            std::vector<uint8_t> buffer(sizeof(SRB_IO_CONTROL) + SENDIDLENGTH);
            SRB_IO_CONTROL* ioCotrol = 
                reinterpret_cast<SRB_IO_CONTROL*>(&buffer.front());
            SENDCMDINPARAMS* scip = 
                reinterpret_cast<SENDCMDINPARAMS*>(&buffer.front() + 
                                                   sizeof(SRB_IO_CONTROL));
            DWORD dummy = 0;

            ioCotrol->HeaderLength = sizeof(SRB_IO_CONTROL);
            ioCotrol->Timeout = 10000;
            ioCotrol->Length = SENDIDLENGTH;
            ioCotrol->ControlCode = IoScsiMiniportIdentify;
            memcpy_s(ioCotrol->Signature, 8, "SCSIDISK", 8);
            scip->irDriveRegs.bCommandReg = IDE_ATA_IDENTIFY;
            scip->bDriveNumber = j;
            if (DeviceIoControl(driveHandle, IOCTL_SCSI_MINIPORT,
                reinterpret_cast<LPVOID>(&buffer.front()),
                sizeof(SRB_IO_CONTROL) + sizeof(SENDCMDINPARAMS) - 1,
                reinterpret_cast<LPVOID>(&buffer.front()),
                sizeof(SRB_IO_CONTROL) + SENDIDLENGTH, &dummy, NULL))
            {
                SENDCMDOUTPARAMS* scop = 
                    reinterpret_cast<SENDCMDOUTPARAMS*>(&buffer.front() + 
                                                        sizeof(SRB_IO_CONTROL));
                IDSECTOR* processbuffer = 
                    reinterpret_cast<IDSECTOR*>(scop->bBuffer);
                if (processbuffer->sModelNumber[0])
                {
                    //process the information for this buffer
                    USHORT* processInfo = 
                        reinterpret_cast<USHORT*>(processbuffer);
                    std::vector<WORD> diskInfo;
                    for(int k = 10; k < 20; ++k) 
                        diskInfo.push_back(processInfo[k]);

                    *serial = ConvertToString(diskInfo, true);
                    done = true;
                }
            }
        }
        ::CloseHandle(driveHandle);
    }

    return done;
}

bool SystemInfo::ReadPhysicalDriveInNTWithZeroRights(
    std::wstring* serial, int diskIndex,
    const std::function<bool (char*, std::wstring*)>& callback)
{
    if (!serial)
        return false;

    std::wstring driveName = L"\\\\.\\PhysicalDrive" + std::to_wstring(diskIndex);
    HANDLE driveHandle = ::CreateFileW(driveName.c_str(), 0,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       NULL, OPEN_EXISTING, 0, NULL);
    if (driveHandle && INVALID_HANDLE_VALUE != driveHandle)
    {
        STORAGE_PROPERTY_QUERY query = {};
        query.PropertyId = StorageDeviceProperty;
        query.QueryType = PropertyStandardQuery;

        DWORD cbBytesReturned = 0;
        char buffer[8192] = { 0 };
        if (::DeviceIoControl(driveHandle, IOCTL_STORAGE_QUERY_PROPERTY,
                              &query, sizeof(query), &buffer,
                              sizeof(buffer), &cbBytesReturned, NULL))
        {
            if (callback)
                return callback(&buffer[0], serial);
        }
        ::CloseHandle(driveHandle);
    }

    return true;
}

std::wstring SystemInfo::GetMac(int macIndex)
{
    if (macs_.empty())
    {
        DWORD bufferLen = 0;
        ::GetAdaptersInfo(NULL, &bufferLen);

        std::unique_ptr<byte[]> buffer(new byte[bufferLen]);
        IP_ADAPTER_INFO* pInfo = 
            reinterpret_cast<IP_ADAPTER_INFO*>(buffer.get());
        if (ERROR_SUCCESS == ::GetAdaptersInfo(pInfo, &bufferLen))
        {
            wchar_t mac[16] = { 0 };
            while (pInfo)
            {
                swprintf_s(mac, L"%02X%02X%02X%02X%02X%02X", pInfo->Address[0],
                           pInfo->Address[1], pInfo->Address[2],
                           pInfo->Address[3], pInfo->Address[4],
                           pInfo->Address[5]);

                macs_.push_back(mac);
                pInfo = pInfo->Next;
            }
        }
    }

    if ((macIndex >= 0) && (macIndex < static_cast<int>(macs_.size())))
        return macs_[macIndex];
    else
        return std::wstring();
}

std::wstring SystemInfo::GetLocalMacs()
{
    if (localMacs_.empty())
    {
        DWORD bufferLen = 0;
        ::GetAdaptersInfo(nullptr, &bufferLen);

        std::unique_ptr<BYTE[]> buffer(new BYTE[bufferLen]);
        IP_ADAPTER_INFO* pInfo = 
            reinterpret_cast<IP_ADAPTER_INFO*>(buffer.get());
        if (NO_ERROR == ::GetAdaptersInfo(pInfo, &bufferLen))
        {
            int count = 0;
            while (pInfo != nullptr && count < 16)
            {
                wchar_t mac[32] = { 0 };
                swprintf_s(mac, L"%02X-%02X-%02X-%02X-%02X-%02X",
                           pInfo->Address[0], pInfo->Address[1],
                           pInfo->Address[2], pInfo->Address[3],
                           pInfo->Address[4], pInfo->Address[5]);

                localMacs_.append(mac);

                pInfo = pInfo->Next;
                ++count;

                if (pInfo != nullptr && count < 16)
                    localMacs_.append(L",");
            }
        }
    }

    return localMacs_;
}
