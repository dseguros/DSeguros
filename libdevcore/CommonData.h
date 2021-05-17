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

/// Converts a (printable) ASCII hex string into the corresponding byte stream.
/// @example fromHex("41626261") == asBytes("Abba")
/// If _throw = ThrowType::DontThrow, it replaces bad hex characters with 0's, otherwise it will throw an exception.
bytes fromHex(std::string const& _s, WhenError _throw = WhenError::DontThrow);

/// @returns true if @a _s is a hex string.
bool isHex(std::string const& _s) noexcept;

/// @returns true if @a _hash is a hash conforming to FixedHash type @a T.
template <class T> static bool isHash(std::string const& _hash)
{
	return (_hash.size() == T::size * 2 || (_hash.size() == T::size * 2 + 2 && _hash.substr(0, 2) == "0x")) && isHex(_hash);
}

/// Converts byte array to a string containing the same (binary) data. Unless
/// the byte array happens to contain ASCII data, this won't be printable.
inline std::string asString(bytes const& _b)
{
	return std::string((char const*)_b.data(), (char const*)(_b.data() + _b.size()));
}

}



