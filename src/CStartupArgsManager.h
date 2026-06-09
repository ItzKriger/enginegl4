#pragma once
#include "CComponent.h"
#include <map>
#include <vector>
#include "U_String.h"

class CStartupArgsManager : public CComponent
{
public:
    CStartupArgsManager();

    void SetupDefaults();
    void Parse();
    void ProcessCommands();
    
    void V_PostInit() override;

    bool IsArgumentSet(const std::string& arg);
    std::string GetArgumentValue(const std::string& arg);

    template<typename T>
    void GetIfExist(const std::string& arg, T& var)
    {
        if(!IsArgumentSet(arg)) { return; }

        std::string val = GetArgumentValue(arg);
        if(val.empty()) { return; }

        var = StringUtils::FromStr<T, std::string>(arg);
    }

    template<typename T>
    T WrapArgumentValue(const std::string& arg)
    {
        std::string val = GetArgumentValue(arg);
        if(val.empty()) { return T{}; }

        return StringUtils::FromStr<T, std::string>(arg);
    }

    std::map<std::string, std::string> Arguments;
    std::vector<std::string> Commands;

    DEFINE_COMPONENT();
};
