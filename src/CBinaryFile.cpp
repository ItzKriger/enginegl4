#include "CBinaryFile.h"

#include "CEngine.h"
#include "U_Log.h"
#include "CCommandProcessor.h"
#include <iostream>

CBinaryFile::CBinaryFile(const std::filesystem::path& filename, bool isread)
{
    isread ? OpenRead(filename) : OpenWrite(filename);
}

CBinaryFile::~CBinaryFile()
{
    Close();
}

bool CBinaryFile::OpenWrite(const std::filesystem::path& filename)
{
    Close();
    m_stream.open(filename, std::ios::binary | std::ios::out);

    m_isWrite = true;
    return IsOpen();
}

bool CBinaryFile::OpenRead(const std::filesystem::path& filename)
{
    Close();
    m_stream.open(filename, std::ios::binary | std::ios::in);

    m_isWrite = false;
    return IsOpen();
}

void CBinaryFile::Close()
{
    if(!IsOpen()) { return; }
    m_stream.close();
}

bool CBinaryFile::IsOpen() const
{
    return m_stream.is_open();
}

void CBinaryFile::SetSeek(size_t seek)
{
    if (!IsOpen()) { return; }
    m_stream.seekp(seek, std::ios_base::beg);
}

size_t CBinaryFile::GetSeek()
{
    if(!IsOpen()) { return 0; }
    return m_stream.tellp();
}

size_t CBinaryFile::GetSize()
{
    if (!IsOpen()) { return 0; }

    size_t oldseek = GetSeek();
    size_t ret = 0;

    m_stream.seekp(0, std::ios_base::end);
    ret = m_stream.tellp();

    SetSeek(oldseek);
    return ret;
}

void CBinaryFile::RawWrite(const void* data, size_t size)
{
    if(!IsOpen() || IsRead()) { return; }
    m_stream.write((char*)data, size);
}

void CBinaryFile::RawRead(void* data, size_t size)
{
    if(!IsOpen() || IsWrite() || GetSeek() + size > GetSize()) { return; }
    m_stream.read((char*)data, size);
}

bool CBinaryFile::IsRead() const
{
    return !m_isWrite;
}

bool CBinaryFile::IsWrite() const
{
    return m_isWrite;
}

bool CBinaryFile::is_eof()
{
    return IsOpen() && GetSeek() <= GetSize();
}

void CBinaryFile::Open(const std::filesystem::path& filename, bool isread)
{
    isread ? OpenRead(filename) : OpenWrite(filename);
}

CFileGuard::CFileGuard(CBinaryFile& file) : m_File(file) {}
CFileGuard::CFileGuard(CBinaryFile& file, const std::filesystem::path& filename, bool isread) : m_File(file) { m_File.Open(filename, isread); }
CFileGuard::~CFileGuard()
{
    if(m_File.IsOpen())
    {
        m_File.Close();
    }
}

class CBinaryFileTester : public CCustomCommandProcessor
{
public:
    CBinaryFileTester()
    {
        Log::Instance() << "Welcome to the binary file tester\n";
    }

    CBinaryFile File;

    template<typename T>
    void Write(const std::wstring& val)
    {
        File.Write<T>(StringUtils::FromStr<T>(val));
    }

    template<typename T>
    std::wstring Read()
    {
        return StringUtils::ToStr<std::wstring, T>(File.Read<T>());
    }

