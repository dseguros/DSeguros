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

        /// @returns the union with the range mask _m, taking also the union of the ground ranges.
	RangeMask unionedWith(RangeMask const& _m) const { return operator+(_m); }
	RangeMask operator+(RangeMask const& _m) const { return RangeMask(*this) += _m; }

	/// @returns a new range mask containing the smallest _items elements (not ranges).
	RangeMask lowest(decltype(T{} - T{}) _items) const
	{
		RangeMask ret(m_all);
		for (auto i = m_ranges.begin(); i != m_ranges.end() && _items; ++i)
			_items -= (ret.m_ranges[i->first] = std::min(i->first + _items, i->second)) - i->first;
		return ret;
	}

	/// @returns the complement of the range mask relative to the ground range.
	RangeMask operator~() const { return inverted(); }

        /// @returns a copy of this range mask representing the complement relative to the ground range.
	RangeMask inverted() const
	{
		RangeMask ret(m_all);
		T last = m_all.first;
		for (auto i: m_ranges)
		{
			if (i.first != last)
				ret.m_ranges[last] = i.first;
			last = i.second;
		}
		if (last != m_all.second)
			ret.m_ranges[last] = m_all.second;
		return ret;
	}

	/// Changes the range mask to its complement relative to the ground range and returns a
	/// reference to itself.
	RangeMask& invert() { return *this = inverted(); }

	template <class S> RangeMask operator-(S const& _m) const { auto ret = *this; return ret -= _m; }
	template <class S> RangeMask& operator-=(S const& _m) { return invert().unionWith(_m).invert(); }

	RangeMask& operator+=(RangeMask const& _m) { return unionWith(_m); }

};
}
