#pragma once
#include "CComponent.h"

class CExecutor : public CComponent
{
public:
    bool V_Init() override;
    void V_PostInit() override;
    void V_DeInit() override;

    static void ExecuteFile(const std::string& file);
    static void WriteConfig(const std::string& file);
    static void WriteAliases(const std::string& file);

    DEFINE_COMPONENT();
};