    void WriteByType(const CCommandArgsWrapper& args, size_t curindex)
    {
        std::wstring type = args[curindex];
        std::wstring val = args[curindex + 1];

        if(type == L"int8") { Write<std::int8_t>(val); }
        else if(type == L"uint8") { Write<std::uint8_t>(val); }
        else if(type == L"int16") { Write<std::int16_t>(val); }
        else if(type == L"uint16") { Write<std::uint16_t>(val); }
        else if(type == L"int32") { Write<std::int32_t>(val); }
        else if(type == L"uint32") { Write<std::uint32_t>(val); }
        else if(type == L"int64") { Write<std::int64_t>(val); }
        else if(type == L"uint64") { Write<std::uint64_t>(val); }
        else if(type == L"float") { Write<float>(val); }
        else if(type == L"double") { Write<double>(val); }
        else if(type == L"bool") { Write<bool>(val); }
        else if(type == L"angle") { Write<CAngle>(val); }
        else if(type == L"angles") { Write<CAngles>(val); }
        else if(type == L"vec1") { Write<glm::vec1>(val); }
        else if(type == L"vec2") { Write<glm::vec2>(val); }
        else if(type == L"vec3") { Write<glm::vec3>(val); }
        else if(type == L"vec4") { Write<glm::vec4>(val); }
        else if(type == L"quat") { Write<glm::quat>(val); }
        else if(type == L"str" || type == L"string") { File.WriteString<std::string>(StringUtils::WstrToStr(val)); }
        else if(type == L"wstr" || type == L"wstring") { File.WriteString<std::wstring>(val); }
        else if(type == L"lenstr8") { File.WriteLenString<std::uint8_t, std::string>(StringUtils::WstrToStr(val)); }
        else if(type == L"lenstr16") { File.WriteLenString<std::uint16_t, std::string>(StringUtils::WstrToStr(val)); }
        else if(type == L"lenstr32") { File.WriteLenString<std::uint32_t, std::string>(StringUtils::WstrToStr(val)); }
        else if(type == L"lenstr64") { File.WriteLenString<std::uint64_t, std::string>(StringUtils::WstrToStr(val)); }
        else if(type == L"lenstr8force") { File.WriteLenString<std::uint8_t, std::string>(StringUtils::WstrToStr(val), true, StringUtils::FromStr<size_t>(args[curindex + 2])); }
        else if(type == L"lenstr16force") { File.WriteLenString<std::uint16_t, std::string>(StringUtils::WstrToStr(val), true, StringUtils::FromStr<size_t>(args[curindex + 2])); }
        else if(type == L"lenstr32force") { File.WriteLenString<std::uint32_t, std::string>(StringUtils::WstrToStr(val), true, StringUtils::FromStr<size_t>(args[curindex + 2])); }
        else if(type == L"lenstr64force") { File.WriteLenString<std::uint64_t, std::string>(StringUtils::WstrToStr(val), true, StringUtils::FromStr<size_t>(args[curindex + 2])); }
        else if(type == L"lenwstr8") { File.WriteLenString<std::uint8_t, std::wstring>(val); }
        else if(type == L"lenwstr16") { File.WriteLenString<std::uint16_t, std::wstring>(val); }
        else if(type == L"lenwstr32") { File.WriteLenString<std::uint32_t, std::wstring>(val); }
        else if(type == L"lenwstr64") { File.WriteLenString<std::uint64_t, std::wstring>(val); }
        else if(type == L"lenwstr8force") { File.WriteLenString<std::uint8_t, std::wstring>(val, true, StringUtils::FromStr<size_t>(args[curindex + 2])); }
        else if(type == L"lenwstr16force") { File.WriteLenString<std::uint16_t, std::wstring>(val, true, StringUtils::FromStr<size_t>(args[curindex + 2])); }
        else if(type == L"lenwstr32force") { File.WriteLenString<std::uint32_t, std::wstring>(val, true, StringUtils::FromStr<size_t>(args[curindex + 2])); }
        else if(type == L"lenwstr64force") { File.WriteLenString<std::uint64_t, std::wstring>(val, true, StringUtils::FromStr<size_t>(args[curindex + 2])); }
        else { Log::Instance() << "Invalid type\n"; }
    }

