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

h128 KeyManager::uuid(Address const& _a) const
{
	auto it = m_addrLookup.find(_a);
	if (it == m_addrLookup.end())
		return h128();
	return it->second;
}

Address KeyManager::address(h128 const& _uuid) const
{
	auto it = m_uuidLookup.find(_uuid);
	if (it == m_uuidLookup.end())
		return Address();
	return it->second;
}

h128 KeyManager::import(Secret const& _s, string const& _accountName, string const& _pass, string const& _passwordHint)
{
	Address addr = KeyPair(_s).address();
	auto passHash = hashPassword(_pass);
	cachePassword(_pass);
	m_passwordHint[passHash] = _passwordHint;
	auto uuid = m_store.importSecret(_s.asBytesSec(), _pass);
	m_keyInfo[addr] = KeyInfo{passHash, _accountName, ""};
	m_addrLookup[addr] = uuid;
	m_uuidLookup[uuid] = addr;
	write(m_keysFile);
	return uuid;
}

Secret KeyManager::brain(string const& _seed)
{
	h256 r = sha3(_seed);
	for (auto i = 0; i < 16384; ++i)
		r = sha3(r);
	Secret ret(r);
	r.ref().cleanse();
	while (toAddress(ret)[0])
		ret = sha3(ret);
	return ret;
}

Secret KeyManager::subkey(Secret const& _s, unsigned _index)
{
	RLPStream out(2);
	out << _s.ref();
	out << _index;
	bytesSec r;
	out.swapOut(r.writable());
	return sha3(r);
}

Address KeyManager::importBrain(string const& _seed, string const& _accountName, string const& _passwordHint)
{
	Address addr = toAddress(brain(_seed));
	m_keyInfo[addr].accountName = _accountName;
	m_keyInfo[addr].passwordHint = _passwordHint;
	write();
	return addr;
}

void KeyManager::importExistingBrain(Address const& _a, string const& _accountName, string const& _passwordHint)
{
	m_keyInfo[_a].accountName = _accountName;
	m_keyInfo[_a].passwordHint = _passwordHint;
	write();
}

void KeyManager::importExisting(h128 const& _uuid, string const& _info, string const& _pass, string const& _passwordHint)
{
	bytesSec key = m_store.secret(_uuid, [&](){ return _pass; });
	if (key.empty())
		return;
	Address a = KeyPair(Secret(key)).address();
	auto passHash = hashPassword(_pass);
	if (!m_cachedPasswords.count(passHash))
		cachePassword(_pass);
	importExisting(_uuid, _info, a, passHash, _passwordHint);
}

void KeyManager::importExisting(h128 const& _uuid, string const& _accountName, Address const& _address, h256 const& _passHash, string const& _passwordHint)
{
	if (!m_passwordHint.count(_passHash))
		m_passwordHint[_passHash] = _passwordHint;
	m_uuidLookup[_uuid] = _address;
	m_addrLookup[_address] = _uuid;
	m_keyInfo[_address].passHash = _passHash;
	m_keyInfo[_address].accountName = _accountName;
	write(m_keysFile);
}

void KeyManager::kill(Address const& _a)
{
	auto id = m_addrLookup[_a];
	m_uuidLookup.erase(id);
	m_addrLookup.erase(_a);
	m_keyInfo.erase(_a);
	m_store.kill(id);
	write(m_keysFile);
}

KeyPair KeyManager::presaleSecret(std::string const& _json, function<string(bool)> const& _password)
{
	js::mValue val;
	json_spirit::read_string(_json, val);
	auto obj = val.get_obj();
	string p = _password(true);
	if (obj["encseed"].type() == js::str_type)
	{
		auto encseed = fromHex(obj["encseed"].get_str());
		while (true)
		{
			KeyPair k = KeyPair::fromEncryptedSeed(&encseed, p);
			if (obj["ethaddr"].type() == js::str_type)
			{
				Address a(obj["ethaddr"].get_str());
				Address b = k.address();
				if (a != b)
				{
					if ((p = _password(false)).empty())
						BOOST_THROW_EXCEPTION(PasswordUnknown());
					continue;
				}
			}
			return k;
		}
	}
	else
		BOOST_THROW_EXCEPTION(Exception() << errinfo_comment("encseed type is not js::str_type"));
}
