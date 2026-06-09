#pragma once
#include <string>
#include <sstream>
#include <locale>
#include <codecvt>
#include <vector>
#include <algorithm>
#include <type_traits>
#include <regex>
#include <iostream>
#include <string_view>

#include <boost/locale.hpp>

#include "U_General.h"
#include "U_Types.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "CAngle.h"
#include "CColor.h"
#include "CColorInt.h"

namespace StringUtils
{
    std::wstring StrToWstr(const std::string& str);
    std::string WstrToStr(const std::wstring& wstr);

    std::u16string Utf8ToU16(const std::string& str);
    std::string U16ToUtf8(const std::u16string& str);
    std::u16string WstrToU16(const std::wstring& wstr);
    std::wstring U16ToWstr(const std::u16string& str);

    void split_str(const std::string& s, char delim, std::vector<std::string>& ua);
    void split_str(const std::wstring& s, wchar_t delim, std::vector<std::wstring>& ua);
    std::vector<std::string> split_str(const std::string& s, char delim);
    std::vector<std::wstring> split_str(const std::wstring& s, wchar_t delim);

    template<typename String = std::string>
    String RemoveSpaces(const String& str, size_t stayBegin = 0, size_t stayEnd = 0)
    {
        using C = std::conditional_t<std::is_same_v<String, std::string>, char, wchar_t>;
        constexpr C charspace = std::is_same_v<C, char> ? ' ' : L' ';
        constexpr C chartab = std::is_same_v<C, char> ? '\t' : L'\t';

        String ret = str;

        size_t spacesAtBegin = 0, spacesAtEnd = 0;
        for(auto it = str.begin(); it != str.end(); ++it)
        {
            if(*it == charspace || *it == chartab)
            {
                spacesAtBegin++;
            }
            else
            {
                break;
            }
        }

        for(auto it = str.rbegin(); it != str.rend(); ++it)
        {
            if(*it == charspace || *it == chartab)
            {
                spacesAtEnd++;
            }
            else
            {
                break;
            }
        }

        int diffBegin = static_cast<int>(spacesAtBegin) - static_cast<int>(stayBegin);
        int diffEnd = static_cast<int>(spacesAtEnd) - static_cast<int>(stayEnd);

        //3 - 1 = 2
        //1 - 1 = 0
        //3 - 4 = -1

        if(diffBegin < 0) { diffBegin = 0; }
        if(diffEnd < 0) { diffEnd = 0; }

        if (diffBegin > ret.size()) { diffBegin = ret.size(); }
        ret.erase(0, diffBegin);

        if (diffEnd > ret.size()) { diffEnd = ret.size(); }
        ret.erase(ret.size() - diffEnd, diffEnd);

        //mleb***
        //0123456 size = 7
        //diffEnd = 3
        //7 - 3 = 4

        return ret;
    }

    template<typename String = std::string>
    String SafeSubstring(const String& str, size_t count, size_t offset = 0)
    {
        //substr
        //0, 3 -> sub

        //substr
        //0, 6 -> substr
        
        //substr
        //1, 3 -> ubs

        //substr
        //4, 4 -> tr (6 - 4) = 2

        //substr
        //4, 5 -> tr (6 - 4) = 2

        if (str.empty()) { return String(); }

        size_t startIndex = offset;
        size_t endIndex = offset + count;

        if (startIndex >= str.size()) { return String(); }
        if (endIndex >= str.size()) { return str.substr(startIndex, str.size() - startIndex); }

        return str.substr(startIndex, count);
    }

    bool StartsWith(const std::string& str, const std::string& startswith);
    bool StartsWith(const std::wstring& str, const std::wstring& startswith);

    template<typename String = std::string>
    String merge_arg(const std::vector<String>& vect, size_t start, size_t end)
    {
        if (vect.empty()) { return {}; }
        String ret;

        for (size_t i = start; i <= end; ++i)
        {
            if constexpr (std::is_same_v<String, std::wstring>)
            {
                if (i != start) { ret += L" "; }
            }
            else
            {
                if (i != start) { ret += " "; }
            }
            ret += vect.at(i);
        }

        return ret;
    }

    template<typename String = std::string>
    void replace(String& str, const String& from, const String& to)
    {
        if (from.empty()) { return; }
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != String::npos)
        {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
    }

    template<typename String = std::string>
    bool hasEnding(const String& fullString, const String& ending)
    {
        if (fullString.length() >= ending.length())
        {
            return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
        }
        return false;
    }

    template<typename String = std::string>
    bool IsEmpty(const String& str)
    {
        if (str.empty()) { return true; }

        using C = std::conditional_t<std::is_same_v<String, std::string>, char, wchar_t>;
        constexpr C charspace = std::is_same_v<C, char> ? ' ' : L' ';
        constexpr C chartab = std::is_same_v<C, char> ? '\t' : L'\t';

        for (C b : str)
        {
            if (b != charspace && b != chartab)
            {
                return false;
            }
        }
        return true;
    }

