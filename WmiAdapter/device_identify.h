#ifndef _DEVICE_IDENTIFY_H_
#define _DEVICE_IDENTIFY_H_

#include <string>

typedef unsigned int uint;

#ifdef _MSC_VER
typedef signed __int8  int8;
typedef __int16 int16;
typedef __int32 int32;
typedef __int64 int64;

typedef unsigned __int8  uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;
typedef unsigned __int64 uint64;
#else
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
#endif

struct Win32_Processor
{
    std::wstring ProcessorID;
    std::wstring DeviceID;
    std::wstring Status;
    uint16 Availability;
    uint16 ProcessorType;
};

struct Win32_BaseBoard
{
    std::wstring SerialNumber;
    std::wstring Manufacturer;
    std::wstring Product;
    std::wstring Status;
};

struct Win32_CDROMDrive
{
    std::wstring Drive;
    std::wstring DeviceID;
    std::wstring Status;
};

struct Win32_DiskDrive
{
    std::wstring DeviceID;
    std::wstring SerialNumber;
    std::wstring InterfaceType;
    std::wstring Model;
    std::wstring Status;
    uint32 Index;
    uint64 Size;
};

struct Win32_Keyboard
{
    std::wstring DeviceID;
    std::wstring Caption;
    std::wstring Status;
};

struct Win32_PointingDevice
{
    std::wstring DeviceID;
    std::wstring Caption;
    std::wstring Status;
};

struct Win32_BIOS
{
    std::wstring PrimaryBIOS;
    std::wstring SerialNumber;
    std::wstring Version;
    std::wstring Status;
};

struct Win32_PhysicalMemory
{
    std::wstring Manufacturer;
    std::wstring SerialNumber;
    std::wstring Caption;
    std::wstring Status;
    uint64 Capacity;
    uint16 MemoryType;
};

struct Win32_NetworkAdapter
{
    std::wstring MACAddress;
    std::wstring Manufacturer;
    std::wstring DeviceID;
    std::wstring GUID;
    std::wstring Status;
};

struct Win32_NetworkAdapterConfiguration
{
    std::wstring MACAddress;
    std::wstring IPAddress;
    std::wstring Caption;
    std::wstring DHCPServer;
    int IPEnabled;
    int DHCPEnabled;
};

struct Win32_OperatingSystem
{
    std::wstring BootDevice;
    std::wstring BuildNumber;
    std::wstring BuildType;
    std::wstring Caption;
    std::wstring Manufacturer;
    std::wstring SerialNumber;
    std::wstring Status;
    bool PortableOperatingSystem;
    bool Primary;
    uint32 ProductType;
};

struct Win32_UserAccount
{
    std::wstring AccountType;
    std::wstring Caption;
    std::wstring Status;
};


struct Win32_GroupUser
{
    std::wstring GroupComponent;
    std::wstring PartComponent;
};

struct Win32_Group
{
    std::wstring Caption;
    std::wstring Status;
    uint64 InstallDate;
    bool LocalAccount;
};

struct Win32_ParallelPort
{
    std::wstring Caption;
    std::wstring DeviceID;
    std::wstring Status;
    uint64 InstallDate;
};

struct Win32_SerialPort
{
    std::wstring Caption;
    std::wstring DeviceID;
    std::wstring Status;
    uint64 InstallDate;
};

struct Win32_SoundDevice
{
    std::wstring Caption;
    std::wstring DeviceID;
    std::wstring Status;
    uint64 InstallDate;
};

struct Win32_USBController
{
    std::wstring Caption;
    std::wstring DeviceID;
    std::wstring Status;
    std::wstring Name;
    uint64 InstallDate;
};

struct Win32_Printer
{
    std::wstring Caption;
    std::wstring DeviceID;
    std::wstring PrintProcessor;
    std::wstring Status;
};

struct Win32_DesktopMonitor
{
    std::wstring Caption;
    std::wstring DeviceID;
    std::wstring Status;
    std::string MonitorType;
    uint32 ScreenHeight;
    uint32 ScreenWidth;
    uint16 DisplayType;
};

struct Win32_VideoController
{
    std::wstring Caption;
    std::wstring DeviceID;
    std::wstring VideoProcessor;
    std::wstring Status;
};

struct Win32_ComputerSystemProduct
{
    std::wstring Caption;
    std::wstring IdentifyingNumber;
    std::wstring Version;
    std::wstring UUID;
};

#endif // _DEVICE_IDENTIFY_H_
