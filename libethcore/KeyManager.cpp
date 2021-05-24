#include "KeyManager.h"
#include <thread>
#include <mutex>
#include <boost/filesystem.hpp>
#include <json_spirit/JsonSpiritHeaders.h>
#include <libdevcore/Log.h>
#include <libdevcore/Guards.h>
#include <libdevcore/RLP.h>
#include <libdevcore/SHA3.h>

using namespace std;
using namespace dev;
using namespace eth;
namespace js = json_spirit;
namespace fs = boost::filesystem;

KeyManager::KeyManager(string const& _keysFile, string const& _secretsPath):
	m_keysFile(_keysFile), m_store(_secretsPath)
{
	for (auto const& uuid: m_store.keys())
	{
		auto addr = m_store.address(uuid);
		m_addrLookup[addr] = uuid;
		m_uuidLookup[uuid] = addr;
	}
}

KeyManager::~KeyManager()
{}

bool KeyManager::exists() const
{
	return !contents(m_keysFile + ".salt").empty() && !contents(m_keysFile).empty();
}

void KeyManager::create(string const& _pass)
{
	m_defaultPasswordDeprecated = asString(h256::random().asBytes());
	write(_pass, m_keysFile);
}

bool KeyManager::recode(Address const& _address, string const& _newPass, string const& _hint, function<string()> const& _pass, KDF _kdf)
{
	noteHint(_newPass, _hint);
	h128 u = uuid(_address);
	if (!store().recode(u, _newPass, [&](){ return getPassword(u, _pass); }, _kdf))
		return false;

	m_keyInfo[_address].passHash = hashPassword(_newPass);
	write();
	return true;
}