    template <typename StringType>
    bool isNumber(const StringType& str)
    {
        if (str.empty()) { return false; }

        typename StringType::const_iterator it = str.begin();
        bool decimalPoint = false;
        bool hasDigits = false;

        typename StringType::value_type dotChar = '.';
        typename StringType::value_type minusChar = '-';

        if (*it == minusChar) { ++it; }
        
        for (; it != str.end(); ++it)
        {
            if (*it == dotChar)
            {
                if (decimalPoint)
                {
                    return false;
                }
                decimalPoint = true;
            }
            else if (!std::isdigit(*it, std::locale()))
            {
                return false;
            }
            else
            {
                hasDigits = true;
            }
        }

        return hasDigits;
    }

    void remove_consecutive_commas(std::string& str);
    void remove_consecutive_commas(std::wstring& str);

    namespace SequentVectorParse
    {
        template<typename String>
        std::vector<String> Split(String str)
        {
            std::vector<String> ret;

            if constexpr (std::is_same_v<String, std::string>)
            {
                replace(str, std::string(" "), std::string(","));
                replace(str, std::string(";"), std::string(","));
                replace(str, std::string("\t"), std::string(","));
                replace(str, std::string(":"), std::string(","));

                remove_consecutive_commas(str);
                split_str(str, ',', ret);
            }
            else if constexpr (std::is_same_v<String, std::wstring>)
            {
                replace(str, std::wstring(L" "), std::wstring(L","));
                replace(str, std::wstring(L";"), std::wstring(L","));
                replace(str, std::wstring(L"\t"), std::wstring(L","));
                replace(str, std::wstring(L":"), std::wstring(L","));

                remove_consecutive_commas(str);
                split_str(str, L',', ret);
            }

            return ret;
        }

        template<typename Vec, typename String = std::string>
        void Parse(Vec& vec, const String& str)
        {
            using sstream = std::conditional_t<std::is_same_v<String, std::string>, std::stringstream, std::wstringstream>;
            auto splitted = Split(str);

            for (auto i = 0; i < vec.length() && i < splitted.size(); i++)
            {
                typename Vec::value_type single;
                sstream ss(splitted[i]);

                ss >> single;

                vec[i] = single;
            }
        }
    }

    template<typename T, typename String = std::string>
    T FromStr(const String& str) //TODO glm::mat
    {
        using sstream = std::conditional_t<std::is_same_v<String, std::string>, std::stringstream, std::wstringstream>;

        constexpr bool is_this_string = std::is_same_v<String, std::string>;
        constexpr bool is_this_wstring = std::is_same_v<String, std::wstring>;

        constexpr bool is_char = std::is_same<T, char>::value || std::is_same<T, unsigned char>::value;
        constexpr bool is_string = std::is_same<T, std::string>::value;
        constexpr bool is_wstring = std::is_same<T, std::wstring>::value;
        constexpr bool is_angle = std::is_same<T, CAngle>::value;
        constexpr bool is_angles = std::is_same<T, CAngles>::value;
        constexpr bool is_color = std::is_same<T, CColor>::value;
        constexpr bool is_colorint = std::is_same<T, CColorInt>::value;
        constexpr bool is_anycolor = is_color || is_colorint;

        constexpr bool is_quat = are_same_template<T, glm::quat>::value;
        constexpr bool is_vec2 = are_same_template<T, glm::vec2>::value;
        constexpr bool is_vec3 = are_same_template<T, glm::vec3>::value;
        constexpr bool is_vec4 = are_same_template<T, glm::vec4>::value;
        constexpr bool is_ivec2 = are_same_template<T, glm::ivec2>::value;
        constexpr bool is_ivec3 = are_same_template<T, glm::ivec3>::value;
        constexpr bool is_ivec4 = are_same_template<T, glm::ivec4>::value;
        constexpr bool is_uvec2 = are_same_template<T, glm::uvec2>::value;
        constexpr bool is_uvec3 = are_same_template<T, glm::uvec3>::value;
        constexpr bool is_uvec4 = are_same_template<T, glm::uvec4>::value;

        constexpr bool is_vec_or_quat = is_glm_quat_v<T> || is_glm_vec_v<T>;

        constexpr bool is_bool = std::is_same<T, bool>::value;

        constexpr bool is_int8 = std::is_same<T, std::int8_t>::value;
        constexpr bool is_uint8 = std::is_same<T, std::uint8_t>::value;

        sstream ss(str);
        if constexpr (is_int8 || is_uint8)
        {
            int ret;
            ss >> ret;
            return ret;
        }
        else if constexpr (is_wstring && is_this_string)
        {
            return StrToWstr(str);
        }
        else if constexpr (is_string && is_this_wstring)
        {
            return WstrToStr(str);
        }
        else if constexpr (is_bool)
        {
            String str_lower = str;

            if constexpr (is_this_string)
            {
                std::transform(str_lower.begin(), str_lower.end(), str_lower.begin(), ::tolower);

                if (str_lower == "true" || str_lower == "yes")
                {
                    return true;
                }
            }
            else
            {
                std::transform(str_lower.begin(), str_lower.end(), str_lower.begin(), ::towlower);

                if (str_lower == L"true" || str_lower == L"yes")
                {
                    return true;
                }
            }

            int num;
            float fl_num;

            ss >> num;
            if (ss.fail())
            {
                ss.clear();
                ss.str(str);

                ss >> fl_num;

                if (ss.fail() || std::isnan(fl_num))
                {
                    return false;
                }
                else
                {
                    return (fl_num > 0.0f);
                }
            }
            else
            {
                return (num > 0);
            }
        }
        else if constexpr (is_vec_or_quat || is_angles || is_anycolor)
        {
            T ret{};
            SequentVectorParse::Parse(ret, str);

            return ret;
        }
        else if constexpr (is_angle)
        {
            float angl;
            ss >> angl;
            return CAngle::radians(angl);
        }
        else
        {
            T ret;
            ss >> ret;
            return ret;
        }
    }

