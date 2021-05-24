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
