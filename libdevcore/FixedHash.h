#pragma once

#include <array>
#include <cstdint>
#include <algorithm>
#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/functional/hash.hpp>
#include "CommonData.h"

namespace dev
{

/// Compile-time calculation of Log2 of constant values.
template <unsigned N> struct StaticLog2 { enum { result = 1 + StaticLog2<N/2>::result }; };
template <> struct StaticLog2<1> { enum { result = 0 }; };

extern boost::random_device s_fixedHashEngine;

/// Fixed-size raw-byte array container type, with an API optimised for storing hashes.
/// Transparently converts to/from the corresponding arithmetic type; this will
/// assume the data contained in the hash is big-endian.
template <unsigned N>
class FixedHash
{

public:
	/// The corresponding arithmetic type.
	using Arith = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<N * 8, N * 8, boost::multiprecision::unsigned_magnitude, boost::multiprecision::unchecked, void>>;

	/// The size of the container.
	enum { size = N };

	/// A dummy flag to avoid accidental construction from pointer.
	enum ConstructFromPointerType { ConstructFromPointer };

	/// Method to convert from a string.
	enum ConstructFromStringType { FromHex, FromBinary };

	/// Method to convert from a string.
	enum ConstructFromHashType { AlignLeft, AlignRight, FailIfDifferent };

	/// Construct an empty hash.
	FixedHash() { m_data.fill(0); }

	/// Construct from another hash, filling with zeroes or cropping as necessary.
	template <unsigned M> explicit FixedHash(FixedHash<M> const& _h, ConstructFromHashType _t = AlignLeft) { m_data.fill(0); unsigned c = std::min(M, N); for (unsigned i = 0; i < c; ++i) m_data[_t == AlignRight ? N - 1 - i : i] = _h[_t == AlignRight ? M - 1 - i : i]; }

	/// Convert from the corresponding arithmetic type.
	FixedHash(Arith const& _arith) { toBigEndian(_arith, m_data); }

	/// Convert from unsigned
	explicit FixedHash(unsigned _u) { toBigEndian(_u, m_data); }


	/// Explicitly construct, copying from a byte array.
	explicit FixedHash(bytes const& _b, ConstructFromHashType _t = FailIfDifferent) { if (_b.size() == N) memcpy(m_data.data(), _b.data(), std::min<unsigned>(_b.size(), N)); else { m_data.fill(0); if (_t != FailIfDifferent) { auto c = std::min<unsigned>(_b.size(), N); for (unsigned i = 0; i < c; ++i) m_data[_t == AlignRight ? N - 1 - i : i] = _b[_t == AlignRight ? _b.size() - 1 - i : i]; } } }

	/// Explicitly construct, copying from a byte array.
	explicit FixedHash(bytesConstRef _b, ConstructFromHashType _t = FailIfDifferent) { if (_b.size() == N) memcpy(m_data.data(), _b.data(), std::min<unsigned>(_b.size(), N)); else { m_data.fill(0); if (_t != FailIfDifferent) { auto c = std::min<unsigned>(_b.size(), N); for (unsigned i = 0; i < c; ++i) m_data[_t == AlignRight ? N - 1 - i : i] = _b[_t == AlignRight ? _b.size() - 1 - i : i]; } } }

	/// Explicitly construct, copying from a bytes in memory with given pointer.
	explicit FixedHash(byte const* _bs, ConstructFromPointerType) { memcpy(m_data.data(), _bs, N); }

	/// Explicitly construct, copying from a  string.
	explicit FixedHash(std::string const& _s, ConstructFromStringType _t = FromHex, ConstructFromHashType _ht = FailIfDifferent): FixedHash(_t == FromHex ? fromHex(_s, WhenError::Throw) : dev::asBytes(_s), _ht) {}

	/// Convert to arithmetic type.
	operator Arith() const { return fromBigEndian<Arith>(m_data); }

	/// @returns true iff this is the empty hash.
	explicit operator bool() const { return std::any_of(m_data.begin(), m_data.end(), [](byte _b) { return _b != 0; }); }

	// The obvious comparison operators.
	bool operator==(FixedHash const& _c) const { return m_data == _c.m_data; }
	bool operator!=(FixedHash const& _c) const { return m_data != _c.m_data; }
	bool operator<(FixedHash const& _c) const { for (unsigned i = 0; i < N; ++i) if (m_data[i] < _c.m_data[i]) return true; else if (m_data[i] > _c.m_data[i]) return false; return false; }
	bool operator>=(FixedHash const& _c) const { return !operator<(_c); }
	bool operator<=(FixedHash const& _c) const { return operator==(_c) || operator<(_c); }
	bool operator>(FixedHash const& _c) const { return !operator<=(_c); }

	// The obvious binary operators.
	FixedHash& operator^=(FixedHash const& _c) { for (unsigned i = 0; i < N; ++i) m_data[i] ^= _c.m_data[i]; return *this; }
	FixedHash operator^(FixedHash const& _c) const { return FixedHash(*this) ^= _c; }
	FixedHash& operator|=(FixedHash const& _c) { for (unsigned i = 0; i < N; ++i) m_data[i] |= _c.m_data[i]; return *this; }
	FixedHash operator|(FixedHash const& _c) const { return FixedHash(*this) |= _c; }
	FixedHash& operator&=(FixedHash const& _c) { for (unsigned i = 0; i < N; ++i) m_data[i] &= _c.m_data[i]; return *this; }
	FixedHash operator&(FixedHash const& _c) const { return FixedHash(*this) &= _c; }
	FixedHash operator~() const { FixedHash ret; for (unsigned i = 0; i < N; ++i) ret[i] = ~m_data[i]; return ret; }
};

}

