#include "ssd_detector.h"

#include <vector>
#include <windows.h>
#include <sstream>


namespace
{

}

std::wstring GetPhysicalDriveString(const std::wstring& path)
{
    std::wstring driver_name = L"\\\\.\\" + path;
    HANDLE drive = CreateFile(
        driver_name.c_str(), 0, FILE_SHARE_READ, NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_READONLY, NULL);

    int error = GetLastError();
    if (!drive)
    {
        return L"";
    }

    STORAGE_DEVICE_NUMBER disk_number;
    DWORD bytes_returned = 0;
    if (!DeviceIoControl(drive, IOCTL_STORAGE_GET_DEVICE_NUMBER,
        NULL, 0, &disk_number, sizeof(STORAGE_DEVICE_NUMBER),
        &bytes_returned, NULL))
    {
        int error = GetLastError();
        return L"";
    }

    std::wstringstream stream;
    stream << L"\\\\.\\PhysicalDrive" << static_cast<int>(disk_number.DeviceNumber);
    return stream.str();
}

bool DetectsSSD(const std::wstring& physical_drive)
{
    if (physical_drive.empty())
        return false;

    std::wstring&& drive_string = GetPhysicalDriveString(physical_drive);
    HANDLE drive = CreateFile(
        drive_string.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
        OPEN_EXISTING, 0, NULL);

    if (!drive)
    {
        int error = GetLastError();
        return false;
    }
    // Check TRIM -- should be Y for SSD
    STORAGE_PROPERTY_QUERY spqTrim;
    spqTrim.PropertyId = (STORAGE_PROPERTY_ID)StorageDeviceTrimProperty;
    spqTrim.QueryType = PropertyStandardQuery;
    DEVICE_TRIM_DESCRIPTOR dtd = { 0 };

    DWORD size = 0;
    DWORD result = DeviceIoControl(
        drive, IOCTL_STORAGE_QUERY_PROPERTY,
        &spqTrim, sizeof(spqTrim), &dtd, sizeof(dtd),
        &size, NULL);
    if (!result)
    {
        int error = GetLastError();
        return false;
    }

    return !!dtd.TrimEnabled;
}

