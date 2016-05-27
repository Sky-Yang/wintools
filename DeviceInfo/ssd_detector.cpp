#include "ssd_detector.h"

#include <vector>
#include <windows.h>
#include <sstream>
#include <set>
#include <ntddscsi.h>


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

struct ATAIdentifyDeviceQuery
{
    ATA_PASS_THROUGH_EX header;
    WORD data[256];
};

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

//    ATAIdentifyDeviceQuery id_query;
//    memset(&id_query, 0, sizeof(id_query));
//
//    id_query.header.Length = sizeof(id_query.header);
//    id_query.header.AtaFlags = ATA_FLAGS_DATA_IN;
//    id_query.header.DataTransferLength = sizeof(id_query.data);
//    id_query.header.TimeOutValue = 5;   //Timeout in seconds
//    id_query.header.DataBufferOffset = offsetof(ATAIdentifyDeviceQuery, data[0]);
//    id_query.header.CurrentTaskFile[6] = 0xec; // ATA IDENTIFY DEVICE
//
//    size = 0;
//    if (::DeviceIoControl(drive, IOCTL_ATA_PASS_THROUGH,
//        &id_query, sizeof(id_query), &id_query, sizeof(id_query), &size, NULL) &&
//        size == sizeof(id_query))
//    {
//        //Got it
//
//        //Index of nominal media rotation rate
//        //SOURCE: http://www.t13.org/documents/UploadedDocuments/docs2009/d2015r1a-ATAATAPI_Command_Set_-_2_ACS-2.pdf
//        //          7.18.7.81 Word 217
//        //QUOTE: Word 217 indicates the nominal media rotation rate of the device and is defined in table:
//        //          Value           Description
//        //          --------------------------------
//        //          0000h           Rate not reported
//        //          0001h           Non-rotating media (e.g., solid state device)
//        //          0002h-0400h     Reserved
//        //          0401h-FFFEh     Nominal media rotation rate in rotations per minute (rpm)
//        //                                  (e.g., 7 200 rpm = 1C20h)
//        //          FFFFh           Reserved
//#define kNominalMediaRotRateWordIndex 217
//        wprintf(L"%d", (UINT)id_query.data[kNominalMediaRotRateWordIndex]);
//    }
//    else
//    {
//        //Failed
//        int err = ::GetLastError();
//    }

    //NV_FEATURE_PARAMETER rpm;
    //NVCACHE_REQUEST_BLOCK req;
    //memset(&req, 0, sizeof(NVCACHE_REQUEST_BLOCK));
    //req.Function = NRB_FUNCTION_NVCACHE_INFO;
    //req.NRBSize = sizeof(NVCACHE_REQUEST_BLOCK);
    //
    //result = DeviceIoControl(drive, IOCTL_SCSI_MINIPORT,
    //                &req, sizeof(NVCACHE_REQUEST_BLOCK),
    //                (void*)&rpm, sizeof(NV_FEATURE_PARAMETER),
    //                &size, NULL);
    //if (!result)
    //{
    //    int error = GetLastError();
    //    return false;
    //}

    return !!dtd.TrimEnabled;
}


std::set<std::wstring> GetSSDDriveString()
{
    DWORD len = GetLogicalDriveStrings(0, nullptr);
    std::wstring drive_string(L"");
    drive_string.resize(len + 1);
    len = GetLogicalDriveStrings(len, &drive_string.front());

    bool found = false;
    std::set<std::wstring> drives;
    std::wstring name(L"");
    for (int i = 0; i < static_cast<int>(len); i++)
    {
        if (drive_string[i] == L'\0')
        {
            continue;
        }
        else
        {
            name = drive_string[i];
            name += L":";
            i += 2;
        }

        switch (GetDriveType(name.c_str()))
        {
            case DRIVE_FIXED:
            case DRIVE_REMOVABLE:
                break;
            default:
                continue;
                break;
        }

        name = GetPhysicalDriveString(name);
        if (!DetectsSSD(name))
            continue;

        drives.insert(name);
        name = L"";
    }

    return drives;
}