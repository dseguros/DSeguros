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

}
}
