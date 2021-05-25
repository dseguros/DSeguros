#pragma once

#include <string>
#include "FixedHash.h"
#include "CommonData.h"
#include "CommonIO.h"

namespace dev
{

template <unsigned S> std::string toJS(FixedHash<S> const& _h)
{
	return "0x" + toHex(_h.ref());
}

template <unsigned N> std::string toJS(boost::multiprecision::number<boost::multiprecision::cpp_int_backend<N, N, boost::multiprecision::unsigned_magnitude, boost::multiprecision::unchecked, void>> const& _n)
{
	std::string h = toHex(toCompactBigEndian(_n, 1));
	// remove first 0, if it is necessary;
	std::string res = h[0] != '0' ? h : h.substr(1);
	return "0x" + res;
}

inline std::string toJS(bytes const& _n, std::size_t _padding = 0)
{
	bytes n = _n;
	n.resize(std::max<unsigned>(n.size(), _padding));
	return "0x" + toHex(n);
}

template<unsigned T> std::string toJS(SecureFixedHash<T> const& _i)
{
	std::stringstream stream;
	stream << "0x" << _i.makeInsecure().hex();
	return stream.str();
}

}

