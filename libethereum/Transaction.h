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

}
}
