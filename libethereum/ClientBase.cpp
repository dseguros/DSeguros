#include "ClientBase.h"
#include <algorithm>
#include "BlockChain.h"
#include "Executive.h"
#include "State.h"

using namespace std;
using namespace dev;
using namespace dev::eth;

const char* WatchChannel::name() { return EthBlue "ℹ" EthWhite "  "; }
const char* WorkInChannel::name() { return EthOrange "⚒" EthGreen "▬▶"; }
const char* WorkOutChannel::name() { return EthOrange "⚒" EthNavy "◀▬"; }
const char* WorkChannel::name() { return EthOrange "⚒" EthWhite "  "; }

static const int64_t c_maxGasEstimate = 50000000;

pair<h256, Address> ClientBase::submitTransaction(TransactionSkeleton const& _t, Secret const& _secret)
{
	prepareForTransaction();
	
	TransactionSkeleton ts(_t);
	ts.from = toAddress(_secret);
	if (_t.nonce == Invalid256)
		ts.nonce = max<u256>(postSeal().transactionsFrom(ts.from), m_tq.maxNonce(ts.from));
	if (ts.gasPrice == Invalid256)
		ts.gasPrice = gasBidPrice();
	if (ts.gas == Invalid256)
		ts.gas = min<u256>(gasLimitRemaining() / 5, balanceAt(ts.from) / ts.gasPrice);

	Transaction t(ts, _secret);
	m_tq.import(t.rlp());
	
	return make_pair(t.sha3(), toAddress(ts.from, ts.nonce));
}

// TODO: remove try/catch, allow exceptions
ExecutionResult ClientBase::call(Address const& _from, u256 _value, Address _dest, bytes const& _data, u256 _gas, u256 _gasPrice, BlockNumber _blockNumber, FudgeFactor _ff)
{
	ExecutionResult ret;
	try
	{
		Block temp = block(_blockNumber);
		u256 nonce = max<u256>(temp.transactionsFrom(_from), m_tq.maxNonce(_from));
		u256 gas = _gas == Invalid256 ? gasLimitRemaining() : _gas;
		u256 gasPrice = _gasPrice == Invalid256 ? gasBidPrice() : _gasPrice;
		Transaction t(_value, gasPrice, gas, _dest, _data, nonce);
		t.forceSender(_from);
		if (_ff == FudgeFactor::Lenient)
			temp.mutableState().addBalance(_from, (u256)(t.gas() * t.gasPrice() + t.value()));
		ret = temp.execute(bc().lastHashes(), t, Permanence::Reverted);
	}
	catch (...)
	{
		// TODO: Some sort of notification of failure.
	}
	return ret;
}

ExecutionResult ClientBase::create(Address const& _from, u256 _value, bytes const& _data, u256 _gas, u256 _gasPrice, BlockNumber _blockNumber, FudgeFactor _ff)
{
	ExecutionResult ret;
	try
	{
		Block temp = block(_blockNumber);
		u256 n = temp.transactionsFrom(_from);
		//	cdebug << "Nonce at " << toAddress(_secret) << " pre:" << m_preSeal.transactionsFrom(toAddress(_secret)) << " post:" << m_postSeal.transactionsFrom(toAddress(_secret));
		Transaction t(_value, _gasPrice, _gas, _data, n);
		t.forceSender(_from);
		if (_ff == FudgeFactor::Lenient)
			temp.mutableState().addBalance(_from, (u256)(t.gas() * t.gasPrice() + t.value()));
		ret = temp.execute(bc().lastHashes(), t, Permanence::Reverted);
	}
	catch (...)
	{
		// TODO: Some sort of notification of failure.
	}
	return ret;
}
