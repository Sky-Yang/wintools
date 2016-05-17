#ifndef _WMI_ADAPTER_H_
#define _WMI_ADAPTER_H_

#include <list>
#include <map>
#include <string>
#include <vector>

class WmiAdapter
{
public:
    struct WmiResult
    {
        std::map<std::wstring, std::wstring> AttrsString;
        std::map<std::wstring, int> AttrsInt;
    };
    WmiAdapter();
    ~WmiAdapter();

    static bool Query(const std::wstring& wql, 
                      const std::vector<std::wstring> attrs,
                      std::list<WmiResult>* results);
};

#endif // _WMI_ADAPTER_H_