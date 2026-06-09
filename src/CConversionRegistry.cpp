#include "CConversionRegistry.h"
#include "CAngle.h"
#include "U_String.h"
#include "U_General.h"
#include "U_Angles.h"
#include "U_Types.h"
#include "sol/sol.hpp"

template<typename From, typename To>
void ConvertFunc(CConversionRegistry& registry) //TODO unified conversion for vec, quat, mat, angles, color, colorint
{
    if constexpr (std::is_same_v<To, From>)
    {
        registry.registerConverter<From, To>([](const From& value) -> To
        {
            return value;
        });
    }
    else if constexpr (std::is_same_v<To, std::string> && std::is_same_v<From, std::wstring>)
    {
        registry.registerConverter<From, To>([](const From& value)
        {
            return StringUtils::WstrToStr(value);
        });
    }
    else if constexpr (std::is_same_v<To, std::wstring> && std::is_same_v<From, std::string>)
    {
        registry.registerConverter<From, To>([](const From& value)
        {
            return StringUtils::StrToWstr(value);
        });
    }
    else if constexpr (std::is_same_v<To, std::string> || std::is_same_v<To, std::wstring>)
    {
        registry.registerConverter<From, To>([](const From& value)
        {
            //return StringUtils::StrToWstr(value);
            return StringUtils::ToStr<To, From>(value);
        });
    }
    else  if constexpr (std::is_same_v<From, std::string> || std::is_same_v<From, std::wstring>)
    {
        registry.registerConverter<From, To>([](const From& value)
        {
            return StringUtils::FromStr<To, From>(value);
        });
    }
    else if constexpr (IsSequentParseable<To>())
    {
        if constexpr (IsSequentParseable<From>())
        {
            registry.registerConverter<From, To>([](const From& value)
            {
                To ret{};
                for(size_t i = 0; i < To::length() && i < From::length(); i++)
                {
                    ret[i] = value[i];
                }
                return ret;
            });
        }
        else if constexpr (!IsSequentParseable<From>() && std::is_convertible_v<From, typename To::value_type>)
        {
            registry.registerConverter<From, To>([](const From& value) -> To
            {
                To ret{};
                for(size_t i = 0; i < To::length(); i++) { ret[i] = value; }

                return ret;
            });
        }
        else if constexpr (!IsSequentParseable<From>() && std::is_same_v<From, CAngle>)
        {
            registry.registerConverter<From, To>([](const From& value) -> To
            {
                To ret{};
                for(size_t i = 0; i < To::length(); i++) { ret[i] = value.asRadians(); }

                return ret;
            });
        }
    }
    else if constexpr (IsSequentParseable<From>())
    {
        if constexpr (IsSequentParseable<To>())
        {
            registry.registerConverter<From, To>([](const From& value)
            {
                To ret{};
                for(size_t i = 0; i < To::length() && i < From::length(); i++)
                {
                    ret[i] = value[i];
                }
                return ret;
            });
        }
        else if constexpr (!IsSequentParseable<To>() && std::is_convertible_v<typename From::value_type, To>)
        {
            registry.registerConverter<From, To>([](const From& value) -> To
            {
                return value[0];
            });
        }
        else if constexpr (!IsSequentParseable<To>() && std::is_same_v<To, CAngle>)
        {
            registry.registerConverter<From, To>([](const From& value) -> To
            {
                return CAngle::radians(value[0]);
            });
        }
    }
    else if constexpr (std::is_same_v<From, CAngle>)
    {
        registry.registerConverter<From, To>([](const From& value) -> To
        {
            return static_cast<To>(value.asRadians());
        });
    }
    else if constexpr (std::is_same_v<To, CAngle>)
    {
        registry.registerConverter<From, To>([](const From& value) -> To
        {
            return CAngle::radians(static_cast<typename To::value_type>(value));
        });
    }
    else
    {
        if constexpr (std::is_convertible_v<From, To>)
        {
            registry.registerConverter<From, To>([](const From& value) -> To
            {
                return static_cast<To>(value);
            });
        }
        else
        {
            registry.registerConverter<From, To>([](const From& value) -> To
            {
                return {};
            });
        }
    }
    //throw std::runtime_error("Unsupported conversion");
}

