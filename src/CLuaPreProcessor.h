#pragma once
#include <string>
#include <vector>
#include <utility>
#include <map>

class CLuaPreProcessor
{
public:
    void ProcessText(const std::wstring& str);
    void Dump(const std::wstring& outName);

    bool IsDirectivePresent(const std::wstring& name);
    std::wstring GetDirectiveValue(const std::wstring& name);

    bool IsDirectivePresent(const std::string& name);
    std::wstring GetDirectiveValue(const std::string& name);

    std::wstring CodeString;
private:
    struct DirectiveResult
    {
        std::map<std::wstring, std::wstring> directives;
        std::vector<std::pair<size_t, size_t>> directiveRanges;
    };

    struct OperatorPattern
    {
        std::wstring symbol;
        std::wstring replacementOp;
    };

    std::vector<std::pair<size_t, size_t>> FindProtectedRanges(const std::wstring& str);
    void FindSkipRanges();

    DirectiveResult FindDirectives(const std::wstring& str);
    std::wstring ApplyCustomOperators(const std::wstring& source);

    std::wstring GetIdentifierBackwards(const std::wstring& text, size_t pos);
    std::wstring GetIdentifierForward(const std::wstring& text, size_t pos);

    std::pair<size_t, size_t> FindLeftExpression(const std::wstring& text, size_t operPos);
    std::pair<size_t, size_t> FindRightExpression(const std::wstring& text, size_t startPos);

    bool IsInProtectedRange(size_t pos);
    bool IsInProtectedRange(size_t rangeStart, size_t rangeEnd);

    bool IsInSkipRange(size_t pos);
    bool IsInSkipRange(size_t rangeStart, size_t rangeEnd);

    std::wstring DeleteDirectives();

    void UpdateInternalRanges();
    void UpdateInternalDirectives();

    std::vector<std::pair<size_t, size_t>> Ranges;
    std::vector<std::pair<size_t, size_t>> SkipRanges;

    DirectiveResult Directives;
};
