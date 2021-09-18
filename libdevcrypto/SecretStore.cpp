#include "SecretStore.h"
#include <thread>
#include <mutex>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <libdevcore/Log.h>
#include <libdevcore/Guards.h>
#include <libdevcore/SHA3.h>
#include <libdevcore/FileSystem.h>
#include <json_spirit/JsonSpiritHeaders.h>
#include <libdevcrypto/Exceptions.h>
using namespace std;
using namespace dev;
namespace js = json_spirit;
namespace fs = boost::filesystem;

static const int c_keyFileVersion = 3;

/// Upgrade the json-format to the current version.
static js::mValue upgraded(string const& _s)
{
	js::mValue v;
	js::read_string(_s, v);
	if (v.type() != js::obj_type)
		return js::mValue();
	js::mObject ret = v.get_obj();
	unsigned version = ret.count("Version") ? stoi(ret["Version"].get_str()) : ret.count("version") ? ret["version"].get_int() : 0;
	if (version == 1)
	{
		// upgrade to version 2
		js::mObject old;
		swap(old, ret);

		ret["id"] = old["Id"];
		js::mObject c;
		c["ciphertext"] = old["Crypto"].get_obj()["CipherText"];
		c["cipher"] = "aes-128-cbc";
		{
			js::mObject cp;
			cp["iv"] = old["Crypto"].get_obj()["IV"];
			c["cipherparams"] = cp;
		}
		c["kdf"] = old["Crypto"].get_obj()["KeyHeader"].get_obj()["Kdf"];
		{
			js::mObject kp;
			kp["salt"] = old["Crypto"].get_obj()["Salt"];
			for (auto const& i: old["Crypto"].get_obj()["KeyHeader"].get_obj()["KdfParams"].get_obj())
				if (i.first != "SaltLen")
					kp[boost::to_lower_copy(i.first)] = i.second;
			c["kdfparams"] = kp;
		}
		c["sillymac"] = old["Crypto"].get_obj()["MAC"];
		c["sillymacjson"] = _s;
		ret["crypto"] = c;
		version = 2;
	}
	if (ret.count("Crypto") && !ret.count("crypto"))
	{
		ret["crypto"] = ret["Crypto"];
		ret.erase("Crypto");
	}
	if (version == 2)
	{
		ret["crypto"].get_obj()["cipher"] = "aes-128-ctr";
		ret["crypto"].get_obj()["compat"] = "2";
		version = 3;
	}
	if (version == c_keyFileVersion)
		return ret;
	return js::mValue();
}

SecretStore::SecretStore(string const& _path): m_path(_path)
{
	load();
}

void SecretStore::setPath(string const& _path)
{
	m_path = _path;
	load();
}

bytesSec SecretStore::secret(h128 const& _uuid, function<string()> const& _pass, bool _useCache) const
{
	auto rit = m_cached.find(_uuid);
	if (_useCache && rit != m_cached.end())
		return rit->second;
	auto it = m_keys.find(_uuid);
	bytesSec key;
	if (it != m_keys.end())
	{
		key = bytesSec(decrypt(it->second.encryptedKey, _pass()));
		if (!key.empty())
			m_cached[_uuid] = key;
	}
	return key;
}

bytesSec SecretStore::secret(Address const& _address, function<string()> const& _pass) const
{
	bytesSec ret;
	if (auto k = key(_address))
		ret = bytesSec(decrypt(k->second.encryptedKey, _pass()));
	return ret;
}

bytesSec SecretStore::secret(string const& _content, string const& _pass)
{
	try
	{
		js::mValue u = upgraded(_content);
		if (u.type() != js::obj_type)
			return bytesSec();
		return decrypt(js::write_string(u.get_obj()["crypto"], false), _pass);
	}
	catch (...)
	{
		return bytesSec();
	}
}

h128 SecretStore::importSecret(bytesSec const& _s, string const& _pass)
{
	h128 r = h128::random();
	EncryptedKey key{encrypt(_s.ref(), _pass), toUUID(r), KeyPair(Secret(_s)).address()};
	m_cached[r] = _s;
	m_keys[r] = move(key);
	save();
	return r;
}

h128 SecretStore::importSecret(bytesConstRef _s, string const& _pass)
{
	h128 r = h128::random();
	EncryptedKey key{encrypt(_s, _pass), toUUID(r), KeyPair(Secret(_s)).address()};
	m_cached[r] = bytesSec(_s);
	m_keys[r] = move(key);
	save();
	return r;
}
