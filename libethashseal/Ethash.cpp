/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file Ethash.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "Ethash.h"
#include <libethash/ethash.h>
#include <libethash/internal.h>
#include <libethereum/Interface.h>
#include <libethcore/ChainOperationParams.h>
#include <libethcore/CommonJS.h>
#include "EthashCPUMiner.h"
using namespace std;
using namespace dev;
using namespace eth;

void Ethash::init()
{
	ETH_REGISTER_SEAL_ENGINE(Ethash);
}

Ethash::Ethash()
{
	map<string, GenericFarm<EthashProofOfWork>::SealerDescriptor> sealers;
	sealers["cpu"] = GenericFarm<EthashProofOfWork>::SealerDescriptor{&EthashCPUMiner::instances, [](GenericMiner<EthashProofOfWork>::ConstructionInfo ci){ return new EthashCPUMiner(ci); }};
	m_farm.setSealers(sealers);
	m_farm.onSolutionFound([=](EthashProofOfWork::Solution const& sol)
	{
//		cdebug << m_farm.work().seedHash << m_farm.work().headerHash << sol.nonce << EthashAux::eval(m_farm.work().seedHash, m_farm.work().headerHash, sol.nonce).value;
		setMixHash(m_sealing, sol.mixHash);
		setNonce(m_sealing, sol.nonce);
		if (!quickVerifySeal(m_sealing))
			return false;

		if (m_onSealGenerated)
		{
			RLPStream ret;
			m_sealing.streamRLP(ret);
			m_onSealGenerated(ret.out());
		}
		return true;
	});
}

strings Ethash::sealers() const
{
	return {"cpu"};
}

h256 Ethash::seedHash(BlockHeader const& _bi)
{
	return EthashAux::seedHash((unsigned)_bi.number());
}

StringHashMap Ethash::jsInfo(BlockHeader const& _bi) const
{
	return { { "nonce", toJS(nonce(_bi)) }, { "seedHash", toJS(seedHash(_bi)) }, { "mixHash", toJS(mixHash(_bi)) }, { "boundary", toJS(boundary(_bi)) }, { "difficulty", toJS(_bi.difficulty()) } };
}

void Ethash::verify(Strictness _s, BlockHeader const& _bi, BlockHeader const& _parent, bytesConstRef _block) const
{
	SealEngineFace::verify(_s, _bi, _parent, _block);

	if (_s != CheckNothingNew)
	{
		if (_bi.difficulty() < chainParams().u256Param("minimumDifficulty"))
			BOOST_THROW_EXCEPTION(InvalidDifficulty() << RequirementError(bigint(chainParams().u256Param("minimumDifficulty")), bigint(_bi.difficulty())) );

		if (_bi.gasLimit() < chainParams().u256Param("minGasLimit"))
			BOOST_THROW_EXCEPTION(InvalidGasLimit() << RequirementError(bigint(chainParams().u256Param("minGasLimit")), bigint(_bi.gasLimit())) );

		if (_bi.gasLimit() > chainParams().u256Param("maxGasLimit"))
			BOOST_THROW_EXCEPTION(InvalidGasLimit() << RequirementError(bigint(chainParams().u256Param("maxGasLimit")), bigint(_bi.gasLimit())) );

		if (_bi.number() && _bi.extraData().size() > chainParams().maximumExtraDataSize)
			BOOST_THROW_EXCEPTION(ExtraDataTooBig() << RequirementError(bigint(chainParams().maximumExtraDataSize), bigint(_bi.extraData().size())) << errinfo_extraData(_bi.extraData()));

		u256 daoHardfork = chainParams().u256Param("daoHardforkBlock");
		if (daoHardfork != 0 && daoHardfork + 9 >= daoHardfork && _bi.number() >= daoHardfork && _bi.number() <= daoHardfork + 9)
			if (_bi.extraData() != fromHex("0x64616f2d686172642d666f726b"))
				BOOST_THROW_EXCEPTION(ExtraDataIncorrect() << errinfo_comment("Received block from the wrong fork (invalid extradata)."));
	}

	if (_parent)
	{
		// Check difficulty is correct given the two timestamps.
		auto expected = calculateDifficulty(_bi, _parent);
		auto difficulty = _bi.difficulty();
		if (difficulty != expected)
			BOOST_THROW_EXCEPTION(InvalidDifficulty() << RequirementError((bigint)expected, (bigint)difficulty));

		auto gasLimit = _bi.gasLimit();
		auto parentGasLimit = _parent.gasLimit();
		if (
			gasLimit < chainParams().u256Param("minGasLimit") ||
			gasLimit > chainParams().u256Param("maxGasLimit") ||
			gasLimit <= parentGasLimit - parentGasLimit / chainParams().u256Param("gasLimitBoundDivisor") ||
			gasLimit >= parentGasLimit + parentGasLimit / chainParams().u256Param("gasLimitBoundDivisor"))
			BOOST_THROW_EXCEPTION(
				InvalidGasLimit()
				<< errinfo_min((bigint)((bigint)parentGasLimit - (bigint)(parentGasLimit / chainParams().u256Param("gasLimitBoundDivisor"))))
				<< errinfo_got((bigint)gasLimit)
				<< errinfo_max((bigint)((bigint)parentGasLimit + parentGasLimit / chainParams().u256Param("gasLimitBoundDivisor")))
			);
	}

	// check it hashes according to proof of work or that it's the genesis block.
	if (_s == CheckEverything && _bi.parentHash() && !verifySeal(_bi))
	{
		InvalidBlockNonce ex;
		ex << errinfo_nonce(nonce(_bi));
		ex << errinfo_mixHash(mixHash(_bi));
		ex << errinfo_seedHash(seedHash(_bi));
		EthashProofOfWork::Result er = EthashAux::eval(seedHash(_bi), _bi.hash(WithoutSeal), nonce(_bi));
		ex << errinfo_ethashResult(make_tuple(er.value, er.mixHash));
		ex << errinfo_hash256(_bi.hash(WithoutSeal));
		ex << errinfo_difficulty(_bi.difficulty());
		ex << errinfo_target(boundary(_bi));
		BOOST_THROW_EXCEPTION(ex);
	}
	else if (_s == QuickNonce && _bi.parentHash() && !quickVerifySeal(_bi))
	{
		InvalidBlockNonce ex;
		ex << errinfo_hash256(_bi.hash(WithoutSeal));
		ex << errinfo_difficulty(_bi.difficulty());
		ex << errinfo_nonce(nonce(_bi));
		BOOST_THROW_EXCEPTION(ex);
	}
}


