#pragma once

#include "Exceptions.h"
#include "debugbreak.h"

namespace dev
{

#if defined(_MSC_VER)
#define ETH_FUNC __FUNCSIG__
#elif defined(__GNUC__)
#define ETH_FUNC __PRETTY_FUNCTION__
#else
#define ETH_FUNC __func__
#endif

#define asserts(A) ::dev::assertAux(A, #A, __LINE__, __FILE__, ETH_FUNC)
#define assertsEqual(A, B) ::dev::assertEqualAux(A, B, #A, #B, __LINE__, __FILE__, ETH_FUNC)

inline bool assertAux(bool _a, char const* _aStr, unsigned _line, char const* _file, char const* _func)
{
	bool ret = _a;
	if (!ret)
	{
		std::cerr << "Assertion failed:" << _aStr << " [func=" << _func << ", line=" << _line << ", file=" << _file << "]" << std::endl;
#if ETH_DEBUG
		debug_break();
#endif
	}
	return !ret;
}
}
