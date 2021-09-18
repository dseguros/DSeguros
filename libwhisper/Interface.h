#pragma once

#include <mutex>
#include <array>
#include <set>
#include <memory>
#include <utility>

#include <libdevcore/RLP.h>
#include <libdevcore/Guards.h>
#include <libdevcore/SHA3.h>
#include "Common.h"
#include "Message.h"

namespace dev
{
namespace shh
{

class Watch;

struct InstalledFilter
{
	InstalledFilter(Topics const& _t): full(_t), filter(_t) {}

	Topics full;
	TopicFilter filter;
	unsigned refCount = 1;
};

struct ClientWatch
{
	ClientWatch() {}
	explicit ClientWatch(h256 _id): id(_id) {}

	h256 id;
	h256s changes;
};

class Interface
{
public:
	virtual ~Interface();

	virtual void inject(Envelope const& _m, WhisperPeer* _from = nullptr) = 0;

	virtual Topics const& fullTopics(unsigned _id) const = 0;
	virtual unsigned installWatch(Topics const& _filter) = 0;
	virtual void uninstallWatch(unsigned _watchId) = 0;
	virtual h256s peekWatch(unsigned _watchId) const = 0;
	virtual h256s checkWatch(unsigned _watchId) = 0;
	virtual h256s watchMessages(unsigned _watchId) = 0;


	virtual Envelope envelope(h256 _m) const = 0;

	void post(bytes const& _payload, Topics _topics, unsigned _ttl = 50, unsigned _workToProve = 50) { inject(Message(_payload).seal(_topics, _ttl, _workToProve)); }
	void post(Public _to, bytes const& _payload, Topics _topics, unsigned _ttl = 50, unsigned _workToProve = 50) { inject(Message(_payload).sealTo(_to, _topics, _ttl, _workToProve)); }
	void post(Secret const& _from, bytes const& _payload, Topics _topics, unsigned _ttl = 50, unsigned _workToProve = 50) { inject(Message(_payload).seal(_from, _topics, _ttl, _workToProve)); }
	void post(Secret const& _from, Public _to, bytes const& _payload, Topics _topics, unsigned _ttl = 50, unsigned _workToProve = 50) { inject(Message(_payload).sealTo(_from, _to, _topics, _ttl, _workToProve)); }
};
}

}

