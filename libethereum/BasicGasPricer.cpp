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

    // make gasPrice versus gasUsed distribution for the last 1000 blocks
	while (c < 1000 && p)
	{
		BlockHeader bi = _bc.info(p);
		if (bi.transactionsRoot() != EmptyTrie)
		{
			auto bb = _bc.block(p);
			RLP r(bb);
			BlockReceipts brs(_bc.receipts(bi.hash()));
			size_t i = 0;
			for (auto const& tr: r[1])
			{
				Transaction tx(tr.data(), CheckTransaction::None);
				u256 gu = brs.receipts[i].gasUsed();
				dist[tx.gasPrice()] += gu;
				total += gu;
				i++;
			}
		}
		p = bi.parentHash();
		++c;
	}
}
