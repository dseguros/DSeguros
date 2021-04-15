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
/** @file Common.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "Common.h"
#include <libdevcore/SHA3.h>
#include "Message.h"
#include "BloomFilter.h"

using namespace std;
using namespace dev;
using namespace dev::p2p;
using namespace dev::shh;

AbridgedTopic dev::shh::abridge(Topic const& _p)
{
	return AbridgedTopic(sha3(_p));
}

AbridgedTopics dev::shh::abridge(Topics const& _topics)
{
	AbridgedTopics ret;
	ret.reserve(_topics.size());
	for (auto const& t: _topics)
		ret.push_back(abridge(t));
	return ret;
}

AbridgedTopics BuildTopic::toAbridgedTopics() const
{
	AbridgedTopics ret;
	ret.reserve(m_parts.size());
	for (auto const& h: m_parts)
		ret.push_back(abridge(h));
	return ret;
}

/*
web3.shh.watch({}).arrived(function(m) { env.note("New message:\n"+JSON.stringify(m)); })
k = web3.shh.newIdentity()
web3.shh.post({from: k, topic: web3.fromAscii("test"), payload: web3.fromAscii("Hello world!")})
*/