template<typename Type>
void RepeatedConversionInit(CConversionRegistry& registry)
{
    ConvertFunc<Type, std::int8_t>(registry);
    ConvertFunc<Type, std::uint8_t>(registry);
    ConvertFunc<Type, std::int16_t>(registry);
    ConvertFunc<Type, std::uint16_t>(registry);
    ConvertFunc<Type, std::int32_t>(registry);
    ConvertFunc<Type, std::uint32_t>(registry);
    ConvertFunc<Type, std::int64_t>(registry);
    ConvertFunc<Type, std::uint64_t>(registry);
    ConvertFunc<Type, float>(registry);
    ConvertFunc<Type, double>(registry);
    ConvertFunc<Type, CAngle>(registry);
    ConvertFunc<Type, CAngles>(registry);
    ConvertFunc<Type, glm::quat>(registry);
    ConvertFunc<Type, glm::vec1>(registry);
    ConvertFunc<Type, glm::vec2>(registry);
    ConvertFunc<Type, glm::vec3>(registry);
    ConvertFunc<Type, glm::vec4>(registry);
    ConvertFunc<Type, glm::ivec1>(registry);
    ConvertFunc<Type, glm::ivec2>(registry);
    ConvertFunc<Type, glm::ivec3>(registry);
    ConvertFunc<Type, glm::ivec4>(registry);
    ConvertFunc<Type, std::string>(registry);
    ConvertFunc<Type, std::wstring>(registry);
    ConvertFunc<Type, bool>(registry);
}

void CConversionRegistry::InitConversions(CConversionRegistry& registry)
{
    RepeatedConversionInit<std::int8_t>(registry);
    RepeatedConversionInit<std::uint8_t>(registry);
    RepeatedConversionInit<std::int16_t>(registry);
    RepeatedConversionInit<std::uint16_t>(registry);
    RepeatedConversionInit<std::int32_t>(registry);
    RepeatedConversionInit<std::uint32_t>(registry);
    RepeatedConversionInit<std::int64_t>(registry);
    RepeatedConversionInit<std::uint64_t>(registry);
    RepeatedConversionInit<float>(registry);
    RepeatedConversionInit<double>(registry);
    RepeatedConversionInit<CAngle>(registry);
    RepeatedConversionInit<CAngles>(registry);
    RepeatedConversionInit<glm::quat>(registry);
    RepeatedConversionInit<glm::vec1>(registry);
    RepeatedConversionInit<glm::vec2>(registry);
    RepeatedConversionInit<glm::vec3>(registry);
    RepeatedConversionInit<glm::vec4>(registry);
    RepeatedConversionInit<glm::ivec1>(registry);
    RepeatedConversionInit<glm::ivec2>(registry);
    RepeatedConversionInit<glm::ivec3>(registry);
    RepeatedConversionInit<glm::ivec4>(registry);
    RepeatedConversionInit<std::string>(registry);
    RepeatedConversionInit<std::wstring>(registry);
    RepeatedConversionInit<bool>(registry);
}

#include "CEngine.h"
#include "CCommandProcessor.h"
#include "CLogger.h"

template<typename T1, typename T2>
void PerformTest(const std::string& t1str)
{
    T1 t1 = StringUtils::FromStr<T1>(t1str);

    std::wstringstream ss;
    ss << L"From " << StringUtils::StrToWstr(typeid(T1).name()) << L" to " << StringUtils::StrToWstr(typeid(T2).name()) << L" (" << StringUtils::ToStr<std::wstring>(t1) << L") to (";

    T2 val = CConversionRegistry::GetInstance()->convert<T1, T2>(t1);
    ss << StringUtils::ToStr<std::wstring>(val) << L")\n";

    COMPONENT_CALL(CLogger, Out(ss.str()));
}

