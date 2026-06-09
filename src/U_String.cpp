#include "U_String.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <cwctype>

std::wstring StringUtils::StrToWstr(const std::string& str)
{
    /*std::vector<wchar_t> buf(str.size());
    std::use_facet<std::ctype<wchar_t>>(std::locale{}).widen(str.data(), str.data() + str.size(), buf.data());

    return std::wstring(buf.data(), buf.size());*/

    return boost::locale::conv::utf_to_utf<wchar_t>(str);
}

std::string StringUtils::WstrToStr(const std::wstring& wstr)
{
    /*std::vector<char> buf(wstr.size());
    std::use_facet<std::ctype<wchar_t>>(std::locale{}).narrow(wstr.data(), wstr.data() + wstr.size(), '?', buf.data());

    return std::string(buf.data(), buf.size());*/

    return boost::locale::conv::utf_to_utf<char>(wstr);
}

std::u16string StringUtils::Utf8ToU16(const std::string& str)
{
    return boost::locale::conv::utf_to_utf<char16_t>(str);
}

std::string StringUtils::U16ToUtf8(const std::u16string& str)
{
    return boost::locale::conv::utf_to_utf<char>(str);
}

std::u16string StringUtils::WstrToU16(const std::wstring& wstr)
{
    return boost::locale::conv::utf_to_utf<char16_t>(wstr);
}

std::wstring StringUtils::U16ToWstr(const std::u16string& str)
{
    return boost::locale::conv::utf_to_utf<wchar_t>(str);
}

bool StringUtils::StartsWith(const std::string& str, const std::string& startswith)
{
    return StringUtils::SafeSubstring(str, startswith.size()) == startswith;
}

bool StringUtils::StartsWith(const std::wstring& str, const std::wstring& startswith)
{
    return StringUtils::SafeSubstring(str, startswith.size()) == startswith;
}

void StringUtils::split_str(const std::string& s, char delim, std::vector<std::string>& ua)
{
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (true)
    {
        if (!ss)
        {
            return;
        }

        if (ss.eof())
        {
            break;
        }

        std::getline(ss, item, delim);
        ua.push_back(item);
    }
}

void StringUtils::split_str(const std::wstring& s, wchar_t delim, std::vector<std::wstring>& ua)
{
    std::wstringstream ss;
    ss.str(s);
    std::wstring item;
    while (true)
    {
        if (!ss)
        {
            return;
        }

        if (ss.eof())
        {
            break;
        }

        std::getline(ss, item, delim);
        ua.push_back(item);
    }
}

std::vector<std::string> StringUtils::split_str(const std::string& s, char delim)
{
    std::vector<std::string> ret;
    split_str(s, delim, ret);
    return ret;
}

std::vector<std::wstring> StringUtils::split_str(const std::wstring& s, wchar_t delim)
{
    std::vector<std::wstring> ret;
    split_str(s, delim, ret);
    return ret;
}

void StringUtils::remove_consecutive_commas(std::string& str)
{
    std::regex comma_pattern(",{2,}");
    str = std::regex_replace(str, comma_pattern, ",");
}

void StringUtils::remove_consecutive_commas(std::wstring& str)
{
    std::wregex comma_pattern(L",{2,}");
    str = std::regex_replace(str, comma_pattern, L",");
}

void CommandPreprocess::Trim(std::wstring& str)
{
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](wchar_t c) { return !std::iswspace(c); }));
    str.erase(std::find_if(str.rbegin(), str.rend(), [](wchar_t c) { return !std::iswspace(c); }).base(), str.end());
}

void CommandPreprocess::PreprocessEscapes(std::wstring& str)
{
    std::wstring result;
    for (size_t i = 0; i < str.size(); ++i)
    {
        if (str[i] == L'\\' && i + 1 < str.size())
        {
            if (str[i + 1] == L';')
            {
                result += L'\x1F';
                ++i;
            }
            else
            {
                result += str[i];
                result += str[i + 1];
                ++i;
            }
        }
        else
        {
            result += str[i];
        }
    }
    str = result;
}

void CommandPreprocess::ReplaceEscapes(std::wstring& str)
{
    std::wstring result;
    for (size_t i = 0; i < str.size(); ++i)
    {
        if (str[i] == L'\x1F')
        {
            result += L';';
        }
        else if (str[i] == L'\\' && i + 1 < str.size())
        {
            switch (str[i + 1])
            {
                case L'\\': result += L'\\'; ++i; break;
                case L'n': result += L'\n'; ++i; break;
                case L'r': result += L'\r'; ++i; break;
                case L'\"': result += L'\"'; ++i; break;
                default: result += str[i]; break;
            }
        }
        else
        {
            result += str[i];
        }
    }
    str = result;
}

