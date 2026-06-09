#pragma once
#include <string>

class CBuildInfo
{
public:
    static std::string GetBuildDate();
    static std::string GetBuildTime();

    static std::string GetCompiler();
    static std::string GetOperatingSystemName();
};
