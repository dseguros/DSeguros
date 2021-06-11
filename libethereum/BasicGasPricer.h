#pragma once

#include <array>
#include "GasPricer.h"

namespace dev
{
namespace eth
{

class BasicGasPricer: public GasPricer
{
public:
	explicit BasicGasPricer(u256 _weiPerRef, u256 _refsPerBlock): m_weiPerRef(_weiPerRef), m_refsPerBlock(_refsPerBlock) {}


    void setRefPrice(u256 _weiPerRef) { if ((bigint)m_refsPerBlock * _weiPerRef > std::numeric_limits<u256>::max() ) BOOST_THROW_EXCEPTION(Overflow() << errinfo_comment("ether price * block fees is larger than 2**256-1, choose a smaller number.") ); else m_weiPerRef = _weiPerRef; }
	void setRefBlockFees(u256 _refsPerBlock) { if ((bigint)m_weiPerRef * _refsPerBlock > std::numeric_limits<u256>::max() ) BOOST_THROW_EXCEPTION(Overflow() << errinfo_comment("ether price * block fees is larger than 2**256-1, choose a smaller number.") ); else m_refsPerBlock = _refsPerBlock; }
};
}
}

