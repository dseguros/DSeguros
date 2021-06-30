#pragma once

#include <map>
#include <utility>
#include <vector>
#include <iterator>
#include <iostream>
#include <assert.h>

namespace dev
{

class RLPStream;

using UnsignedRange = std::pair<unsigned, unsigned>;
using UnsignedRanges = std::vector<UnsignedRange>;


/**
 * Set of elements of a certain "ground range" representable by unions of ranges inside this
 * ground range.
 * Ranges are given as pairs (begin, end), denoting the interval [begin, end), i.e. end is excluded.
 * Supports set-theoretic operators, size and iteration.
 */
template <class T>
class RangeMask
{
	template <class U> friend std::ostream& operator<<(std::ostream& _out, RangeMask<U> const& _r);

public:
	using Range = std::pair<T, T>;
	using Ranges = std::vector<Range>;

	/// Constructs an empty range mask with empty ground range.
	RangeMask(): m_all(0, 0) {}
	/// Constructs an empty range mask with ground range [_begin, _end).
	RangeMask(T _begin, T _end): m_all(_begin, _end) {}
	/// Constructs an empty range mask with ground range _c.
	RangeMask(Range const& _c): m_all(_c) {}

};
}
