#include "ChainParams.h"
#include <json_spirit/JsonSpiritHeaders.h>
#include <libdevcore/Log.h>
#include <libdevcore/TrieDB.h>
#include <libethcore/SealEngine.h>
#include <libethcore/BlockHeader.h>
#include <libethcore/Precompiled.h>
#include "GenesisInfo.h"
#include "State.h"
#include "Account.h"
using namespace std;
using namespace dev;
using namespace eth;
namespace js = json_spirit;

ChainParams::ChainParams()
{
	for (unsigned i = 1; i <= 4; ++i)
		genesisState[Address(i)] = Account(0, 1);
	// Setup default precompiled contracts as equal to genesis of Frontier.
	precompiled.insert(make_pair(Address(1), PrecompiledContract(3000, 0, PrecompiledRegistrar::executor("ecrecover"))));
	precompiled.insert(make_pair(Address(2), PrecompiledContract(60, 12, PrecompiledRegistrar::executor("sha256"))));
	precompiled.insert(make_pair(Address(3), PrecompiledContract(600, 120, PrecompiledRegistrar::executor("ripemd160"))));
	precompiled.insert(make_pair(Address(4), PrecompiledContract(15, 3, PrecompiledRegistrar::executor("identity"))));
}

ChainParams::ChainParams(string const& _json, h256 const& _stateRoot)
{
	*this = loadConfig(_json, _stateRoot);
}

ChainParams ChainParams::loadConfig(string const& _json, h256 const& _stateRoot) const
{
	ChainParams cp(*this);
	js::mValue val;
	json_spirit::read_string(_json, val);
	js::mObject obj = val.get_obj();

	cp.sealEngineName = obj["sealEngine"].get_str();
	// params
	js::mObject params = obj["params"].get_obj();
	cp.accountStartNonce = u256(fromBigEndian<u256>(fromHex(params["accountStartNonce"].get_str())));
	cp.maximumExtraDataSize = u256(fromBigEndian<u256>(fromHex(params["maximumExtraDataSize"].get_str())));
	cp.tieBreakingGas = params.count("tieBreakingGas") ? params["tieBreakingGas"].get_bool() : true;
	cp.blockReward = u256(fromBigEndian<u256>(fromHex(params["blockReward"].get_str())));
	for (auto i: params)
		if (i.first != "accountStartNonce" && i.first != "maximumExtraDataSize" && i.first != "blockReward" && i.first != "tieBreakingGas")
			cp.otherParams[i.first] = i.second.get_str();
	// genesis
	string genesisStr = json_spirit::write_string(obj["genesis"], false);
	cp.dataDir = obj.count("datadir") ? obj["datadir"].get_str() : "/tmp/ethereum/data/";
	cp.broadcastToNormalNode = obj.count("broadcastToNormalNode") ? ( (obj["broadcastToNormalNode"].get_str() == "ON") ? true : false) : false;
	cp = cp.loadGenesis(genesisStr, _stateRoot);
	// genesis state
	string genesisStateStr = json_spirit::write_string(obj["accounts"], false);
	cp = cp.loadGenesisState(genesisStateStr, _stateRoot);
	return cp;
}
