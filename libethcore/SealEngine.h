#pragma once

#include <functional>
#include <unordered_map>
#include <libdevcore/Guards.h>
#include <libdevcore/RLP.h>
#include "BlockHeader.h"
#include "Common.h"

namespace dev
{
namespace eth
{

class BlockHeader;
struct ChainOperationParams;
class Interface;
class PrecompiledFace;
class TransactionBase;
class EnvInfo;

class SealEngineFace
{
public:
	virtual ~SealEngineFace() {}

	virtual std::string name() const = 0;
	virtual unsigned revision() const { return 0; }
	virtual unsigned sealFields() const { return 0; }
	virtual bytes sealRLP() const { return bytes(); }
	virtual StringHashMap jsInfo(BlockHeader const&) const { return StringHashMap(); }


       /// Don't forget to call Super::verify when subclassing & overriding.
	virtual void verify(Strictness _s, BlockHeader const& _bi, BlockHeader const& _parent = BlockHeader(), bytesConstRef _block = bytesConstRef()) const;
	/// Additional verification for transactions in blocks.
	virtual void verifyTransaction(ImportRequirements::value _ir, TransactionBase const& _t, EnvInfo const& _env) const;
	/// Don't forget to call Super::populateFromParent when subclassing & overriding.
	virtual void populateFromParent(BlockHeader& _bi, BlockHeader const& _parent) const;

	bytes option(std::string const& _name) const { Guard l(x_options); return m_options.count(_name) ? m_options.at(_name) : bytes(); }
	bool setOption(std::string const& _name, bytes const& _value) { Guard l(x_options); try { if (onOptionChanging(_name, _value)) { m_options[_name] = _value; return true; } } catch (...) {} return false; }

	virtual strings sealers() const { return { "default" }; }
	virtual std::string sealer() const { return "default"; }
	virtual void setSealer(std::string const&) {}

	virtual bool shouldSeal(Interface*) { return true; }
	//virtual void generateSeal(BlockHeader const& _bi) = 0;
	virtual void generateSeal(BlockHeader const& _bi, bytes const& _block_data = bytes()) = 0;
	virtual void onSealGenerated(std::function<void(bytes const& s)> const& _f) = 0;
	virtual void cancelGeneration() {}

	virtual void setIntervalBlockTime(u256 _time) { m_intervalBlockTime = _time;}
	virtual u256 getIntervalBlockTime() {return m_intervalBlockTime;}
	ChainOperationParams const& chainParams() const { return m_params; }
	void setChainParams(ChainOperationParams const& _params) { m_params = _params; }
	SealEngineFace* withChainParams(ChainOperationParams const& _params) { setChainParams(_params); return this; }
	//virtual EVMSchedule const& evmSchedule(EnvInfo const&) const = 0;
	virtual EVMSchedule const& evmSchedule(EnvInfo const&) const { return DefaultSchedule; }
	virtual bool isPrecompiled(Address const& _a, u256 const& _blockNumber) const
	{
		return m_params.precompiled.count(_a) != 0 && _blockNumber >= m_params.precompiled.at(_a).startingBlock();
	}
	virtual bigint costOfPrecompiled(Address const& _a, bytesConstRef _in, u256 const&) const { return m_params.precompiled.at(_a).cost(_in); }
	virtual std::pair<bool, bytes> executePrecompiled(Address const& _a, bytesConstRef _in, u256 const&) const { return m_params.precompiled.at(_a).execute(_in); }
};

}
}


