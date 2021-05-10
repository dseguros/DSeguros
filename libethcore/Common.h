
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

/// The log bloom's size (2048-bit).
using LogBloom = h2048;

/// Many log blooms.
using LogBlooms = std::vector<LogBloom>;

// The various denominations; here for ease of use where needed within code.
static const u256 ether = exp10<18>();
static const u256 finney = exp10<15>();
static const u256 szabo = exp10<12>();
static const u256 shannon = exp10<9>();
static const u256 wei = exp10<0>();

using Nonce = h64;

using BlockNumber = unsigned;

static const BlockNumber LatestBlock = (BlockNumber)-2;
static const BlockNumber PendingBlock = (BlockNumber)-1;
static const h256 LatestBlockHash = h256(2);
static const h256 EarliestBlockHash = h256(1);
static const h256 PendingBlockHash = h256(0);

static const u256 DefaultBlockGasLimit = 4712388;

enum class RelativeBlock: BlockNumber
{
	Latest = LatestBlock,
	Pending = PendingBlock
};

}


}
