
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

