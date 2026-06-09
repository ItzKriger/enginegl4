#include "CLuaPreProcessor.h"
#include "U_String.h"
#include "CScopeExit.h"
#include "U_Log.h"

#include <fstream>

void CLuaPreProcessor::UpdateInternalRanges()
{
    Ranges = FindProtectedRanges(CodeString);
    FindSkipRanges();
}

void CLuaPreProcessor::FindSkipRanges()
{
    if(CodeString.empty()) { return; }
    for(size_t i = 0; i < CodeString.size(); ++i)
    {
        if(IsInProtectedRange(i) || IsInProtectedRange(i + 1) || i + 1 >= CodeString.size()) { continue; }
        std::wstring sub = CodeString.substr(i, 2);

        if(sub == L"..") { SkipRanges.push_back({ i, i + 1 }); ++i; }
    }
}

std::vector<std::pair<size_t, size_t>> CLuaPreProcessor::FindProtectedRanges(const std::wstring& str)
{
    if(str.empty()) { return {}; }

    enum class State
    {
        Normal,
        InSingleQuote,
        InDoubleQuote,
        InLongString,
        InSingleLineComment,
        InMultiLineComment
    };

    std::vector<std::pair<size_t, size_t>> protectedRanges;
    State state = State::Normal;

    size_t i = 0;
    size_t start = 0;

    auto startsWith = [&](const std::wstring& prefix, size_t pos) -> bool
    {
        return str.compare(pos, prefix.length(), prefix) == 0;
    };

    while (i < str.length())
    {
        switch (state)
        {
            case State::Normal:
                if (startsWith(L"--[[", i))
                {
                    state = State::InMultiLineComment;
                    start = i;
                    i += 4;
                }
                else if (startsWith(L"--", i))
                {
                    state = State::InSingleLineComment;
                    start = i;
                    i += 2;
                }
                else if (startsWith(L"[[", i))
                {
                    state = State::InLongString;
                    start = i;
                    i += 2;
                }
                else if (str[i] == L'"')
                {
                    state = State::InDoubleQuote;
                    start = i++;
                }
                else if (str[i] == L'\'')
                {
                    state = State::InSingleQuote;
                    start = i++;
                }
                else
                {
                    ++i;
                }
                break;

            case State::InSingleLineComment:
                if (str[i] == L'\n')
                {
                    protectedRanges.emplace_back(start, i);
                    state = State::Normal;
                }
                ++i;
                break;

            case State::InMultiLineComment:
                if (startsWith(L"]]", i))
                {
                    i += 2;
                    protectedRanges.emplace_back(start, i);
                    state = State::Normal;
                }
                else
                {
                    ++i;
                }
                break;

            case State::InSingleQuote:
                if (str[i] == L'\\' && i + 1 < str.size())
                {
                    i += 2;
                }
                else if (str[i] == L'\'')
                {
                    ++i;
                    protectedRanges.emplace_back(start, i);
                    state = State::Normal;
                }
                else
                {
                    ++i;
                }
                break;

            case State::InDoubleQuote:
                if (str[i] == L'\\' && i + 1 < str.size())
                {
                    i += 2;
                }
                else if (str[i] == L'"')
                {
                    ++i;
                    protectedRanges.emplace_back(start, i);
                    state = State::Normal;
                }
                else
                {
                    ++i;
                }
                break;

            case State::InLongString:
                if (startsWith(L"]]", i))
                {
                    i += 2;
                    protectedRanges.emplace_back(start, i);
                    state = State::Normal;
                }
                else
                {
                    ++i;
                }
                break;
        }
    }

    if (state != State::Normal)
    {
        protectedRanges.emplace_back(start, str.length());
    }

    return protectedRanges;
}

bool CLuaPreProcessor::IsInProtectedRange(size_t pos)
{
    for (const auto& [start, end] : Ranges)
    {
        if (pos >= start && pos < end) { return true; }
    }
    return false;
}

bool CLuaPreProcessor::IsInProtectedRange(size_t rangeStart, size_t rangeEnd)
{
    for (const auto& [protectedStart, protectedEnd] : Ranges)
    {
        if (rangeStart < protectedEnd && rangeEnd > protectedStart)
        {
            return true;
        }
    }
    return false;
}

bool CLuaPreProcessor::IsInSkipRange(size_t pos)
{
    for (const auto& [start, end] : SkipRanges)
    {
        if (pos >= start && pos < end) { return true; }
    }
    return false;
}

