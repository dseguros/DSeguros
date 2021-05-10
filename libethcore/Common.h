
#pragma once

#include <string>
#include <functional>
#include <libdevcore/Common.h>
#include <libdevcore/FixedHash.h>
#include <libdevcrypto/Common.h>

namespace dev
{
namespace eth
{

/// Current protocol version.
extern const unsigned c_protocolVersion;

/// Current minor protocol version.
extern const unsigned c_minorProtocolVersion;

/// Current database version.
extern const unsigned c_databaseVersion;

/// User-friendly string representation of the amount _b in wei.
std::string formatBalance(bigint const& _b);

DEV_SIMPLE_EXCEPTION(InvalidAddress);

/// Convert the given string into an address.
Address toAddress(std::string const& _s);

/// Get information concerning the currency denominations.
std::vector<std::pair<u256, std::string>> const& units();

}
}
