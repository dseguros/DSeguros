#include "ChainOperationParams.h"
#include <libdevcore/Log.h>
#include <libdevcore/CommonData.h>
using namespace std;
using namespace dev;
using namespace eth;

PrecompiledContract::PrecompiledContract(
	unsigned _base,
	unsigned _word,
	PrecompiledExecutor const& _exec,
	u256 const& _startingBlock
):
	PrecompiledContract([=](bytesConstRef _in) -> bigint
	{
		bigint s = _in.size();
		bigint b = _base;
		bigint w = _word;
		return b + (s + 31) / 32 * w;
	}, _exec, _startingBlock)
{}

ChainOperationParams::ChainOperationParams()
{
	otherParams = std::unordered_map<std::string, std::string>{
		{"minGasLimit", "0x1388"},
		{"maxGasLimit", "0x7fffffffffffffff"},
		{"gasLimitBoundDivisor", "0x0400"},
		{"minimumDifficulty", "0x020000"},
		{"difficultyBoundDivisor", "0x0800"},
		{"durationLimit", "0x0d"},
		{"registrar", "5e70c0bbcd5636e0f9f9316e9f8633feb64d4050"},
		{"networkID", "0x0"}
	};
}

