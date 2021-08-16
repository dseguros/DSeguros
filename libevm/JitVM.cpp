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

}

}
}
