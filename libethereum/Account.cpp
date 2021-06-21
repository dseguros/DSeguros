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

}

