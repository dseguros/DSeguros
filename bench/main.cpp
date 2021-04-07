#include <clocale>
#include <fstream>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <json_spirit/JsonSpiritHeaders.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/RLP.h>
#include <libdevcore/SHA3.h>
#include <libdevcore/MemoryDB.h>
#include <libdevcore/TrieDB.h>
#include <libdevcrypto/Common.h>
#include <libdevcrypto/CryptoPP.h>

using namespace std;
using namespace dev;
namespace js = json_spirit;

void help()
{
	cout
		<< "Usage bench <mode> [OPTIONS]" << endl
		<< "Modes:" << endl
		<< "    trie  Trie benchmarks." << endl
		<< endl
		<< "General options:" << endl
		<< "    -h,--help  Print this help message and exit." << endl
		<< "    -V,--version  Show the version and exit." << endl
		;
	exit(0);
}