bool CLuaPreProcessor::IsInSkipRange(size_t rangeStart, size_t rangeEnd)
{
    for (const auto& [protectedStart, protectedEnd] : SkipRanges)
    {
        if (rangeStart < protectedEnd && rangeEnd > protectedStart)
        {
            return true;
        }
    }
    return false;
}

void CLuaPreProcessor::UpdateInternalDirectives()
{
    Directives = FindDirectives(CodeString);
}

CLuaPreProcessor::DirectiveResult CLuaPreProcessor::FindDirectives(const std::wstring& str)
{
    DirectiveResult result;

    size_t i = 0;
    bool additiveMode = false;
    bool atLineStart = true;
    std::wstring directiveString;
    size_t directiveStart = 0;

    auto flushDirective = [&](size_t endPos)
    {
        if (!directiveString.empty())
        {
            auto splitted = StringUtils::split_str(directiveString, L' ');
            if (!splitted.empty())
            {
                std::wstring merged_arg;
                if (splitted.size() > 1)
                {
                    merged_arg = StringUtils::merge_arg(splitted, 1, splitted.size() - 1);
                }
                result.directives.insert({ splitted[0], merged_arg });
                result.directiveRanges.emplace_back(directiveStart, endPos);
            }
            directiveString.clear();
        }
        additiveMode = false;
    };

    while (i < str.length())
    {
        if (str[i] == L'\n')
        {
            flushDirective(i + 1);
            atLineStart = true;
            ++i;
            continue;
        }

        if (IsInProtectedRange(i, i + 1))
        {
            flushDirective(i);
            ++i;
            continue;
        }

        if (atLineStart)
        {
            size_t lineStart = i;
            while (i < str.length() && (str[i] == L' ' || str[i] == L'\t'))
            {
                ++i;
            }

            if (i < str.length() && str[i] == L'#')
            {
                additiveMode = true;
                directiveString.clear();
                directiveStart = lineStart;
                directiveString += str[i++];
                atLineStart = false;
                continue;
            }

            atLineStart = false;
        }

        if (additiveMode)
        {
            directiveString += str[i];
        }

        ++i;
    }

    flushDirective(i);
    return result;
}

std::wstring CLuaPreProcessor::DeleteDirectives()
{
    std::wstring result;
    size_t current = 0;
    for (const auto& [start, end] : Directives.directiveRanges)
    {
        if (current < start)
        {
            result.append(CodeString.begin() + current, CodeString.begin() + start);
        }
        current = end;
    }
    if (current < CodeString.size())
    {
        result.append(CodeString.begin() + current, CodeString.end());
    }
    return result;
}

std::wstring CLuaPreProcessor::GetIdentifierBackwards(const std::wstring& text, size_t pos)
{
    int brackets = 0;
    bool dotFound = false;

    std::wstring identifier;
    size_t i = pos;

    auto isSpace = [](wchar_t _char) -> bool { return _char == L'\t' || _char == L' ' || _char == L'\n'; };
    auto isBracket = [](wchar_t _char) -> bool
    {
        return _char == L'.' || _char == L':' || _char == L'(' ||
               _char == L')' || _char == L'[' || _char == L']';
    };

    for(;;)
    {
        if(IsInProtectedRange(i)) { break; }
        if((text.at(i) == L'\t' || text.at(i) == L' ' || text.at(i) == L'\n') && (brackets == 0 && !dotFound) && !identifier.empty() && !isBracket(identifier.front()))
        {
            bool furtherWillBeBracket = false;
            bool expectWidening = isBracket(identifier.front());

            if(!expectWidening)
            {
                break;
            }

            size_t j = i;
            for(;;)
            {
                if(text.at(j) == L'\t' || text.at(j) == L' ' || text.at(j) == L'\n') { if(j == 0) { break; } --j; continue; }

                if(text.at(j) == L'.' || text.at(j) == L':' || text.at(j) == L'(' ||
                   text.at(j) == L')' || text.at(j) == L'[' || text.at(j) == L']')
                {
                    furtherWillBeBracket = true;
                    break;
                }
                else
                {
                    break;
                }

                if(j == 0) { break; }
                --j;
            }

            if(!furtherWillBeBracket)
            {
                break;
            }
        }
        else
        {
            identifier.insert(identifier.begin(), text.at(i));
            if(IsInSkipRange(i)) { --i; continue; } 

            if(text.at(i) == L']' || text.at(i) == L')')
            {
                ++brackets;
            }
            else if(text.at(i) == L'[' || text.at(i) == L'(')
            {
                --brackets;
            }
            else if(text.at(i) == '.' || text.at(i) == L':')
            {
                dotFound = true;
            }
            else
            {
                dotFound = false;
            }
        }
        --i;
    }
    return identifier;
}

