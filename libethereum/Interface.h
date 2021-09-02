#pragma once

#include <libdevcore/Common.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/Guards.h>
#include <libdevcrypto/Common.h>
#include <libethcore/SealEngine.h>
#include "GasPricer.h"
#include "LogFilter.h"
#include "Transaction.h"
#include "TransactionQueue.h"
#include "BlockDetails.h"

namespace dev
{
namespace eth
{

struct SyncStatus;

using TransactionHashes = h256s;
using UncleHashes = h256s;

enum class Reaping
{
	Automatic,
	Manual
};

enum class FudgeFactor
{
	Strict,
	Lenient
};

struct GasEstimationProgress
{
	u256 lowerBound;
	u256 upperBound;
};

using GasEstimationCallback = std::function<void(GasEstimationProgress const&)>;

class Interface
{
public:
	/// Constructor.
	Interface() {}

	/// Destructor.
	virtual ~Interface() {}

	// [TRANSACTION API]

	/// Submits a new transaction.
	/// @returns the transaction's hash.
	virtual std::pair<h256, Address> submitTransaction(TransactionSkeleton const& _t, Secret const& _secret) = 0;

	/// Submits the given message-call transaction.
	void submitTransaction(Secret const& _secret, u256 const& _value, Address const& _dest, bytes const& _data = bytes(), u256 const& _gas = 1000000, u256 const& _gasPrice = DefaultGasPrice, u256 const& _nonce = Invalid256);

	/// Submits a new contract-creation transaction.
	/// @returns the new contract's address (assuming it all goes through).
	Address submitTransaction(Secret const& _secret, u256 const& _endowment, bytes const& _init, u256 const& _gas = 1000000, u256 const& _gasPrice = DefaultGasPrice, u256 const& _nonce = Invalid256);

	/// Blocks until all pending transactions have been processed.
	virtual void flushTransactions() = 0;

	/// Makes the given call. Nothing is recorded into the state.
	virtual ExecutionResult call(Address const& _from, u256 _value, Address _dest, bytes const& _data, u256 _gas, u256 _gasPrice, BlockNumber _blockNumber, FudgeFactor _ff = FudgeFactor::Strict) = 0;
	ExecutionResult call(Address const& _from, u256 _value, Address _dest, bytes const& _data = bytes(), u256 _gas = 1000000, u256 _gasPrice = DefaultGasPrice, FudgeFactor _ff = FudgeFactor::Strict) { return call(_from, _value, _dest, _data, _gas, _gasPrice, m_default, _ff); }
	ExecutionResult call(Secret const& _secret, u256 _value, Address _dest, bytes const& _data, u256 _gas, u256 _gasPrice, BlockNumber _blockNumber, FudgeFactor _ff = FudgeFactor::Strict) { return call(toAddress(_secret), _value, _dest, _data, _gas, _gasPrice, _blockNumber, _ff); }
	ExecutionResult call(Secret const& _secret, u256 _value, Address _dest, bytes const& _data, u256 _gas, u256 _gasPrice, FudgeFactor _ff = FudgeFactor::Strict) { return call(toAddress(_secret), _value, _dest, _data, _gas, _gasPrice, _ff); }

	/// Does the given creation. Nothing is recorded into the state.
	/// @returns the pair of the Address of the created contract together with its code.
	virtual ExecutionResult create(Address const& _from, u256 _value, bytes const& _data, u256 _gas, u256 _gasPrice, BlockNumber _blockNumber, FudgeFactor _ff = FudgeFactor::Strict) = 0;
	ExecutionResult create(Address const& _from, u256 _value, bytes const& _data = bytes(), u256 _gas = 1000000, u256 _gasPrice = DefaultGasPrice, FudgeFactor _ff = FudgeFactor::Strict) { return create(_from, _value, _data, _gas, _gasPrice, m_default, _ff); }
	ExecutionResult create(Secret const& _secret, u256 _value, bytes const& _data, u256 _gas, u256 _gasPrice, BlockNumber _blockNumber, FudgeFactor _ff = FudgeFactor::Strict) { return create(toAddress(_secret), _value, _data, _gas, _gasPrice, _blockNumber, _ff); }
	ExecutionResult create(Secret const& _secret, u256 _value, bytes const& _data, u256 _gas, u256 _gasPrice, FudgeFactor _ff = FudgeFactor::Strict) { return create(toAddress(_secret), _value, _data, _gas, _gasPrice, _ff); }


};

}
}

namespace std
{

inline void swap(dev::eth::Watch& _a, dev::eth::Watch& _b)
{
	swap(_a.m_c, _b.m_c);
	swap(_a.m_id, _b.m_id);
}

}
