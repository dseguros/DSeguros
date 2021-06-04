#include "SealEngine.h"
#include "Transaction.h"
#include <libevm/ExtVMFace.h>

using namespace std;
using namespace dev;
using namespace eth;

SealEngineRegistrar* SealEngineRegistrar::s_this = nullptr;

void NoProof::init()
{
	ETH_REGISTER_SEAL_ENGINE(NoProof);
}

void SealEngineFace::verify(Strictness _s, BlockHeader const& _bi, BlockHeader const& _parent, bytesConstRef _block) const
{
	_bi.verify(_s, _parent, _block);
}

void SealEngineFace::populateFromParent(BlockHeader& _bi, BlockHeader const& _parent) const
{
	_bi.populateFromParent(_parent);
}

void SealEngineFace::verifyTransaction(ImportRequirements::value _ir, TransactionBase const& _t, EnvInfo const& _env) const
{
	if ((_ir & ImportRequirements::TransactionSignatures) && _env.number() < chainParams().u256Param("metropolisForkBlock") && _t.hasZeroSignature() && _t.isCreation())
		BOOST_THROW_EXCEPTION(InvalidSignature());

	if ((_ir & ImportRequirements::TransactionBasic) &&
		_env.number() >= chainParams().u256Param("metropolisForkBlock") &&
		_t.hasZeroSignature() &&
		(_t.value() != 0 || _t.gasPrice() != 0 || _t.nonce() != 0))
			BOOST_THROW_EXCEPTION(InvalidZeroSignatureTransaction() << errinfo_got((bigint)_t.gasPrice()) << errinfo_got((bigint)_t.value()) << errinfo_got((bigint)_t.nonce()));

	if (_env.number() >= chainParams().u256Param("homsteadForkBlock") && (_ir & ImportRequirements::TransactionSignatures))
		_t.checkLowS();
}