void Ethash::verifyTransaction(ImportRequirements::value _ir, TransactionBase const& _t, EnvInfo const& _env) const
{
	SealEngineFace::verifyTransaction(_ir, _t, _env);

	if (_ir & ImportRequirements::TransactionSignatures)
	{
		if (_env.number() >= chainParams().u256Param("EIP158ForkBlock"))
		{
			int chainID(chainParams().u256Param("chainID"));
			_t.checkChainId(chainID);
		}
		else
			_t.checkChainId(-4);
	}
	if (_ir & ImportRequirements::TransactionBasic && _t.baseGasRequired(evmSchedule(_env)) > _t.gas())
		BOOST_THROW_EXCEPTION(OutOfGasIntrinsic());
}

u256 Ethash::childGasLimit(BlockHeader const& _bi, u256 const& _gasFloorTarget) const
{
	u256 gasFloorTarget = _gasFloorTarget == Invalid256 ? 3141562 : _gasFloorTarget;
	u256 gasLimit = _bi.gasLimit();
	u256 boundDivisor = chainParams().u256Param("gasLimitBoundDivisor");
	if (gasLimit < gasFloorTarget)
		return min<u256>(gasFloorTarget, gasLimit + gasLimit / boundDivisor - 1);
	else
		return max<u256>(gasFloorTarget, gasLimit - gasLimit / boundDivisor + 1 + (_bi.gasUsed() * 6 / 5) / boundDivisor);
}

void Ethash::manuallySubmitWork(const h256& _mixHash, Nonce _nonce)
{
	m_farm.submitProof(EthashProofOfWork::Solution{_nonce, _mixHash}, nullptr);
}

u256 Ethash::calculateDifficulty(BlockHeader const& _bi, BlockHeader const& _parent) const
{
	const unsigned c_expDiffPeriod = 100000;

	if (!_bi.number())
		throw GenesisBlockCannotBeCalculated();
	auto minimumDifficulty = chainParams().u256Param("minimumDifficulty");
	auto difficultyBoundDivisor = chainParams().u256Param("difficultyBoundDivisor");
	auto durationLimit = chainParams().u256Param("durationLimit");

	bigint target;	// stick to a bigint for the target. Don't want to risk going negative.
	if (_bi.number() < chainParams().u256Param("homsteadForkBlock"))
		// Frontier-era difficulty adjustment
		target = _bi.timestamp() >= _parent.timestamp() + durationLimit ? _parent.difficulty() - (_parent.difficulty() / difficultyBoundDivisor) : (_parent.difficulty() + (_parent.difficulty() / difficultyBoundDivisor));
	else
	{
		bigint const timestampDiff = bigint(_bi.timestamp()) - _parent.timestamp();
		bigint const adjFactor = _bi.number() < chainParams().u256Param("metropolisForkBlock") ?
			max<bigint>(1 - timestampDiff / 10, -99) : // Homestead-era difficulty adjustment
			max<bigint>((_parent.hasUncles() ? 2 : 1) - timestampDiff / 9, -99); // Metropolis-era difficulty adjustment

		target = _parent.difficulty() + _parent.difficulty() / 2048 * adjFactor;
	}

	bigint o = target;
	unsigned periodCount = unsigned(_parent.number() + 1) / c_expDiffPeriod;
	if (periodCount > 1)
		o += (bigint(1) << (periodCount - 2));	// latter will eventually become huge, so ensure it's a bigint.

	o = max<bigint>(minimumDifficulty, o);
	return u256(min<bigint>(o, std::numeric_limits<u256>::max()));
}
