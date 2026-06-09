#pragma once
#include <filesystem>
#include <fstream>

#include "IBinaryStream.h"

class CBinaryFile : public IBinaryStream
{
public:
    CBinaryFile() = default;
    CBinaryFile(const std::filesystem::path& filename, bool isread = true);
    ~CBinaryFile();

    bool OpenWrite(const std::filesystem::path& filename);
    bool OpenRead(const std::filesystem::path& filename);

    void Open(const std::filesystem::path& filename, bool isread = true);

    bool IsRead() const;
    bool IsWrite() const;

    void Close();
    bool IsOpen() const;

    void SetSeek(size_t seek);
    size_t GetSeek();
    size_t GetSize();

    void RawWrite(const void* data, size_t size) override;
    void RawRead(void* data, size_t size) override;
    bool is_eof() override;
private:
	std::fstream m_stream;
	bool m_isWrite = false;
};

class CFileGuard
{
public:
    CFileGuard() = delete;

    //doesn't open the file
    CFileGuard(CBinaryFile& file);
    //opens file with filename
    CFileGuard(CBinaryFile& file, const std::filesystem::path& filename, bool isread = true);

    ~CFileGuard();
private:
    CBinaryFile& m_File;
};

void TestBinaryFile();