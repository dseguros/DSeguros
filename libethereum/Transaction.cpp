
#include <libdevcore/vector_ref.h>
#include <libdevcore/Log.h>
#include <libdevcore/CommonIO.h>
#include <libdevcrypto/Common.h>
#include <libethcore/Exceptions.h>
#include <libevm/VMFace.h>
#include "Interface.h"
#include "Transaction.h"
using namespace std;
using namespace dev;
using namespace dev::eth;

#define ETH_ADDRESS_DEBUG 0

std::ostream& dev::eth::operator<<(std::ostream& _out, ExecutionResult const& _er)
{
	_out << "{" << _er.gasUsed << ", " << _er.newAddress << ", " << toHex(_er.output) << "}";
	return _out;
}
