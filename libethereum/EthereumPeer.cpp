#include "EthereumPeer.h"

#include <chrono>
#include <libdevcore/Common.h>
#include <libethcore/Exceptions.h>
#include <libp2p/Session.h>
#include <libp2p/Host.h>
#include "EthereumHost.h"

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace p2p;

static const unsigned c_maxIncomingNewHashes = 1024;
static const unsigned c_maxHeadersToSend = 1024;


static string toString(Asking _a)
{
	switch (_a)
	{
	case Asking::BlockHeaders: return "BlockHeaders";
	case Asking::BlockBodies: return "BlockBodies";
	case Asking::NodeData: return "NodeData";
	case Asking::Receipts: return "Receipts";
	case Asking::Nothing: return "Nothing";
	case Asking::State: return "State";
	}
	return "?";
}
