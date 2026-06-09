#pragma once
#include "CNetMessage.h"
#include "CWorld.h"
#include "U_TypeDefs.h"

class CNetChangeWorld : public CNetMessage
{
public:
    void V_Write(CBufferWrapper& wrapper) override;
    void V_Read(CBufferWrapper& wrapper) override;

    void Process() override;

    worldid_t WorldID = 0;
    DEFINE_NET_MESSAGE();
};

class CNetServerDisconnect : public CNetMessage //always unreliable
{
public:
    void V_Write(CBufferWrapper& wrapper) override;
    void V_Read(CBufferWrapper& wrapper) override;

    void Process() override;

    std::wstring Reason;
    DEFINE_NET_MESSAGE();
};

class CNetClientDisconnect : public CNetMessage //always unreliable
{
public:
    void V_Write(CBufferWrapper& wrapper) override;
    void V_Read(CBufferWrapper& wrapper) override;

    void Process() override;

    std::wstring Reason;
    DEFINE_NET_MESSAGE();
};

class CNetKeepAlive : public CNetMessage //always unreliable
{
public:
    void V_Write(CBufferWrapper& wrapper) override;
    void V_Read(CBufferWrapper& wrapper) override;

    void Process() override;

    DEFINE_NET_MESSAGE();
};
