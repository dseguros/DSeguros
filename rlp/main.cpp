/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file main.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 * RLP tool.
 */
#include <clocale>
#include <fstream>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <json_spirit/JsonSpiritHeaders.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/RLP.h>
#include <libdevcore/SHA3.h>
#include <libdevcrypto/Common.h>
#include <libdevcrypto/CryptoPP.h>
using namespace std;
using namespace dev;
namespace js = json_spirit;

void help()
{
	cout
		<< "Usage rlp <mode> [OPTIONS]" << endl
		<< "Modes:" << endl
		<< "    create <json>  Given a simplified JSON string, output the RLP." << endl
		<< "    render [ <file> | -- ]  Render the given RLP. Options:" << endl
		<< "      --indent <string>  Use string as the level indentation (default '  ')." << endl
		<< "      --hex-ints  Render integers in hex." << endl
		<< "      --string-ints  Render integers in the same way as strings." << endl
		<< "      --ascii-strings  Render data as C-style strings or hex depending on content being ASCII." << endl
		<< "      --force-string  Force all data to be rendered as C-style strings." << endl
		<< "      --force-escape  When rendering as C-style strings, force all characters to be escaped." << endl
		<< "      --force-hex  Force all data to be rendered as raw hex." << endl
		<< "    list [ <file> | -- ]  List the items in the RLP list by hash and size." << endl
		<< "    extract [ <file> | -- ]  Extract all items in the RLP list, named by hash." << endl
		<< "    assemble [ <manifest> | <base path> ] <file> ...  Given a manifest & files, output the RLP." << endl
		<< "      -D,--dapp  Dapp-building mode; equivalent to --encrypt --64." << endl
		<< endl
		<< "General options:" << endl
		<< "    -e,--encrypt  Encrypt the RLP data prior to output." << endl
		<< "    -L,--lenience  Try not to bomb out early if possible." << endl
		<< "    -x,--hex,--base-16  Treat input RLP as hex encoded data." << endl
		<< "    -k,--keccak  Output Keccak-256 hash only." << endl
		<< "    --64,--base-64  Treat input RLP as base-64 encoded data." << endl
		<< "    -b,--bin,--base-256  Treat input RLP as raw binary data." << endl
		<< "    -q,--quiet  Don't place additional information on stderr." << endl
		<< "    -h,--help  Print this help message and exit." << endl
		<< "    -V,--version  Show the version and exit." << endl
		;
	exit(0);
}

