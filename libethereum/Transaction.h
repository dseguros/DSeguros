#pragma once

#include <libdevcore/RLP.h>
#include <libdevcore/SHA3.h>
#include <libethcore/Common.h>
#include <libethcore/Transaction.h>
#include <libethcore/ChainOperationParams.h>

namespace dev
{
namespace eth
{

enum class TransactionException
{
	None = 0,
	Unknown,
	BadRLP,
	InvalidFormat,
	OutOfGasIntrinsic,		///< Too little gas to pay for the base transaction cost.
	InvalidSignature,
	InvalidNonce,
	NotEnoughCash,
	OutOfGasBase,			///< Too little gas to pay for the base transaction cost.
	BlockGasLimitReached,
	BadInstruction,
	BadJumpDestination,
	OutOfGas,				///< Ran out of gas executing code of the transaction.
	OutOfStack,				///< Ran out of stack executing code of the transaction.
	StackUnderflow,
	RevertInstruction,
	InvalidZeroSignatureFormat
};

enum class CodeDeposit
{
	None = 0,
	Failed,
	Success
};

struct VMException;

TransactionException toTransactionException(Exception const& _e);
std::ostream& operator<<(std::ostream& _out, TransactionException const& _er);

/// Description of the result of executing a transaction.
struct ExecutionResult
{
	u256 gasUsed = 0;
	TransactionException excepted = TransactionException::Unknown;
	Address newAddress;
	bytes output;
	CodeDeposit codeDeposit = CodeDeposit::None;					///< Failed if an attempted deposit failed due to lack of gas.
	u256 gasRefunded = 0;
	unsigned depositSize = 0; 										///< Amount of code of the creation's attempted deposit.
	u256 gasForDeposit; 											///< Amount of gas remaining for the code deposit phase.
};
}
}
