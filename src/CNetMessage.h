#pragma once
#include <string>
#include "U_Typedefs.h"
#include "CBufferWrapper.h"
#include "CTime.h"

#define LINK_NET_MESSAGE_TO_CLASS(_class, name) std::string _class::GetType() const { return #name; }; CFactoryInitter<_class, CNetMessage> __entity_initter_ ## name = CFactoryInitter<_class, CNetMessage>(#name, std::function<void(CFactoryInitter<_class, CNetMessage>*)>([](CFactoryInitter<_class, CNetMessage>* tter) -> void { tter->m_factory = &CEngine::GetInstance()->NetMessagesFactory; }));
#define DEFINE_NET_MESSAGE() std::string GetType() const override

class CNetMessage
{
public:
    virtual ~CNetMessage() = default;

    virtual std::string GetType() const;

    crc32_t Write(std::vector<std::uint8_t>& buffer, size_t offset = 0);
    crc32_t Read(std::vector<std::uint8_t>& buffer, size_t offset = 0);

    crc32_t Write(CBufferWrapper& wrapper);
    crc32_t Read(CBufferWrapper& wrapper);

    virtual void V_Write(CBufferWrapper& wrapper);
    virtual void V_Read(CBufferWrapper& wrapper);

    virtual void Process();

    size_t GetLastReadSize() const;
    size_t GetLastWriteSize() const;

    CTimePoint NetTime;
private:
    size_t m_lastReadSize = 0;
    size_t m_lastWriteSize = 0;
};