    template<typename String = std::string, typename T>
    String ToStr(const T& val) //TODO glm::mat
    {
        using sstream = std::conditional_t<std::is_same_v<String, std::string>, std::stringstream, std::wstringstream>;

        constexpr bool is_int8 = std::is_same<T, std::int8_t>::value;
        constexpr bool is_uint8 = std::is_same<T, std::uint8_t>::value;

        constexpr bool is_this_string = std::is_same_v<String, std::string>;
        constexpr bool is_this_wstring = std::is_same_v<String, std::wstring>;

        constexpr bool is_char = std::is_same<T, char>::value || std::is_same<T, unsigned char>::value;
        constexpr bool is_double = std::is_floating_point<T>::value;
        constexpr bool is_string = std::is_same<T, std::string>::value;
        constexpr bool is_wstring = std::is_same<T, std::wstring>::value;
        constexpr bool is_angle = std::is_same<T, CAngle>::value;
        constexpr bool is_angles = std::is_same<T, CAngles>::value;
        
        constexpr bool is_quat = are_same_template<T, glm::quat>::value;
        constexpr bool is_vec2 = are_same_template<T, glm::vec2>::value;
        constexpr bool is_vec3 = are_same_template<T, glm::vec3>::value;
        constexpr bool is_vec4 = are_same_template<T, glm::vec4>::value;
        constexpr bool is_ivec2 = are_same_template<T, glm::ivec2>::value;
        constexpr bool is_ivec3 = are_same_template<T, glm::ivec3>::value;
        constexpr bool is_ivec4 = are_same_template<T, glm::ivec4>::value;
        constexpr bool is_uvec2 = are_same_template<T, glm::uvec2>::value;
        constexpr bool is_uvec3 = are_same_template<T, glm::uvec3>::value;
        constexpr bool is_uvec4 = are_same_template<T, glm::uvec4>::value;

        constexpr bool is_color = std::is_same<T, CColor>::value;
        constexpr bool is_colorint = std::is_same<T, CColorInt>::value;
        constexpr bool is_anycolor = is_color || is_colorint;

        constexpr bool is_vec_or_quat = is_glm_quat_v<T> || is_glm_vec_v<T>;

        if constexpr (is_char)
        {
            int v = val;
            
            sstream ss;
            ss << v;
            return ss.str();
        }
        else if constexpr (is_wstring && is_this_string)
        {
            return WstrToStr(val);
        }
        else if constexpr (is_string && is_this_wstring)
        {
            return StrToWstr(val);
        }
        else if constexpr (is_angle)
        {
            sstream strs;
            strs.precision(std::numeric_limits<T>::digits10);
            strs << std::fixed << val.asRadians();
            return strs.str();
        }
        else if constexpr (is_vec_or_quat || is_angles || is_anycolor)
        {
            sstream strs;

            for (int i = 0; i < val.length(); i++)
            {
                if constexpr (std::is_floating_point<typename T::value_type>::value)
                {
                    strs.precision(std::numeric_limits<typename T::value_type>::digits10);
                    strs << std::fixed;
                }

                strs << val[i];

                if (i != (val.length() - 1))
                {
                    if constexpr (is_this_wstring)
                    {
                        strs << L' ';
                    }
                    else
                    {
                        strs << ' ';
                    }
                }
            }

            return strs.str();
        }
        else if constexpr (is_double)
        {
            sstream strs;
            strs.precision(std::numeric_limits<T>::digits10);
            strs << std::fixed << val;
            return strs.str();
        }
        else
        {
            sstream ss;
            ss << val;
            return ss.str();
        }
    }
}

namespace CommandPreprocess
{
    void Trim(std::wstring& str);
    void PreprocessEscapes(std::wstring& str);
    void ReplaceEscapes(std::wstring& str);
    std::vector<std::wstring> SplitCommands(const std::wstring& input);
    std::vector<std::wstring> ParseArguments(const std::wstring& command);
    std::vector<std::vector<std::wstring>> PreProcessString(std::wstring cmdline);
}

namespace ArgumentsPreprocess
{
    void ReplaceEscapes(std::string& str);
    std::vector<std::string> ParseArguments(std::string cmdline);
    bool Compare(const std::string& str, const std::string& op, const std::string& arg);
}
