#pragma warning(push)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <boost/math/distributions/normal.hpp>
#pragma warning(pop)
#pragma GCC diagnostic pop
#include "BasicGasPricer.h"
#include "BlockChain.h"

void BasicGasPricer::update(BlockChain const& _bc)
{
	unsigned c = 0;
	h256 p = _bc.currentHash();
	m_gasPerBlock = _bc.info(p).gasLimit();

	map<u256, u256> dist;
	u256 total = 0;

}
