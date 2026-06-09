#include "IBinaryStream.h"

void IBinaryStream::ReadData(void* data, size_t size)
{
    RawRead(data, size);
}

std::unique_ptr<std::uint8_t[]> IBinaryStream::ReadData(size_t size)
{
    std::unique_ptr<std::uint8_t[]> mem(new std::uint8_t[size]);
    RawRead(mem.get(), size);
    return mem;
}

std::vector<std::uint8_t> IBinaryStream::ReadDataVec(size_t size)
{
    std::vector<std::uint8_t> mem;
    mem.resize(size);
    RawRead(&mem[0], size);
    return mem;
}

void IBinaryStream::WriteData(const void* data, size_t size)
{
    RawWrite(data, size);
}

void IBinaryStream::WriteData(std::unique_ptr<std::uint8_t[]> data, size_t size)
{
    RawWrite(data.get(), size);
}

bool IBinaryStream::is_eof()
{
    return false;
}
