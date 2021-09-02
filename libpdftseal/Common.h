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

// for pbft
struct PBFTMsgPacket {
	u256 node_idx;
	h512 node_id;
	unsigned packet_id;
	bytes data; // rlp data
	u256 timestamp;

	PBFTMsgPacket(): node_idx(h256(0)), node_id(h512(0)), packet_id(0), timestamp(utcTime()) {}
	PBFTMsgPacket(u256 _idx, h512 _id, unsigned _pid, bytesConstRef _data)
		: node_idx(_idx), node_id(_id), packet_id(_pid), data(_data.toBytes()), timestamp(utcTime()) {}
};
using PBFTMsgQueue = dev::concurrent_queue<PBFTMsgPacket>;

enum PBFTStatus
{
	PBFT_INIT = 0,
	PBFT_PREPARED,
	PBFT_COMMITTED
};

}
}
