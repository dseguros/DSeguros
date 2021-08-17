
#include "Executive.h"

#include <boost/timer.hpp>
#include <json/json.h>
#include <libdevcore/CommonIO.h>
#include <libevm/VMFactory.h>
#include <libevm/VM.h>
#include <libethcore/CommonJS.h>
#include "Interface.h"
#include "State.h"
#include "ExtVM.h"
#include "BlockChain.h"
#include "Block.h"
using namespace std;
using namespace dev;
using namespace dev::eth;

const char* VMTraceChannel::name() { return "EVM"; }
const char* ExecutiveWarnChannel::name() { return WarnChannel::name(); }

StandardTrace::StandardTrace():
	m_trace(Json::arrayValue)
{}

bool changesMemory(Instruction _inst)
{
	return
		_inst == Instruction::MSTORE ||
		_inst == Instruction::MSTORE8 ||
		_inst == Instruction::MLOAD ||
		_inst == Instruction::CREATE ||
		_inst == Instruction::CALL ||
		_inst == Instruction::CALLCODE ||
		_inst == Instruction::SHA3 ||
		_inst == Instruction::CALLDATACOPY ||
		_inst == Instruction::CODECOPY ||
		_inst == Instruction::EXTCODECOPY ||
		_inst == Instruction::DELEGATECALL;
}

bool changesStorage(Instruction _inst)
{
	return _inst == Instruction::SSTORE;
}

void StandardTrace::operator()(uint64_t _steps, uint64_t PC, Instruction inst, bigint newMemSize, bigint gasCost, bigint gas, VM* voidVM, ExtVMFace const* voidExt)
{
	(void)_steps;

	ExtVM const& ext = dynamic_cast<ExtVM const&>(*voidExt);
	VM& vm = *voidVM;

	Json::Value r(Json::objectValue);

	Json::Value stack(Json::arrayValue);
	if (!m_options.disableStack)
	{
		for (auto const& i: vm.stack())
			stack.append("0x" + toHex(toCompactBigEndian(i, 1)));
		r["stack"] = stack;
	}

	bool newContext = false;
	Instruction lastInst = Instruction::STOP;

	if (m_lastInst.size() == ext.depth)
	{
		// starting a new context
		assert(m_lastInst.size() == ext.depth);
		m_lastInst.push_back(inst);
		newContext = true;
	}
	else if (m_lastInst.size() == ext.depth + 2)
	{
		m_lastInst.pop_back();
		lastInst = m_lastInst.back();
	}
	else if (m_lastInst.size() == ext.depth + 1)
	{
		// continuing in previous context
		lastInst = m_lastInst.back();
		m_lastInst.back() = inst;
	}
	else
	{
		cwarn << "GAA!!! Tracing VM and more than one new/deleted stack frame between steps!";
		cwarn << "Attmepting naive recovery...";
		m_lastInst.resize(ext.depth + 1);
	}

	Json::Value memJson(Json::arrayValue);
	if (!m_options.disableMemory && (changesMemory(lastInst) || newContext))
	{
		for (unsigned i = 0; i < vm.memory().size(); i += 32)
		{
			bytesConstRef memRef(vm.memory().data() + i, 32);
			memJson.append(toHex(memRef, 2, HexPrefix::DontAdd));
		}
		r["memory"] = memJson;
	}

	if (!m_options.disableStorage && (m_options.fullStorage || changesStorage(lastInst) || newContext))
	{
		Json::Value storage(Json::objectValue);
		for (auto const& i: ext.state().storage(ext.myAddress))
			storage["0x" + toHex(toCompactBigEndian(i.second.first, 1))] = "0x" + toHex(toCompactBigEndian(i.second.second, 1));
		r["storage"] = storage;
	}

	if (m_showMnemonics)
		r["op"] = instructionInfo(inst).name;
	r["pc"] = toString(PC);
	r["gas"] = toString(gas);
	r["gasCost"] = toString(gasCost);
	if (!!newMemSize)
		r["memexpand"] = toString(newMemSize);

	m_trace.append(r);
}

string StandardTrace::json(bool _styled) const
{
	return _styled ? Json::StyledWriter().write(m_trace) : Json::FastWriter().write(m_trace);
}

Executive::Executive(Block& _s, BlockChain const& _bc, unsigned _level):
	m_s(_s.mutableState()),
	m_envInfo(_s.info(), _bc.lastHashes(_s.info().parentHash())),
	m_depth(_level),
	m_sealEngine(*_bc.sealEngine())
{
}

Executive::Executive(Block& _s, LastHashes const& _lh, unsigned _level):
	m_s(_s.mutableState()),
	m_envInfo(_s.info(), _lh),
	m_depth(_level),
	m_sealEngine(*_s.sealEngine())
{
}

Executive::Executive(State& _s, Block const& _block, unsigned _txIndex, BlockChain const& _bc, unsigned _level):
	m_s(_s = _block.fromPending(_txIndex)),
	m_envInfo(_block.info(), _bc.lastHashes(_block.info().parentHash()), _txIndex ? _block.receipt(_txIndex - 1).gasUsed() : 0),
	m_depth(_level),
	m_sealEngine(*_bc.sealEngine())
{
}
