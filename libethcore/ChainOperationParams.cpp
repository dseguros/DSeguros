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
