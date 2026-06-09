#pragma once
#include "CNetMessage.h"
#include "CColorInt.h"

class CNetAlert : public CNetMessage
{
public:
    void V_Write(CBufferWrapper& wrapper) override;
    void V_Read(CBufferWrapper& wrapper) override;

    void Process() override;

    std::wstring String;
    CColorInt TextColor, BackgroundColor;
    
    DEFINE_NET_MESSAGE();
};
