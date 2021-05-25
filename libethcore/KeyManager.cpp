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

bool KeyManager::recode(Address const& _address, SemanticPassword _newPass, function<string()> const& _pass, KDF _kdf)
{
	h128 u = uuid(_address);
	string p;
	if (_newPass == SemanticPassword::Existing)
		p = getPassword(u, _pass);
	else if (_newPass == SemanticPassword::Master)
		p = defaultPassword();
	else
		return false;

	return recode(_address, p, string(), _pass, _kdf);
}

bool KeyManager::load(string const& _pass)
{
	try
	{
		bytes salt = contents(m_keysFile + ".salt");
		bytes encKeys = contents(m_keysFile);
		if (encKeys.empty())
			return false;
		m_keysFileKey = SecureFixedHash<16>(pbkdf2(_pass, salt, 262144, 16));
		bytesSec bs = decryptSymNoAuth(m_keysFileKey, h128(), &encKeys);
		RLP s(bs.ref());
		unsigned version = unsigned(s[0]);
		if (version == 1)
		{
			bool saveRequired = false;
			for (auto const& i: s[1])
			{
				h128 uuid(i[1]);
				Address addr(i[0]);
				if (uuid)
				{
					if (m_store.contains(uuid))
					{
						m_addrLookup[addr] = uuid;
						m_uuidLookup[uuid] = addr;
						m_keyInfo[addr] = KeyInfo(h256(i[2]), string(i[3]), i.itemCount() > 4 ? string(i[4]) : "");
						if (m_store.noteAddress(uuid, addr))
							saveRequired = true;
					}
					else
						cwarn << "Missing key:" << uuid << addr;
				}
				else
				{
					// TODO: brain wallet.
					m_keyInfo[addr] = KeyInfo(h256(i[2]), string(i[3]), i.itemCount() > 4 ? string(i[4]) : "");
				}
//				cdebug << toString(addr) << toString(uuid) << toString((h256)i[2]) << (string)i[3];
			}
			if (saveRequired)
				m_store.save();

			for (auto const& i: s[2])
				m_passwordHint[h256(i[0])] = string(i[1]);
			m_defaultPasswordDeprecated = string(s[3]);
		}
//		cdebug << hashPassword(m_password) << toHex(m_password);
		cachePassword(m_defaultPasswordDeprecated);
//		cdebug << hashPassword(asString(m_key.ref())) << m_key.hex();
		cachePassword(asString(m_keysFileKey.ref()));
//		cdebug << hashPassword(_pass) << _pass;
		m_master = hashPassword(_pass);
		cachePassword(_pass);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

Secret KeyManager::secret(Address const& _address, function<string()> const& _pass, bool _usePasswordCache) const
{
	if (m_addrLookup.count(_address))
		return secret(m_addrLookup.at(_address), _pass, _usePasswordCache);
	else
		return brain(_pass());
}

Secret KeyManager::secret(h128 const& _uuid, function<string()> const& _pass, bool _usePasswordCache) const
{
	if (_usePasswordCache)
		return Secret(m_store.secret(_uuid, [&](){ return getPassword(_uuid, _pass); }, _usePasswordCache));
	else
		return Secret(m_store.secret(_uuid, _pass, _usePasswordCache));
}

string KeyManager::getPassword(h128 const& _uuid, function<string()> const& _pass) const
{
	h256 ph;
	auto ait = m_uuidLookup.find(_uuid);
	if (ait != m_uuidLookup.end())
	{
		auto kit = m_keyInfo.find(ait->second);
		if (kit != m_keyInfo.end())
			ph = kit->second.passHash;
	}
	return getPassword(ph, _pass);
}

string KeyManager::getPassword(h256 const& _passHash, function<string()> const& _pass) const
{
	auto it = m_cachedPasswords.find(_passHash);
	if (it != m_cachedPasswords.end())
		return it->second;
	for (unsigned i = 0; i < 10; ++i)
	{
		string p = _pass();
		if (p.empty())
			break;
		if (_passHash == UnknownPassword || hashPassword(p) == _passHash)
		{
			cachePassword(p);
			return p;
		}
	}
	return string();
}