std::vector<std::wstring> CommandPreprocess::SplitCommands(const std::wstring& input)
{
    std::vector<std::wstring> commands;
    std::wstring current;
    bool inString = false;

    for (size_t i = 0; i < input.size(); ++i)
    {
        if (input[i] == L'\"' && (i == 0 || input[i - 1] != L'\\'))
        {
            inString = !inString;
        }
        else if (input[i] == L';' && !inString)
        {
            Trim(current);
            if (!current.empty())
            {
                commands.push_back(current);
                current.clear();
            }
            continue;
        }
        current += input[i];
    }

    Trim(current);
    if (!current.empty())
    {
        commands.push_back(current);
    }

    return commands;
}

std::vector<std::wstring> CommandPreprocess::ParseArguments(const std::wstring& command)
{
    std::vector<std::wstring> arguments;
    std::wstring current;
    bool inString = false;

    for (size_t i = 0; i < command.size(); ++i)
    {
        if (command[i] == L'\"' && (i == 0 || command[i - 1] != L'\\'))
        {
            inString = !inString;
            continue;
        }

        if (std::iswspace(command[i]) && !inString)
        {
            if (!current.empty())
            {
                ReplaceEscapes(current);
                arguments.push_back(current);
                current.clear();
            }
            continue;
        }

        current += command[i];
    }

    if (!current.empty())
    {
        ReplaceEscapes(current);
        arguments.push_back(current);
    }

    return arguments;
}

std::vector<std::vector<std::wstring>> CommandPreprocess::PreProcessString(std::wstring cmdline)
{
    Trim(cmdline);
    if (cmdline.empty())
    {
        return {};
    }

    PreprocessEscapes(cmdline);

    std::vector<std::vector<std::wstring>> result;
    std::vector<std::wstring> commands = SplitCommands(cmdline);

    for (auto& cmd : commands)
    {
        std::vector<std::wstring> args = ParseArguments(cmd);
        if (!args.empty())
        {
            result.push_back(args);
        }
    }

    return result;
}

void ArgumentsPreprocess::ReplaceEscapes(std::string& str)
{
    std::string result;
    for (size_t i = 0; i < str.size(); ++i)
    {
        if (str[i] == '\\' && i + 1 < str.size())
        {
            switch (str[i + 1])
            {
                case '\\': result += '\\'; ++i; break;
                case 'n': result += '\n'; ++i; break;
                case 'r': result += '\r'; ++i; break;
                case 't': result += '\t'; ++i; break;
                case '\"': result += '\"'; ++i; break;
                default: result += str[i]; break;
            }
        }
        else
        {
            result += str[i];
        }
    }
    str = result;
}

std::vector<std::string> ArgumentsPreprocess::ParseArguments(std::string cmdline)
{
    std::vector<std::string> arguments;
    std::string current;
    bool inString = false;

    for (size_t i = 0; i < cmdline.size(); ++i)
    {
        if (cmdline[i] == '\"' && (i == 0 || cmdline[i - 1] != '\\'))
        {
            inString = !inString;
            continue;
        }

        if ((cmdline[i] == ' ' || cmdline[i] == '\t') && !inString)
        {
            if (!current.empty())
            {
                ReplaceEscapes(current);
                arguments.push_back(current);
                current.clear();
            }
            continue;
        }

        current += cmdline[i];
    }

    if (!current.empty())
    {
        ReplaceEscapes(current);
        arguments.push_back(current);
    }

    return arguments;
}

bool ArgumentsPreprocess::Compare(const std::string& str, const std::string& op, const std::string& arg) //TODO ugly code
{
    if (op.empty()) { return false; }

    if (StringUtils::isNumber(str))
    {
        double numstr = StringUtils::FromStr<double>(str);
        double numarg = StringUtils::FromStr<double>(arg);

        if (op == "==")
        {
            return numstr == numarg;
        }
        else if (op == ">")
        {
            return numstr > numarg;
        }
        else if (op == "<")
        {
            return numstr < numarg;
        }
        else if (op == ">=")
        {
            return numstr >= numarg;
        }
        else if (op == "<=")
        {
            return numstr <= numarg;
        }
        else if (op == "!=")
        {
            return numstr != numarg;
        }
        else
        {
            return false;
        }
    }

    if (op == "==")
    {
        return str == arg;
    }
    else if (op == ">")
    {
        return str > arg;
    }
    else if (op == "<")
    {
        return str < arg;
    }
    else if (op == ">=")
    {
        return str >= arg;
    }
    else if (op == "<=")
    {
        return str <= arg;
    }
    else if (op == "!=")
    {
        return str != arg;
    }
    return false;
}
