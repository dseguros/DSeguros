#pragma once

#include <libdevcore/concurrent_queue.h>
//#include <libdevcore/easylog.h>
#include <libdevcore/RLP.h>
#include <libdevcore/SHA3.h>
#include <libdevcrypto/Common.h>
#include <libethcore/Exceptions.h>

namespace dev
{

namespace eth
{

 const std::string backup_key_committed = "committed";

enum PBFTPacketType : byte
{
	PrepareReqPacket = 0x00,
	SignReqPacket = 0x01,
	CommitReqPacket = 0x02,
	ViewChangeReqPacket = 0x03,

	PBFTPacketCount
};
}
}