std::wstring CLuaPreProcessor::GetIdentifierForward(const std::wstring& text, size_t pos)
{
    int brackets = 0;
    bool dotFound = false;

    std::wstring identifier;
    size_t i = pos;

    auto isDot = [&text](size_t pos) -> bool { return text.at(pos) == L'.' && (pos == text.size() - 1 || text.at(pos + 1) != L'.'); };

    auto isSpace = [](wchar_t _char) -> bool { return _char == L'\t' || _char == L' ' || _char == L'\n'; };
    auto isBracket = [](wchar_t _char) -> bool
    {
        return _char == L'.' || _char == L':' || _char == L'(' ||
               _char == L')' || _char == L'[' || _char == L']';
    };

    for(;;)
    {
        if(IsInProtectedRange(i)) { break; }
        if((text.at(i) == L'\t' || text.at(i) == L' ' || text.at(i) == L'\n') && (brackets == 0 && !dotFound) && !identifier.empty() && !isBracket(identifier.front()))
        {
            bool furtherWillBeBracket = false;

            size_t j = i;
            for(;;)
            {
                if(text.at(j) == L'\t' || text.at(j) == L' ' || text.at(j) == L'\n') { if(j >= text.size()) { break; } ++j; continue; }

                if(isDot(j) || text.at(j) == L':' || text.at(j) == L'(' ||
                   text.at(j) == L')' || text.at(j) == L'[' || text.at(j) == L']')
                {
                    furtherWillBeBracket = true;
                    break;
                }
                else
                {
                    break;
                }

                if(j >= text.size()) { break; }
                ++j;
            }

            if(!furtherWillBeBracket)
            {
                break;
            }
        }
        else
        {
            identifier.push_back(text.at(i));
            if(IsInSkipRange(i)) { ++i; continue; }

            if(text.at(i) == L']' || text.at(i) == L')')
            {
                --brackets;
            }
            else if(text.at(i) == L'[' || text.at(i) == L'(')
            {
                ++brackets;
            }
            else if(isDot(i) || text.at(i) == L':')
            {
                dotFound = true;
            }
            else
            {
                dotFound = false;
            }
        }
        ++i;
    }
    return identifier;
}

std::wstring CLuaPreProcessor::ApplyCustomOperators(const std::wstring& source)
{
    if(source.empty()) { return {}; }

    std::wstring ret = source;
    std::vector<OperatorPattern> patterns =
    {
        {L"+=", L"+"},
        {L"-=", L"-"},
        {L"*=", L"*"},
        {L"/=", L"/"},
        {L"%=", L"%"}
    };

    size_t i = 0;
    while (i < ret.length() - 1)
    {
        if (!IsInProtectedRange(i, i + 2))
        {
            std::wstring oper;
            if(i < ret.size() - 2)
            {
                oper = ret.substr(i, 2);
            }

            auto it = std::find_if(patterns.begin(), patterns.end(), [&oper](const OperatorPattern& p) { return p.symbol == oper; });

            if (it != patterns.end())
            {
                std::wstring backwards = GetIdentifierBackwards(source, i - 1);
                std::wstring forward = GetIdentifierForward(source, i + 2);

                std::wstring finalString = backwards + L" = " + backwards + L" " + it->replacementOp + L" " + forward;

                auto isSpace = [](wchar_t _char) -> bool { return (_char == L' ' || _char == L'\t'); };
                auto newEnd = std::unique(finalString.begin(), finalString.end(), [&isSpace](wchar_t a, wchar_t b) { return isSpace(a) && isSpace(b); });
                finalString.erase(newEnd, finalString.end());

                Log::Instance() << "Found identifiers:\nBackward: \"" << backwards << "\"\nForward: \"" << forward <<
                    "\"\nFinal string: \"" << finalString << "\"\n\n";
                ++i;
                continue;

                size_t lhs_end = i;
                size_t lhs_start = lhs_end;

                while (lhs_start > 0 && iswspace(ret[lhs_start - 1]))
                {
                    --lhs_start;
                }

                while (lhs_start > 0 && (iswalnum(ret[lhs_start - 1]) || ret[lhs_start - 1] == L'_'))
                {
                    --lhs_start;
                }

                //auto range = FindLeftExpression(ret, i);
                //std::wstring lhs = ret.substr(range.first, range.second - range.first);
                std::wstring lhs = ret.substr(lhs_start, lhs_end - lhs_start);

                size_t rhs_start = i + 2;
                while (rhs_start < ret.length() && iswspace(ret[rhs_start]))
                {
                    ++rhs_start;
                }

                size_t rhs_end = rhs_start;
                while (rhs_end < ret.length() && (iswalnum(ret[rhs_end]) || ret[rhs_end] == L'_'))
                {
                    ++rhs_end;
                }

                std::wstring rhs = ret.substr(rhs_start, rhs_end - rhs_start);
                std::wstring replacement = lhs + L" = " + lhs + L" " + it->replacementOp + L" " + rhs;

                //auto isSpace = [](wchar_t _char) -> bool { return (_char == L' ' || _char == L'\t'); };
                //auto newEnd = std::unique(replacement.begin(), replacement.end(), [&isSpace](wchar_t a, wchar_t b) { return isSpace(a) && isSpace(b); });
                //replacement.erase(newEnd, replacement.end());

                ret.replace(lhs_start, rhs_end - lhs_start, replacement);
                i = lhs_start + replacement.length();
                continue;
            }
        }
        ++i;
    }
    return ret;
}

