#include "CNetMessage.h"
#include "boost/crc.hpp"

std::string CNetMessage::GetType() const { return {}; }

crc32_t CNetMessage::Write(std::vector<std::uint8_t>& buffer, size_t offset)
{
    CBufferWrapper wrapper(buffer);
    wrapper.SetSeek(offset);

    return Write(wrapper);
}

crc32_t CNetMessage::Read(std::vector<std::uint8_t>& buffer, size_t offset)
{
    CBufferWrapper wrapper(buffer);
    wrapper.SetSeek(offset);

    return Read(wrapper);
}

crc32_t CNetMessage::Write(CBufferWrapper& wrapper)
{
    size_t startOffset = wrapper.GetSeek();
    V_Write(wrapper);

    size_t endOffset = wrapper.GetSeek();
    m_lastWriteSize = endOffset - startOffset;

    //startOffset 3
    //endOffset 6
    //start: 3, size 6-3 = 3
    //0 1 2 3 4 5
    //      x x x

    boost::crc_32_type result;
    result.process_bytes(&wrapper.GetBuffer()[startOffset], endOffset - startOffset);

    return result.checksum();
}

crc32_t CNetMessage::Read(CBufferWrapper& wrapper)
{
    size_t startOffset = wrapper.GetSeek();
    V_Read(wrapper);

    size_t endOffset = wrapper.GetSeek();
    m_lastReadSize = endOffset - startOffset;

    boost::crc_32_type result;
    result.process_bytes(&wrapper.GetBuffer()[startOffset], endOffset - startOffset);

    return result.checksum();
}

size_t CNetMessage::GetLastReadSize() const
{
    return m_lastReadSize;
}

size_t CNetMessage::GetLastWriteSize() const
{
    return m_lastWriteSize;
}

void CNetMessage::V_Write(CBufferWrapper& wrapper) {}
void CNetMessage::V_Read(CBufferWrapper& wrapper) {}
void CNetMessage::Process() {}
