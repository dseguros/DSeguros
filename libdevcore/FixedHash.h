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


};

}

