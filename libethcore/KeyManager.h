#pragma once

#include <functional>
#include <mutex>
#include <libdevcore/FileSystem.h>
#include <libdevcore/CommonData.h>
#include <libdevcrypto/SecretStore.h>

namespace dev
{
namespace eth
{

class PasswordUnknown: public Exception {};

struct KeyInfo
{
	KeyInfo() = default;
	KeyInfo(h256 const& _passHash, std::string const& _accountName, std::string const& _passwordHint = std::string()): passHash(_passHash), accountName(_accountName), passwordHint(_passwordHint) {}

	/// Hash of the password or h256() / UnknownPassword if unknown.
	h256 passHash;
	/// Name of the key, or JSON key info if begins with '{'.
	std::string accountName;
	/// Hint of the password. Alternative place for storage than the hash-based lookup.
	std::string passwordHint;
};

static h256 const UnknownPassword;
/// Password query function that never returns a password.
static auto const DontKnowThrow = [](){ throw PasswordUnknown(); return std::string(); };

enum class SemanticPassword
{
	Existing,
	Master
};

// TODO: This one is specifically for Ethereum, but we can make it generic in due course.
// TODO: hidden-partition style key-store.
/**
 * @brief High-level manager of password-encrypted keys for Ethereum.
 * Usage:
 *
 * Call exists() to check whether there is already a database. If so, get the master password from
 * the user and call load() with it. If not, get a new master password from the user (get them to type
 * it twice and keep some hint around!) and call create() with it.
 *
 * Uses a "key file" (and a corresponding .salt file) that contains encrypted information about the keys and
 * a directory called "secrets path" that contains a file for each key.
 */
class KeyManager
{
public:
	enum class NewKeyType { DirectICAP = 0, NoVanity, FirstTwo, FirstTwoNextTwo, FirstThree, FirstFour };

	KeyManager(std::string const& _keysFile = defaultPath(), std::string const& _secretsPath = SecretStore::defaultPath());
	~KeyManager();

	void setSecretsPath(std::string const& _secretsPath) { m_store.setPath(_secretsPath); }
	void setKeysFile(std::string const& _keysFile) { m_keysFile = _keysFile; }
	std::string const& keysFile() const { return m_keysFile; }

	bool exists() const;
	void create(std::string const& _pass);
	bool load(std::string const& _pass);
	void save(std::string const& _pass) const { write(_pass, m_keysFile); }

}


}
}
