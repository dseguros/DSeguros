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

};

}
}






