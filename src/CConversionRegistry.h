#pragma once
#include <unordered_map>
#include <functional>
#include <typeindex>
#include <stdexcept>
#include <tuple>
#include <string>
#include <iostream>

#include "CSingleton.h"

class CConversionRegistry : public CSingleton<CConversionRegistry>
{
public:
    CConversionRegistry()
    {
        InitConversions(*this);
    }

    void SetTestCommand(const std::string& cmdname) const;

    template <typename From, typename To>
    void registerConverter(std::function<To(const From&)> func)
    {
        auto key = std::make_tuple(std::type_index(typeid(From)), std::type_index(typeid(To)));
        m_converters[key] = [func](const void* from) -> void*
        {
            const From* fromPtr = static_cast<const From*>(from);
            return new To(func(*fromPtr));
        };
    }

    template <typename From, typename To>
    To convert(const From& from) const
    {
        auto key = std::make_tuple(std::type_index(typeid(From)), std::type_index(typeid(To)));
        auto it = m_converters.find(key);
        if (it != m_converters.end())
        {
            void* result = it->second(static_cast<const void*>(&from));
            To converted = *static_cast<To*>(result);
            delete static_cast<To*>(result);
            return converted;
        }
        throw std::runtime_error("Conversion not registered");
    }

    void* convert(const void* from, const std::type_info& fromType, const std::type_info& toType) const
    {
        auto key = std::make_tuple(std::type_index(fromType), std::type_index(toType));
        auto it = m_converters.find(key);
        if (it != m_converters.end())
        {
            return it->second(from);
        }
        throw std::runtime_error("Conversion not registered");
    }

    static void InitConversions(CConversionRegistry& registry);
private:
    using ConverterKey = std::tuple<std::type_index, std::type_index>;

    struct ConverterKeyHash
    {
        std::size_t operator()(const ConverterKey& key) const
        {
            auto hash1 = std::hash<std::type_index>()(std::get<0>(key));
            auto hash2 = std::hash<std::type_index>()(std::get<1>(key));
            return hash1 ^ (hash2 << 1);
        }
    };

    struct ConverterKeyEqual
    {
        bool operator()(const ConverterKey& lhs, const ConverterKey& rhs) const
        {
            return std::get<0>(lhs) == std::get<0>(rhs) && std::get<1>(lhs) == std::get<1>(rhs);
        }
    };

    std::unordered_map<ConverterKey, std::function<void* (const void*)>, ConverterKeyHash, ConverterKeyEqual> m_converters;
};
