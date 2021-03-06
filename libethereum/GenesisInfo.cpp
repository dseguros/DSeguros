#include <libdevcrypto/Common.h>
#include <json_spirit/JsonSpiritHeaders.h>
#include "GenesisInfo.h"

using namespace dev;
using namespace eth;

std::string const dev::eth::c_genesisInfoTestBasicAuthority =
R"E(
{
	"sealEngine": "BasicAuthority",
	"params": {
		"accountStartNonce": "0x00",
		"maximumExtraDataSize": "0x20",
		"minGasLimit": "0x1388",
		"maxGasLimit": "0x7fffffffffffffff",
		"gasLimitBoundDivisor": "0x0400",
		"minimumDifficulty": "0x020000",
		"difficultyBoundDivisor": "0x0800",
		"durationLimit": "0x0d",
		"blockReward": "0x4563918244F40000",
		"registrar" : "0xc6d9d2cd449a754c494264e1809c50e34d64562b",
		"networkID" : "0x1"
	},
	"genesis": {
		"nonce": "0x0000000000000042",
		"difficulty": "0x400000000",
		"mixHash": "0x0000000000000000000000000000000000000000000000000000000000000000",
		"author": "0x0000000000000000000000000000000000000000",
		"timestamp": "0x00",
		"parentHash": "0x0000000000000000000000000000000000000000000000000000000000000000",
		"extraData": "0x11bbe8db4e347b4e8c937c1c8370e4b5ed33adb3db69cbdb7a38e1e50b1b82fa",
		"gasLimit": "0x2fefd8"
	},
	"accounts": {
		"0000000000000000000000000000000000000001": { "wei": "1", "precompiled": { "name": "ecrecover", "linear": { "base": 3000, "word": 0 } } },
		"0000000000000000000000000000000000000002": { "wei": "1", "precompiled": { "name": "sha256", "linear": { "base": 60, "word": 12 } } },
		"0000000000000000000000000000000000000003": { "wei": "1", "precompiled": { "name": "ripemd160", "linear": { "base": 600, "word": 120 } } },
		"0000000000000000000000000000000000000004": { "wei": "1", "precompiled": { "name": "identity", "linear": { "base": 15, "word": 3 } } },
		"00c9b024c2efc853ecabb8be2fb1d16ce8174ab1": { "wei": "1606938044258990275541962092341162602522202993782792835301376" }
	}
}
)E";


dev::Addresses dev::eth::childDaos()
{
	Addresses daos;
	json_spirit::mValue val;
	json_spirit::read_string(c_childDaos, val);
	json_spirit::mObject obj = val.get_obj();
	for(auto const& items: obj)
	{
		json_spirit::mArray array = items.second.get_array();
		for (auto account: array)
		{
			daos.push_back(Address(account.get_obj()["address"].get_str()));
			daos.push_back(Address(account.get_obj()["extraBalanceAccount"].get_str()));
		}
	}
	return daos;
}
