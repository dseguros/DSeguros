#pragma once

#include "Common.h"

namespace dev
{
namespace shh
{

template <unsigned N>
class TopicBloomFilterBase: public FixedHash<N>
{
public:
	TopicBloomFilterBase() { init(); }
	TopicBloomFilterBase(FixedHash<N> const& _h): FixedHash<N>(_h) { init(); }

	void addBloom(AbridgedTopic const& _h) { addRaw(bloom(_h)); }
	void removeBloom(AbridgedTopic const& _h) { removeRaw(bloom(_h)); }
	bool containsBloom(AbridgedTopic const& _h) const { return this->contains(bloom(_h)); }


	void addRaw(FixedHash<N> const& _h);
	void removeRaw(FixedHash<N> const& _h);
	bool containsRaw(FixedHash<N> const& _h) const { return this->contains(_h); }

	static FixedHash<N> bloom(AbridgedTopic const& _h);
	static void setBit(FixedHash<N>& _h, unsigned index);
	static bool isBitSet(FixedHash<N> const& _h, unsigned _index);
private:
	void init() { for (unsigned i = 0; i < CounterSize; ++i) m_refCounter[i] = 0; }

	static const unsigned CounterSize = N * 8;
	std::array<uint16_t, CounterSize> m_refCounter;
};

}
}






