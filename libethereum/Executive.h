#pragma once

#include <functional>
#include <json/json.h>
#include <libdevcore/Log.h>
#include <libevmcore/Instruction.h>
#include <libethcore/Common.h>
#include <libevm/VMFace.h>
#include "Transaction.h"

namespace Json
{
	class Value;
}


namespace dev
{

class OverlayDB;

namespace eth
{

class State;
class Block;
class BlockChain;
class ExtVM;
class SealEngineFace;
struct Manifest;

struct VMTraceChannel: public LogChannel { static const char* name(); static const int verbosity = 11; };
struct ExecutiveWarnChannel: public LogChannel { static const char* name(); static const int verbosity = 1; };

class StandardTrace
{
public:
	struct DebugOptions
	{
		bool disableStorage = false;
		bool disableMemory = false;
		bool disableStack = false;
		bool fullStorage = false;
	};

	StandardTrace();
	void operator()(uint64_t _steps, uint64_t _PC, Instruction _inst, bigint _newMemSize, bigint _gasCost, bigint _gas, VM* _vm, ExtVMFace const* _extVM);

	void setShowMnemonics() { m_showMnemonics = true; }
	void setOptions(DebugOptions _options) { m_options = _options; }

	std::string json(bool _styled = false) const;

	OnOpFunc onOp() { return [=](uint64_t _steps, uint64_t _PC, Instruction _inst, bigint _newMemSize, bigint _gasCost, bigint _gas, VM* _vm, ExtVMFace const* _extVM) { (*this)(_steps, _PC, _inst, _newMemSize, _gasCost, _gas, _vm, _extVM); }; }

private:
	bool m_showMnemonics = false;
	std::vector<Instruction> m_lastInst;
	bytes m_lastCallData;
	Json::Value m_trace;
	DebugOptions m_options;
};

}
}

