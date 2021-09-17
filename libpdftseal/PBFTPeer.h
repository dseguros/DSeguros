#pragma once

#include <libdevcore/RLP.h>
#include <libdevcore/Guards.h>
#include <libethcore/Common.h>
#include <libp2p/Capability.h>
#include <libp2p/HostCapability.h>
#include "Common.h"

namespace dev
{

namespace eth
{

class PBFTPeer : public p2p::Capability
{
	friend class PBFT;
public:
	PBFTPeer(std::shared_ptr<p2p::SessionFace> _s, p2p::HostCapabilityFace* _h, unsigned _i, p2p::CapDesc const& _cap, uint16_t _capID);

};

}
}
