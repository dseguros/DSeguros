#pragma once

#include <vector>
#include <algorithm>
#include <unordered_set>
#include <type_traits>
#include <cstring>
#include <string>
#include "Common.h"

namespace dev
{

// String conversion functions, mainly to/from hex/nibble/byte representations.

enum class WhenError
{
	DontThrow = 0,
	Throw = 1,
};

enum class HexPrefix
{
	DontAdd = 0,
	Add = 1,
};

/// Convert a series of bytes to the corresponding string of hex duplets.
/// @param _w specifies the width of the first of the elements. Defaults to two - enough to represent a byte.
/// @example toHex("A\x69") == "4169"
template <class T>
std::string toHex(T const& _data, int _w = 2, HexPrefix _prefix = HexPrefix::DontAdd)
{
	std::ostringstream ret;
	unsigned ii = 0;
	for (auto i: _data)
		ret << std::hex << std::setfill('0') << std::setw(ii++ ? 2 : _w) << (int)(typename std::make_unsigned<decltype(i)>::type)i;
	return (_prefix == HexPrefix::Add) ? "0x" + ret.str() : ret.str();
}

}


