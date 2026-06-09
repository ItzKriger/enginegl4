#pragma once
#include "IBinaryStream.h"

template<typename Unit = std::uint8_t, typename Container = std::vector<Unit>>
class CBufferWrapper_impl : public IBinaryStream
{
public:
    CBufferWrapper_impl() = delete;
    CBufferWrapper_impl(Container& cont) : m_buffer(cont) {}

    void RawWrite(const void* data, size_t size) override
    {
        //10 size
        //15 data size
        //seek 5
        //would be end at 20

        //10 written
        //5 + 15 = 20

        //end = 5 + 15

        if(m_seek >= m_buffer.size() || (m_seek + size) > m_buffer.size())
        {
            m_buffer.resize(m_seek + size);
        }

        memcpy(&m_buffer[m_seek], data, size);
        if (m_movingSeek) { m_seek += size; }
    }

    void RawRead(void* data, size_t size) override
    {

        //seek = 1
        //bufsize = 33
        //size = 32
        //seek + size = 33

        if(m_seek >= m_buffer.size() || (m_seek + size) > m_buffer.size())
        {
            return;
        }

        memcpy(data, &m_buffer[m_seek], size);
        if (m_movingSeek) { m_seek += size; }
    }

    bool is_eof() override
    {
        return m_seek >= m_buffer.size();
    }

    void SetSeek(size_t seek) { m_seek = seek; }
    size_t GetSeek() { return m_seek; }
    size_t GetSize() { return m_buffer.size(); }

    void SetAutoSeek(bool enable = true) { m_movingSeek = enable; }
    bool GetAutoSeek() { return m_movingSeek; }

    Container& GetBuffer() { return m_buffer; }
private:
    size_t m_seek = 0;
    bool m_movingSeek = true;

    Container& m_buffer;
};

using CBufferWrapper = CBufferWrapper_impl<std::uint8_t, std::vector<std::uint8_t>>;