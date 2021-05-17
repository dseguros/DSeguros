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

}


