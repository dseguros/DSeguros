
#include "Interface.h"
using namespace std;
using namespace dev;
using namespace eth;

void Interface::submitTransaction(Secret const& _secret, u256 const& _value, Address const& _dest, bytes const& _data, u256 const& _gas, u256 const& _gasPrice, u256 const& _nonce)
{
	TransactionSkeleton ts;
	ts.creation = false;
	ts.value = _value;
	ts.to = _dest;
	ts.data = _data;
	ts.gas = _gas;
	ts.gasPrice = _gasPrice;
	ts.nonce = _nonce;
	submitTransaction(ts, _secret);
}