template<typename T>
void StartTest(const std::string& type2, const std::string& val)
{
    if(type2 == "int8") { PerformTest<T, std::int8_t>(val); }
    else if(type2 == "bool") { PerformTest<T, bool>(val); }
    else if(type2 == "uint8") { PerformTest<T, std::uint8_t>(val); }
    else if(type2 == "int16") { PerformTest<T, std::int16_t>(val); }
    else if(type2 == "uint16") { PerformTest<T, std::uint16_t>(val); }
    else if(type2 == "int32") { PerformTest<T, std::int32_t>(val); }
    else if(type2 == "uint32") { PerformTest<T, std::uint32_t>(val); }
    else if(type2 == "int64") { PerformTest<T, std::int64_t>(val); }
    else if(type2 == "uint64") { PerformTest<T, std::uint64_t>(val); }
    else if(type2 == "float") { PerformTest<T, float>(val); }
    else if(type2 == "double") { PerformTest<T, double>(val); }
    else if(type2 == "angle") { PerformTest<T, CAngle>(val); }
    else if(type2 == "angles") { PerformTest<T, CAngles>(val); }
    else if(type2 == "quat") { PerformTest<T, glm::quat>(val); }
    else if(type2 == "vec2") { PerformTest<T, glm::vec2>(val); }
    else if(type2 == "vec3") { PerformTest<T, glm::vec3>(val); }
    else if(type2 == "vec4") { PerformTest<T, glm::vec4>(val); }
    else if(type2 == "ivec2") { PerformTest<T, glm::ivec2>(val); }
    else if(type2 == "ivec3") { PerformTest<T, glm::ivec3>(val); }
    else if(type2 == "ivec4") { PerformTest<T, glm::ivec4>(val); }
    else if(type2 == "string") { PerformTest<T, std::string>(val); }
    else if(type2 == "wstring") { PerformTest<T, std::wstring>(val); }
    else { COMPONENT_CALL(CLogger, Errln(L"Invalid second type")); }
}

void CConversionRegistry::SetTestCommand(const std::string& cmdname) const
{
    auto proc = CEngine::GetInstance()->Components.GetComponentTyped<CCommandProcessor>();
    proc->Commands.CreateCommand(cmdname, CCommand::CType::Server, CCommand::CmdRight::Client, []COMMAND_LAMBDA
    {
        if(args.size() < 3) { COMPONENT_CALL(CLogger, Errln(L"Not enough arguments")); return CMD_INC; }
        
        std::string type1;
        std::string type2;
        std::string val;

        type1 = StringUtils::WstrToStr(args[0]);
        type2 = StringUtils::WstrToStr(args[1]);
        val = StringUtils::WstrToStr(args[2]);

        if(type1 == "int8") { StartTest<std::int8_t>(type2, val); }
        else if(type1 == "bool") { StartTest<bool>(type2, val); }
        else if(type1 == "uint8") { StartTest<std::uint8_t>(type2, val); }
        else if(type1 == "int16") { StartTest<std::int16_t>(type2, val); }
        else if(type1 == "uint16") { StartTest<std::uint16_t>(type2, val); }
        else if(type1 == "int32") { StartTest<std::int32_t>(type2, val); }
        else if(type1 == "uint32") { StartTest<std::uint32_t>(type2, val); }
        else if(type1 == "int64") { StartTest<std::int64_t>(type2, val); }
        else if(type1 == "uint64") { StartTest<std::uint64_t>(type2, val); }
        else if(type1 == "float") { StartTest<float>(type2, val); }
        else if(type1 == "double") { StartTest<double>(type2, val); }
        else if(type1 == "angle") { StartTest<CAngle>(type2, val); }
        else if(type1 == "angles") { StartTest<CAngles>(type2, val); }
        else if(type1 == "quat") { StartTest<glm::quat>(type2, val); }
        else if(type1 == "vec2") { StartTest<glm::vec2>(type2, val); }
        else if(type1 == "vec3") { StartTest<glm::vec3>(type2, val); }
        else if(type1 == "vec4") { StartTest<glm::vec4>(type2, val); }
        else if(type1 == "ivec2") { StartTest<glm::ivec2>(type2, val); }
        else if(type1 == "ivec3") { StartTest<glm::ivec3>(type2, val); }
        else if(type1 == "ivec4") { StartTest<glm::ivec4>(type2, val); }
        else if(type1 == "string") { StartTest<std::string>(type2, val); }
        else if(type1 == "wstring") { StartTest<std::wstring>(type2, val); }
        else { COMPONENT_CALL(CLogger, Errln(L"Invalid first type")); return CMD_INC; }

        return CMD_OK;
    });
}
