
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

Address const& TransactionBase::sender() const
{
	if (!m_sender)
	{
		if (hasZeroSignature())
			m_sender = MaxAddress;
		else
		{
			auto p = recover(m_vrs, sha3(WithoutSignature));
			if (!p)
				BOOST_THROW_EXCEPTION(InvalidSignature());
			m_sender = right160(dev::sha3(bytesConstRef(p.data(), sizeof(p))));
		}
	}
	return m_sender;
}

void TransactionBase::sign(Secret const& _priv)
{
	auto sig = dev::sign(_priv, sha3(WithoutSignature));
	SignatureStruct sigStruct = *(SignatureStruct const*)&sig;
	if (sigStruct.isValid())
		m_vrs = sigStruct;
}

void TransactionBase::streamRLP(RLPStream& _s, IncludeSignature _sig, bool _forEip155hash) const
{
	if (m_type == NullTransaction)
		return;

	_s.appendList((_sig || _forEip155hash ? 3 : 0) + 6);
	_s << m_nonce << m_gasPrice << m_gas;
	if (m_type == MessageCall)
		_s << m_receiveAddress;
	else
		_s << "";
	_s << m_value << m_data;

	if (_sig)
	{
		int vOffset = m_chainId*2 + 35;
		_s << (m_vrs.v + vOffset) << (u256)m_vrs.r << (u256)m_vrs.s;
	}
	else if (_forEip155hash)
		_s << m_chainId << 0 << 0;
}

static const u256 c_secp256k1n("115792089237316195423570985008687907852837564279074904382605163141518161494337");

void TransactionBase::checkLowS() const
{
	if (m_vrs.s > c_secp256k1n / 2)
		BOOST_THROW_EXCEPTION(InvalidSignature());
}

void TransactionBase::checkChainId(int chainId) const
{
	if (m_chainId != chainId && m_chainId != -4)
		BOOST_THROW_EXCEPTION(InvalidSignature());
}

int64_t TransactionBase::baseGasRequired(bool _contractCreation, bytesConstRef _data, EVMSchedule const& _es)
{
	int64_t g = _contractCreation ? _es.txCreateGas : _es.txGas;

	// Calculate the cost of input data.
	// No risk of overflow by using int64 until txDataNonZeroGas is quite small
	// (the value not in billions).
	for (auto i: _data)
		g += i ? _es.txDataNonZeroGas : _es.txDataZeroGas;
	return g;
}

h256 TransactionBase::sha3(IncludeSignature _sig) const
{
	if (_sig == WithSignature && m_hashWith)
		return m_hashWith;

	RLPStream s;
	streamRLP(s, _sig, m_chainId > 0 && _sig == WithoutSignature);

	auto ret = dev::sha3(s.out());
	if (_sig == WithSignature)
		m_hashWith = ret;
	return ret;
}
