#include "Account.h"
#include <json_spirit/JsonSpiritHeaders.h>
#include <libethcore/ChainOperationParams.h>
#include <libethcore/Precompiled.h>

void Account::setNewCode(bytes&& _code)
{
	m_codeCache = std::move(_code);
	m_hasNewCode = true;
	m_codeHash = sha3(m_codeCache);
}

namespace js = json_spirit;

namespace
{

uint64_t toUnsigned(js::mValue const& _v)
{
	switch (_v.type())
	{
	case js::int_type: return _v.get_uint64();
	case js::str_type: return fromBigEndian<uint64_t>(fromHex(_v.get_str()));
	default: return 0;
	}
}

PrecompiledContract createPrecompiledContract(js::mObject& _precompiled)
{
	auto n = _precompiled["name"].get_str();
	try
	{
		u256 startingBlock = 0;
		if (_precompiled.count("startingBlock"))
			startingBlock = u256(_precompiled["startingBlock"].get_str());

		if (!_precompiled.count("linear"))
			return PrecompiledContract(PrecompiledRegistrar::pricer(n), PrecompiledRegistrar::executor(n), startingBlock);

		auto l = _precompiled["linear"].get_obj();
		unsigned base = toUnsigned(l["base"]);
		unsigned word = toUnsigned(l["word"]);
		return PrecompiledContract(base, word, PrecompiledRegistrar::executor(n), startingBlock);
	}
	catch (PricerNotFound const&)
	{
		cwarn << "Couldn't create a precompiled contract account. Missing a pricer called:" << n;
		throw;
	}
	catch (ExecutorNotFound const&)
	{
		// Oh dear - missing a plugin?
		cwarn << "Couldn't create a precompiled contract account. Missing an executor called:" << n;
		throw;
	}
}
}

