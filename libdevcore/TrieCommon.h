#pragma once

#include "Common.h"
#include "RLP.h"

namespace dev
{

inline byte nibble(bytesConstRef _data, unsigned _i)
{
	return (_i & 1) ? (_data[_i / 2] & 15) : (_data[_i / 2] >> 4);
}

}
