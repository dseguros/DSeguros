#include "Common.h"
#include <boost/algorithm/string/case_conv.hpp>
#include <libdevcore/Base64.h>
#include <libdevcore/Terminal.h>
#include <libdevcore/CommonData.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/Log.h>
#include <libdevcore/SHA3.h>
#include "ICAP.h"
#include "Exceptions.h"
#include "BlockHeader.h"

using namespace std;
using namespace dev;
using namespace dev::eth;

namespace dev
{
namespace eth
{

const unsigned c_protocolVersion = 63;
#if ETH_FATDB
const unsigned c_minorProtocolVersion = 3;
const unsigned c_databaseBaseVersion = 9;
const unsigned c_databaseVersionModifier = 1;
#else
const unsigned c_minorProtocolVersion = 2;
const unsigned c_databaseBaseVersion = 9;
const unsigned c_databaseVersionModifier = 0;
#endif

const unsigned c_databaseVersion = c_databaseBaseVersion + (c_databaseVersionModifier << 8) + (23 << 9);

}
}

