#pragma once

#include <libdevcore/RLP.h>
#include <libdevcore/SHA3.h>
#include <libethcore/Common.h>

namespace dev
{
namespace eth
{

struct EVMSchedule;

/// Named-boolean type to encode whether a signature be included in the serialisation process.
enum IncludeSignature
{
	WithoutSignature = 0,	///< Do not include a signature.
	WithSignature = 1,		///< Do include a signature.
};

enum class CheckTransaction
{
	None,
	Cheap,
	Everything
};

/// Encodes a transaction, ready to be exported to or freshly imported from RLP.
class TransactionBase
{
public:
	/// Constructs a null transaction.
	TransactionBase() {}

	/// Constructs a transaction from a transaction skeleton & optional secret.
	TransactionBase(TransactionSkeleton const& _ts, Secret const& _s = Secret());

	/// Constructs a signed message-call transaction.
	TransactionBase(u256 const& _value, u256 const& _gasPrice, u256 const& _gas, Address const& _dest, bytes const& _data, u256 const& _nonce, Secret const& _secret): m_type(MessageCall), m_nonce(_nonce), m_value(_value), m_receiveAddress(_dest), m_gasPrice(_gasPrice), m_gas(_gas), m_data(_data) { sign(_secret); }

	/// Constructs a signed contract-creation transaction.
	TransactionBase(u256 const& _value, u256 const& _gasPrice, u256 const& _gas, bytes const& _data, u256 const& _nonce, Secret const& _secret): m_type(ContractCreation), m_nonce(_nonce), m_value(_value), m_gasPrice(_gasPrice), m_gas(_gas), m_data(_data) { sign(_secret); }

	/// Constructs an unsigned message-call transaction.
	TransactionBase(u256 const& _value, u256 const& _gasPrice, u256 const& _gas, Address const& _dest, bytes const& _data, u256 const& _nonce = 0): m_type(MessageCall), m_nonce(_nonce), m_value(_value), m_receiveAddress(_dest), m_gasPrice(_gasPrice), m_gas(_gas), m_data(_data) {}

	/// Constructs an unsigned contract-creation transaction.
	TransactionBase(u256 const& _value, u256 const& _gasPrice, u256 const& _gas, bytes const& _data, u256 const& _nonce = 0): m_type(ContractCreation), m_nonce(_nonce), m_value(_value), m_gasPrice(_gasPrice), m_gas(_gas), m_data(_data) {}

	/// Constructs a transaction from the given RLP.
	explicit TransactionBase(bytesConstRef _rlp, CheckTransaction _checkSig);

	/// Constructs a transaction from the given RLP.
	explicit TransactionBase(bytes const& _rlp, CheckTransaction _checkSig): TransactionBase(&_rlp, _checkSig) {}

	/// Checks equality of transactions.
	bool operator==(TransactionBase const& _c) const { return m_type == _c.m_type && (m_type == ContractCreation || m_receiveAddress == _c.m_receiveAddress) && m_value == _c.m_value && m_data == _c.m_data; }
	/// Checks inequality of transactions.
	bool operator!=(TransactionBase const& _c) const { return !operator==(_c); }

	/// @returns sender of the transaction from the signature (and hash).
	Address const& sender() const;
	/// Like sender() but will never throw. @returns a null Address if the signature is invalid.
	Address const& safeSender() const noexcept;
	/// Force the sender to a particular value. This will result in an invalid transaction RLP.
	void forceSender(Address const& _a) { m_sender = _a; }

};
}
}

