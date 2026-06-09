#include "CBuildInfo.h"

std::string CBuildInfo::GetBuildDate() { return __DATE__; }
std::string CBuildInfo::GetBuildTime() { return __TIME__; }

std::string CBuildInfo::GetCompiler()
{
#if defined(__clang__)
	return "Clang/LLVM";
#elif defined(__ICC) || defined(__INTEL_COMPILER)
	return "Intel ICC/ICPC";
#elif defined(__GNUC__) || defined(__GNUG__)
	return "GNU GCC/G++";
#elif defined(__HP_cc) || defined(__HP_aCC)
	return "Hewlett-Packard C/aC++";
#elif defined(__IBMC__) || defined(__IBMCPP__)
	return "IBM XL C/C++";
#elif defined(_MSC_VER)
	return "Microsoft Visual Studio";
#elif defined(__PGI)
	return "Portland Group PGCC/PGCPP";
#elif defined(__SUNPRO_C) || defined(__SUNPRO_CC)
	return "Oracle Solaris Studio";
#else
	return "unknown compiler";
#endif
}

std::string CBuildInfo::GetOperatingSystemName()
{
#ifdef _WIN64
	return "Windows 64-bit";
#elif defined _WIN32
	return "Windows 32-bit";
#elif defined __APPLE__ || __MACH__
	return "Mac OSX";
#elif defined __linux__
	return "Linux";
#elif defined __FreeBSD__
	return "FreeBSD";
#elif defined __unix || __unix__
	return "Unix";
#else
	return "unknown operating system";
#endif
}
