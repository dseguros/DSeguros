
#include <libdevcore/vector_ref.h>
#include <libdevcore/Log.h>
#include <libdevcore/CommonIO.h>
#include <libdevcrypto/Common.h>
#include <libevmcore/EVMSchedule.h>
#include <libethcore/Exceptions.h>
#include "Transaction.h"

using namespace std;
using namespace dev;
using namespace dev::eth;

TransactionBase::TransactionBase(TransactionSkeleton const& _ts, Secret const& _s):
	m_type(_ts.creation ? ContractCreation : MessageCall),
	m_nonce(_ts.nonce),
	m_value(_ts.value),
	m_receiveAddress(_ts.to),
	m_gasPrice(_ts.gasPrice),
	m_gas(_ts.gas),
	m_data(_ts.data),
	m_sender(_ts.from)
{
	if (_s)
		sign(_s);
}

TransactionBase::TransactionBase(bytesConstRef _rlpData, CheckTransaction _checkSig)
{
	int field = 0;
	RLP rlp(_rlpData);
	try
	{
		if (!rlp.isList())
			BOOST_THROW_EXCEPTION(InvalidTransactionFormat() << errinfo_comment("transaction RLP must be a list"));

		m_nonce = rlp[field = 0].toInt<u256>();
		m_gasPrice = rlp[field = 1].toInt<u256>();
		m_gas = rlp[field = 2].toInt<u256>();
		m_type = rlp[field = 3].isEmpty() ? ContractCreation : MessageCall;
		m_receiveAddress = rlp[field = 3].isEmpty() ? Address() : rlp[field = 3].toHash<Address>(RLP::VeryStrict);
		m_value = rlp[field = 4].toInt<u256>();

		if (!rlp[field = 5].isData())
			BOOST_THROW_EXCEPTION(InvalidTransactionFormat() << errinfo_comment("transaction data RLP must be an array"));

		m_data = rlp[field = 5].toBytes();

		byte v = rlp[field = 6].toInt<byte>();
		h256 r = rlp[field = 7].toInt<u256>();
		h256 s = rlp[field = 8].toInt<u256>();

		m_vrs = SignatureStruct{ r, s, v };

		if (hasZeroSignature())
			m_chainId = m_vrs.v;
		else
		{
			if (v > 36)
				m_chainId = (v - 35) / 2;
			else if (v == 27 || v == 28)
				m_chainId = -4;
			else
				BOOST_THROW_EXCEPTION(InvalidSignature());
			m_vrs.v = v - (m_chainId * 2 + 35);

			if (_checkSig >= CheckTransaction::Cheap && !m_vrs.isValid())
				BOOST_THROW_EXCEPTION(InvalidSignature());
		}

		if (_checkSig == CheckTransaction::Everything)
			m_sender = sender();

		if (rlp.itemCount() > 9)
			BOOST_THROW_EXCEPTION(InvalidTransactionFormat() << errinfo_comment("to many fields in the transaction RLP"));
	}
	catch (Exception& _e)
	{
		_e << errinfo_name("invalid transaction format: " + toString(rlp) + " RLP: " + toHex(rlp.data()));
		throw;
	}
}

Address const& TransactionBase::safeSender() const noexcept
{
	try
	{
		return sender();
	}
	catch (...)
	{
		return ZeroAddress;
	}
}
