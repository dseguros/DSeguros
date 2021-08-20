#pragma once

#include <map>
#include <functional>
#include <libethcore/Common.h>
#include <libevm/ExtVMFace.h>
#include <libethcore/SealEngine.h>
#include "State.h"
#include "Executive.h"

namespace dev
{
namespace eth
{

class SealEngineFace;

/**
 * @brief Externality interface for the Virtual Machine providing access to world state.
 */
class ExtVM: public ExtVMFace
{
public:
	/// Full constructor.
	ExtVM(State& _s, EnvInfo const& _envInfo, SealEngineFace const& _sealEngine, Address _myAddress, Address _caller, Address _origin, u256 _value, u256 _gasPrice, bytesConstRef _data, bytesConstRef _code, h256 const& _codeHash, unsigned _depth = 0):
		ExtVMFace(_envInfo, _myAddress, _caller, _origin, _value, _gasPrice, _data, _code.toBytes(), _codeHash, _depth), m_s(_s), m_sealEngine(_sealEngine)
	{
		// Contract: processing account must exist. In case of CALL, the ExtVM
		// is created only if an account has code (so exist). In case of CREATE
		// the account must be created first.
		assert(m_s.addressInUse(_myAddress));
	}

	/// Read storage location.
	virtual u256 store(u256 _n) override final { return m_s.storage(myAddress, _n); }

	/// Write a value in storage.
	virtual void setStore(u256 _n, u256 _v) override final;

	/// Read address's code.
	virtual bytes const& codeAt(Address _a) override final { return m_s.code(_a); }

	/// @returns the size of the code in  bytes at the given address.
	virtual size_t codeSizeAt(Address _a) override final;

	/// Create a new contract.
	virtual h160 create(u256 _endowment, u256& io_gas, bytesConstRef _code, OnOpFunc const& _onOp = {}) override final;
};

}
}

