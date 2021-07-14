#include "TrieCommon.h"

namespace dev
{


/*
 * Hex-prefix Notation. First nibble has flags: oddness = 2^0 & termination = 2^1
 * NOTE: the "termination marker" and "leaf-node" specifier are completely equivalent.
 * [0,0,1,2,3,4,5]   0x10012345
 * [0,1,2,3,4,5]     0x00012345
 * [1,2,3,4,5]       0x112345
 * [0,0,1,2,3,4]     0x00001234
 * [0,1,2,3,4]       0x101234
 * [1,2,3,4]         0x001234
 * [0,0,1,2,3,4,5,T] 0x30012345
 * [0,0,1,2,3,4,T]   0x20001234
 * [0,1,2,3,4,5,T]   0x20012345
 * [1,2,3,4,5,T]     0x312345
 * [1,2,3,4,T]       0x201234
 */

std::string hexPrefixEncode(bytes const& _hexVector, bool _leaf, int _begin, int _end)
{
	unsigned begin = _begin;
	unsigned end = _end < 0 ? _hexVector.size() + 1 + _end : _end;
	bool odd = ((end - begin) % 2) != 0;

	std::string ret(1, ((_leaf ? 2 : 0) | (odd ? 1 : 0)) * 16);
	if (odd)
	{
		ret[0] |= _hexVector[begin];
		++begin;
	}
	for (unsigned i = begin; i < end; i += 2)
		ret += _hexVector[i] * 16 + _hexVector[i + 1];
	return ret;
}

}
