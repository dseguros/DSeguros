#pragma once
#include <libethcore/Common.h>
#include <libethereum/CommonNet.h>
#include <libp2p/Capability.h>
#include <libp2p/HostCapability.h>
#include "Common.h"
#include "PBFTPeer.h"

namespace dev
{
namespace eth
{

class PBFTHost: public p2p::HostCapability<PBFTPeer>
{
public:
	typedef std::function<void(unsigned, std::shared_ptr<p2p::Capability>, RLP const&)> MsgHandler;

	PBFTHost(MsgHandler h): m_msg_handler(h) {}
	virtual ~PBFTHost() {}

};

}
}
