#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>

#include "CAngle.h"
#include "U_Types.h"

class IBinaryStream //TODO missing const
{
public:
    virtual void RawWrite(const void* data, size_t size) = 0;
    virtual void RawRead(void* data, size_t size) = 0;
    virtual bool is_eof();
    
    virtual ~IBinaryStream() = default;

    void ReadData(void* data, size_t size);
    std::unique_ptr<std::uint8_t[]> ReadData(size_t size);
    std::vector<std::uint8_t> ReadDataVec(size_t size);

    void WriteData(const void* data, size_t size);
    void WriteData(std::unique_ptr<std::uint8_t[]> data, size_t size);

    template<typename T>
    void Write(const T& val)
    {
        if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::wstring>)
        {
            WriteString<T>(val);
        }
        else if constexpr (std::is_same_v<T, CAngle>)
        {
            typename T::value_type degr = val.asRadians();
            RawWrite(&degr, sizeof(typename T::value_type));
        }
        else if constexpr (is_glm_vec_v<T> || is_glm_quat_v<T> || std::is_same_v<T, CAngles>)
        {
            for(size_t i = 0; i < T::length(); i++)
            {
                Write<typename T::value_type>(val[i]);
            }
        }
        else if constexpr (is_glm_mat_v<T>)
        {
            for(size_t y = 0; y < T::col_type::length(); y++)
            {
                for(size_t x = 0; x < T::length(); x++)
                {
                    typename T::value_type value = val[x][y];
                    RawWrite(&value, sizeof(float));
                }
            }
        }
        else
        {
            RawWrite(&val, sizeof(T));
        }
    }

    template<typename T>
    T Read()
    {
        T ret{};
        if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::wstring>)
        {
            ret = ReadString<T>();
        }
        else if constexpr (std::is_same_v<T, CAngle>)
        {
            typename T::value_type degr{};
            RawRead(&degr, sizeof(typename T::value_type));

            ret = T::radians(degr);
        }
        else if constexpr (is_glm_vec_v<T> || is_glm_quat_v<T> || std::is_same_v<T, CAngles>) //TODO why not reading the raw data
        {
            for(size_t i = 0; i < T::length(); i++)
            {
                typename T::value_type item{};
                RawRead(&item, sizeof(typename T::value_type));

                ret[i] = item;
            }
        }
        else if constexpr (is_glm_mat_v<T>) //TODO why not reading the raw data
        {
            for(size_t y = 0; y < T::col_type::length(); y++)
            {
                for(size_t x = 0; x < T::length(); x++)
                {
                    typename T::value_type value{};
                    RawRead(&value, sizeof(float));

                    ret[x][y] = value;
                }
            }
        }
        else
        {
            RawRead(&ret, sizeof(T));
        }
        return ret;
    }

    template<typename String = std::string>
    String ReadString(size_t fixedSize = 0)
    {
        constexpr typename String::value_type charnull = 0; //std::is_same_v<typename String::value_type, char> ? '\0' : L'\0';

        typename String::value_type reat = 1;
        String ret;

        if(fixedSize)
        {
            ret.resize(fixedSize);
            ReadData(static_cast<void*>(ret.data()), fixedSize * sizeof(typename String::value_type));
        }
        else
        {
            ret.reserve(260);

            for(;;)
            {
                reat = Read<typename String::value_type>();
                if (reat == charnull)
                {
                    break;
                }
                else
                {
                    ret += reat;
                }
            }
        }
        return ret;
    }

    template<typename String = std::string>
    void WriteString(const String& str, bool null_terminated = true)
    {
        constexpr typename String::value_type charnull = 0; //std::is_same_v<typename String::value_type, char> ? '\0' : L'\0';

        WriteData(str.data(), str.size());
        if (null_terminated) { Write(charnull); }
    }

    template<typename L = std::uint16_t, typename T = std::string>
    void WriteLenString(const T& str, bool writelen = true, L forcelen = 0U)
    {
        constexpr typename T::value_type charnull = 0; //std::is_same_v<C, char> ? '\0' : L'\0';

        if (writelen)
        {
            Write<L>((forcelen ? forcelen : L(str.size())));
        }

        if (forcelen)
        {
            L lenToWrite = std::min<L>(str.size(), forcelen);

            if (lenToWrite > 0)
            {
                WriteData(static_cast<void*>(const_cast<typename T::value_type*>(str.data())), lenToWrite * sizeof(typename T::value_type));
            }

            if (forcelen > lenToWrite)
            {
                std::vector<typename T::value_type> padding(forcelen - lenToWrite, charnull);
                WriteData(static_cast<void*>(padding.data()), padding.size() * sizeof(typename T::value_type));
            }
        }
        else
        {
            WriteData(static_cast<void*>(const_cast<typename T::value_type*>(str.data())), str.size() * sizeof(typename T::value_type));
        }
    }

    template<typename L = std::uint16_t, typename T = std::string>
    T ReadLenString(bool readlen = true, L forcelen = 0U, bool removenulls = true)
    {
        constexpr typename T::value_type charnull = 0; //std::is_same_v<C, char> ? '\0' : L'\0';

        T ret;
        typename T::value_type reat;
        L len = forcelen;

        if (readlen)
        {
            len = Read<L>();
        }

        ret.resize(len);
        ReadData(static_cast<void*>(ret.data()), len * sizeof(typename T::value_type));

        if (removenulls)
        {
            size_t pos = ret.find_last_not_of(charnull);
            if (pos != T::npos)
            {
                ret.resize(pos + 1);
            }
            else
            {
                ret.clear();
            }
        }
        return ret;
    }

    template<typename T>
    void Read(T& var)
    {
        var = Read<T>();
    }

    template<typename String = std::string>
    void ReadString(String& var, size_t fixedSize = 0)
    {
        var = ReadString<String>(fixedSize);
    }

    template<typename L = std::uint16_t, typename T = std::string>
    void ReadLenString(T& var, bool readlen = true, L forcelen = 0U, bool removenulls = true)
    {
        var = ReadLenString<L, T>(readlen, forcelen, removenulls);
    }
};