    std::wstring ReadByType(const CCommandArgsWrapper& args, size_t curindex)
    {
        std::wstring type = args[curindex];
        std::wstring val = args[curindex + 1];
        std::wstring ret;

        if(type == L"int8") { ret = StringUtils::ToStr<std::wstring>(Read<std::int8_t>()); }
        else if(type == L"uint8") { ret = StringUtils::ToStr<std::wstring>(Read<std::uint8_t>()); }
        else if(type == L"int16") { ret = StringUtils::ToStr<std::wstring>(Read<std::int16_t>()); }
        else if(type == L"uint16") { ret = StringUtils::ToStr<std::wstring>(Read<std::uint16_t>()); }
        else if(type == L"int32") { ret = StringUtils::ToStr<std::wstring>(Read<std::int32_t>()); }
        else if(type == L"uint32") { ret = StringUtils::ToStr<std::wstring>(Read<std::uint32_t>()); }
        else if(type == L"int64") { ret = StringUtils::ToStr<std::wstring>(Read<std::int64_t>()); }
        else if(type == L"uint64") { ret = StringUtils::ToStr<std::wstring>(Read<std::uint64_t>()); }
        else if(type == L"float") { ret = StringUtils::ToStr<std::wstring>(Read<float>()); }
        else if(type == L"double") { ret = StringUtils::ToStr<std::wstring>(Read<double>()); }
        else if(type == L"bool") { ret = StringUtils::ToStr<std::wstring>(Read<bool>()); }
        else if(type == L"angle") { ret = StringUtils::ToStr<std::wstring>(Read<CAngle>()); }
        else if(type == L"angles") { ret = StringUtils::ToStr<std::wstring>(Read<CAngles>()); }
        else if(type == L"vec1") { ret = StringUtils::ToStr<std::wstring>(Read<glm::vec1>()); }
        else if(type == L"vec2") { ret = StringUtils::ToStr<std::wstring>(Read<glm::vec2>()); }
        else if(type == L"vec3") { ret = StringUtils::ToStr<std::wstring>(Read<glm::vec3>()); }
        else if(type == L"vec4") { ret = StringUtils::ToStr<std::wstring>(Read<glm::vec4>()); }
        else if(type == L"quat") { ret = StringUtils::ToStr<std::wstring>(Read<glm::quat>()); }
        else if(type == L"str" || type == L"string") { ret = StringUtils::ToStr<std::wstring>(File.ReadString<std::string>()); }
        else if(type == L"wstr" || type == L"wstring") { ret = File.ReadString<std::wstring>(); }
        else if(type == L"lenstr8") { ret = StringUtils::ToStr<std::wstring>(File.ReadLenString<std::uint8_t, std::string>()); }
        else if(type == L"lenstr16") { ret = StringUtils::ToStr<std::wstring>(File.ReadLenString<std::uint16_t, std::string>()); }
        else if(type == L"lenstr32") { ret = StringUtils::ToStr<std::wstring>(File.ReadLenString<std::uint32_t, std::string>()); }
        else if(type == L"lenstr64") { ret = StringUtils::ToStr<std::wstring>(File.ReadLenString<std::uint64_t, std::string>()); }
        else if(type == L"lenwstr8") { ret = File.ReadLenString<std::uint8_t, std::wstring>(); }
        else if(type == L"lenwstr16") { ret = File.ReadLenString<std::uint16_t, std::wstring>(); }
        else if(type == L"lenwstr32") { ret = File.ReadLenString<std::uint32_t, std::wstring>(); }
        else if(type == L"lenwstr64") { ret = File.ReadLenString<std::uint64_t, std::wstring>(); }
        else { Log::Instance() << "Invalid type\n"; }

        return ret;
    }

    void m_process(const std::string& cmd, const CCommandArgsWrapper& args, const CCommandSender& sender) override
    {
        if(cmd == "open")
        {
            if(args[0] == L"write")
            {
                File.OpenWrite(args.WrapArgument<std::string>(1));
            }
            else if(args[0] == L"read")
            {
                File.OpenRead(args.WrapArgument<std::string>(1));
            }
            Log::Instance() << (File.IsOpen() ? "Opened file\n" : "Not opened file\n");
        }
        else if(cmd == "close")
        {
            File.Close();
            Log::Instance() << (File.IsOpen() ? "File wasn't closed\n" : "File isn't opened now\n");
        }
        else if(cmd == "size")
        {
            Log::Instance() << "Size is " << File.GetSize() << "\n";
        }
        else if(cmd == "seek")
        {
            bool isnum = !args[0].empty() && StringUtils::isNumber(args[0]);
            if(!isnum)
            {
                Log::Instance() << "Seek is " << File.GetSeek() << "\n";
            }
            else
            {
                File.SetSeek(StringUtils::FromStr<size_t>(args[0]));
                Log::Instance() << "Set seek to " << File.GetSeek() << "\n";
            }
        }
        else if(cmd == "mode")
        {
            if(!File.IsOpen()) { Log::Instance() << "Not opened\n"; }
            else
            {
                Log::Instance() << (File.IsWrite() ? "Write mode\n" : "Read mode\n");
            }
        }
        else if(cmd == "eof")
        {
            Log::Instance() << (File.is_eof() ? "Eof is true\n" : "NOT eof\n");
        }
        else if(cmd == "write")
        {
            WriteByType(args, 0);
            Log::Instance() << "Written\n";
        }
        else if(cmd == "read")
        {
            std::wstring ret = ReadByType(args, 0);
            Log::Instance() << "Read result: \"" << ret << "\"\n";
        }
    }
};

void TestBinaryFile()
{
    COMPONENT_CALL(CCommandProcessor, AddCustomProcessor(std::make_unique<CBinaryFileTester>()));
}