bool CLuaPreProcessor::IsDirectivePresent(const std::wstring& name)
{
    return Directives.directives.find(name) != Directives.directives.end();
}

std::wstring CLuaPreProcessor::GetDirectiveValue(const std::wstring& name)
{
    auto it = Directives.directives.find(name);
    return it != Directives.directives.end() ? it->second : std::wstring();
}

bool CLuaPreProcessor::IsDirectivePresent(const std::string& name) { return IsDirectivePresent(StringUtils::StrToWstr(name)); }
std::wstring CLuaPreProcessor::GetDirectiveValue(const std::string& name) { return GetDirectiveValue(StringUtils::StrToWstr(name)); }

void CLuaPreProcessor::ProcessText(const std::wstring& str)
{
    size_t oldLines = 0;

    CScopeExit _scope([&oldLines, this]()
    {
        size_t newLines = 0;
        for(const wchar_t& _char : CodeString) { if(_char == L'\n') { newLines++; } }

        Log::Instance() << "CLuaPreProcessor::oldLines: " << oldLines << "\nCLuaPreProcessor::newLines: " << newLines << Log::Endl;
    });

    CodeString = str;
    StringUtils::replace<std::wstring>(CodeString, L"\r\n", L"\n");

    for(const wchar_t& _char : CodeString) { if(_char == L'\n') { oldLines++; } }
 
    UpdateInternalRanges();
    UpdateInternalDirectives();

    if(!Directives.directives.empty())
    {
        CodeString = DeleteDirectives();
        UpdateInternalRanges();
    }
    if(IsDirectivePresent(L"#nopreprocess")) { return; }

    if(IsDirectivePresent(L"#disable"))
    {
        CodeString.clear();
        UpdateInternalRanges();
        return;
    }

    CodeString = ApplyCustomOperators(CodeString);
    UpdateInternalRanges();
}

void CLuaPreProcessor::Dump(const std::wstring& outName)
{
    std::wostream* output;
    std::wofstream file;

    CScopeExit guard([&file]() { if(file.is_open()) { file.close(); } });

    if(outName == L"$cerr")
    {
        output = &std::wcerr;
    }
    else if(outName == L"$cout")
    {
        output = &std::wcout;
    }
    else
    {
        file.open(outName.c_str());
        if(!file.is_open()) { return; }
    }

    (*output) << L"-- Processed code: --\n\n" << CodeString << L"\n\n---------------------\n\n-- Protected ranges: --\n\n";
    for(auto& pair : Ranges)
    {
        (*output) << pair.first << L" : " << pair.second << L'\n';
    }
    
    (*output) << L"\n-----------------------\n\n-- Directives list: --\n\n";

    size_t index = 0;
    for(auto& dirct : Directives.directives)
    {
        (*output) << L'[' << index << L"] \"" << dirct.first << L"\": \"" << dirct.second << L"\"\n";
        index++;
    }

    (*output) << L"\n-----------------------\n\n-- Directives ranges: --\n\n";

    index = 0;
    for(auto& dirct : Directives.directiveRanges)
    {
        (*output) << L'[' << index << L"] " << dirct.first << L" : " << dirct.second << L'\n';
        index++;
    }
    (*output) << L"\n-----------------------\n";
}
