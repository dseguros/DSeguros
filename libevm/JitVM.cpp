#include "JitVM.h"

#include <libdevcore/Log.h>
#include <libevm/VM.h>
#include <libevm/VMFactory.h>

namespace dev
{
namespace eth
{
namespace
{

static_assert(sizeof(Address) == sizeof(evm_uint160be),
              "Address types size mismatch");
static_assert(alignof(Address) == alignof(evm_uint160be),
              "Address types alignment mismatch");

inline evm_uint160be toEvmC(Address _addr)
{
	return *reinterpret_cast<evm_uint160be*>(&_addr);
}

inline Address fromEvmC(evm_uint160be _addr)
{
	return *reinterpret_cast<Address*>(&_addr);
}

static_assert(sizeof(h256) == sizeof(evm_uint256be), "Hash types size mismatch");
static_assert(alignof(h256) == alignof(evm_uint256be), "Hash types alignment mismatch");

inline evm_uint256be toEvmC(h256 _h)
{
	return *reinterpret_cast<evm_uint256be*>(&_h);
}

inline u256 asUint(evm_uint256be _n)
{
	return fromBigEndian<u256>(_n.bytes);
}

inline h256 asHash(evm_uint256be _n)
{
	return h256(&_n.bytes[0], h256::ConstructFromPointer);
}

void evm_query(
	evm_variant* o_result,
	evm_env* _opaqueEnv,
	evm_query_key _key,
	evm_variant const* _arg
) noexcept
{
	auto &env = *reinterpret_cast<ExtVMFace*>(_opaqueEnv);
	switch (_key)
	{
	case EVM_ADDRESS:
		o_result->address = toEvmC(env.myAddress);
		break;
	case EVM_CALLER:
		o_result->address = toEvmC(env.caller);
		break;
	case EVM_ORIGIN:
		o_result->address = toEvmC(env.origin);
		break;
	case EVM_GAS_PRICE:
		o_result->uint256be = toEvmC(env.gasPrice);
		break;
	case EVM_COINBASE:
		o_result->address = toEvmC(env.envInfo().author());
		break;
	case EVM_DIFFICULTY:
		o_result->uint256be = toEvmC(env.envInfo().difficulty());
		break;
	case EVM_GAS_LIMIT:
		o_result->int64 = env.envInfo().gasLimit();
		break;
	case EVM_NUMBER:
		// TODO: Handle overflow / exception
		o_result->int64 = static_cast<int64_t>(env.envInfo().number());
		break;
	case EVM_TIMESTAMP:
		// TODO: Handle overflow / exception
		o_result->int64 = static_cast<int64_t>(env.envInfo().timestamp());
		break;
	case EVM_CODE_BY_ADDRESS:
	{
		auto addr = fromEvmC(_arg->address);
		auto &code = env.codeAt(addr);
		o_result->data = code.data();
		o_result->data_size = code.size();
		break;
	}
	case EVM_CODE_SIZE:
	{
		auto addr = fromEvmC(_arg->address);
		o_result->int64 = env.codeSizeAt(addr);
		break;
	}
	case EVM_BALANCE:
	{
		auto addr = fromEvmC(_arg->address);
		o_result->uint256be = toEvmC(env.balance(addr));
		break;
	}
	case EVM_BLOCKHASH:
		o_result->uint256be = toEvmC(env.blockHash(_arg->int64));
		break;
	case EVM_SLOAD:
	{
		auto key = asUint(_arg->uint256be);
		o_result->uint256be = toEvmC(env.store(key));
		break;
	}
	case EVM_ACCOUNT_EXISTS:
	{
		auto addr = fromEvmC(_arg->address);
		o_result->int64 = env.exists(addr);
		break;
	}
	case EVM_CALL_DEPTH:
		o_result->int64 = env.depth;
		break;
	}
}

void evm_update(
	evm_env* _opaqueEnv,
	evm_update_key _key,
	evm_variant const* _arg1,
	evm_variant const* _arg2
) noexcept
{
	auto &env = *reinterpret_cast<ExtVMFace*>(_opaqueEnv);
	switch (_key)
	{
	case EVM_SSTORE:
	{
		auto index = asUint(_arg1->uint256be);
		auto value = asUint(_arg2->uint256be);
		if (value == 0 && env.store(index) != 0)                   // If delete
			env.sub.refunds += env.evmSchedule().sstoreRefundGas;  // Increase refund counter

		env.setStore(index, value);    // Interface uses native endianness
		break;
	}
	case EVM_LOG:
	{
		size_t numTopics = _arg2->data_size / sizeof(h256);
		h256 const* pTopics = reinterpret_cast<h256 const*>(_arg2->data);
		env.log({pTopics, pTopics + numTopics}, {_arg1->data, _arg1->data_size});
		break;
	}
	case EVM_SELFDESTRUCT:
		// Register selfdestruction beneficiary.
		env.suicide(fromEvmC(_arg1->address));
		break;
	}
}

int64_t evm_call(
	evm_env* _opaqueEnv,
	evm_call_kind _kind,
	int64_t _gas,
	evm_uint160be const* _address,
	evm_uint256be const* _value,
	uint8_t const* _inputData,
	size_t _inputSize,
	uint8_t* _outputData,
	size_t _outputSize
) noexcept
{
	assert(_gas >= 0 && "Invalid gas value");
	auto &env = *reinterpret_cast<ExtVMFace*>(_opaqueEnv);
	auto value = asUint(*_value);
	bytesConstRef input{_inputData, _inputSize};

	if (_kind == EVM_CREATE)
	{
		assert(_outputSize == 20);
		u256 gas = _gas;
		auto addr = env.create(value, gas, input, {});
		auto gasLeft = static_cast<decltype(_gas)>(gas);
		if (addr)
			std::memcpy(_outputData, addr.data(), 20);
		else
			gasLeft |= EVM_CALL_FAILURE;
		return gasLeft;
	}

	CallParameters params;
	params.gas = _gas;
	params.apparentValue = _kind == EVM_DELEGATECALL ? env.value : value;
	params.valueTransfer = _kind == EVM_DELEGATECALL ? 0 : params.apparentValue;
	params.senderAddress = _kind == EVM_DELEGATECALL ? env.caller : env.myAddress;
	params.codeAddress = fromEvmC(*_address);
	params.receiveAddress = _kind == EVM_CALL ? params.codeAddress : env.myAddress;
	params.data = input;
	params.onOp = {};

	auto output = env.call(params);
	auto gasLeft = static_cast<int64_t>(params.gas);

	if (output.second)
		output.second.copyTo({_outputData, _outputSize});
	
	if (!output.first)
		// Add failure indicator.
		gasLeft |= EVM_CALL_FAILURE;

	return gasLeft;
}

}

}
}
