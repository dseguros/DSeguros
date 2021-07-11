#pragma once

#include "Common.h"
#include "RLP.h"

namespace dev
{

inline byte nibble(bytesConstRef _data, unsigned _i)
{
	return (_i & 1) ? (_data[_i / 2] & 15) : (_data[_i / 2] >> 4);
}

/// Interprets @a _first and @a _second as vectors of nibbles and returns the length of the longest common
/// prefix of _first[_beginFirst..._endFirst] and _second[_beginSecond..._endSecond].
inline unsigned sharedNibbles(bytesConstRef _first, unsigned _beginFirst, unsigned _endFirst, bytesConstRef _second, unsigned _beginSecond, unsigned _endSecond)
{
	unsigned ret = 0;
	while (_beginFirst < _endFirst && _beginSecond < _endSecond && nibble(_first, _beginFirst) == nibble(_second, _beginSecond))
	{
		++_beginFirst;
		++_beginSecond;
		++ret;
	}
	return ret;
}

}
