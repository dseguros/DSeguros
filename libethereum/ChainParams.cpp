#include "ChainParams.h"
#include <json_spirit/JsonSpiritHeaders.h>
#include <libdevcore/Log.h>
#include <libdevcore/TrieDB.h>
#include <libethcore/SealEngine.h>
#include <libethcore/BlockHeader.h>
#include <libethcore/Precompiled.h>
#include "GenesisInfo.h"
#include "State.h"
#include "Account.h"
using namespace std;
using namespace dev;
using namespace eth;
namespace js = json_spirit;

