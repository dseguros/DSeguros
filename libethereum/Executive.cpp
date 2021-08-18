
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

u256 Executive::gasUsed() const
{
	return m_t.gas() - m_gas;
}

void Executive::accrueSubState(SubState& _parentContext)
{
	if (m_ext)
		_parentContext += m_ext->sub;
}

void Executive::initialize(Transaction const& _transaction)
{
	m_t = _transaction;

	try
	{
		m_sealEngine.verifyTransaction(ImportRequirements::Everything, _transaction, m_envInfo);
	}
	catch (Exception const& ex)
	{
		m_excepted = toTransactionException(ex);
		throw;
	}

	// Avoid transactions that would take us beyond the block gas limit.
	u256 startGasUsed = m_envInfo.gasUsed();
	if (startGasUsed + (bigint)m_t.gas() > m_envInfo.gasLimit())
	{
		clog(ExecutiveWarnChannel) << "Cannot fit tx in block" << m_envInfo.number() << ": Require <" << (m_envInfo.gasLimit() - startGasUsed) << " Got" << m_t.gas();
		m_excepted = TransactionException::BlockGasLimitReached;
		BOOST_THROW_EXCEPTION(BlockGasLimitReached() << RequirementError((bigint)(m_envInfo.gasLimit() - startGasUsed), (bigint)m_t.gas()));
	}

	// Check gas cost is enough.
	m_baseGasRequired = m_t.baseGasRequired(m_sealEngine.evmSchedule(m_envInfo));
	if (m_baseGasRequired > m_t.gas())
	{
		clog(ExecutiveWarnChannel) << "Not enough gas to pay for the transaction: Require >" << m_baseGasRequired << " Got" << m_t.gas();
		m_excepted = TransactionException::OutOfGasBase;
		BOOST_THROW_EXCEPTION(OutOfGasBase() << RequirementError((bigint)m_baseGasRequired, (bigint)m_t.gas()));
	}

	if (!m_t.hasZeroSignature())
	{
		// Avoid invalid transactions.
		u256 nonceReq;
		try
		{
			nonceReq = m_s.getNonce(m_t.sender());
		}
		catch (InvalidSignature const&)
		{
			clog(ExecutiveWarnChannel) << "Invalid Signature";
			m_excepted = TransactionException::InvalidSignature;
			throw;
		}
		if (m_t.nonce() != nonceReq)
		{
			clog(ExecutiveWarnChannel) << "Invalid Nonce: Require" << nonceReq << " Got" << m_t.nonce();
			m_excepted = TransactionException::InvalidNonce;
			BOOST_THROW_EXCEPTION(InvalidNonce() << RequirementError((bigint)nonceReq, (bigint)m_t.nonce()));
		}

		// Avoid unaffordable transactions.
		bigint gasCost = (bigint)m_t.gas() * m_t.gasPrice();
		bigint totalCost = m_t.value() + gasCost;
		if (m_s.balance(m_t.sender()) < totalCost)
		{
			clog(ExecutiveWarnChannel) << "Not enough cash: Require >" << totalCost << "=" << m_t.gas() << "*" << m_t.gasPrice() << "+" << m_t.value() << " Got" << m_s.balance(m_t.sender()) << "for sender: " << m_t.sender();
			m_excepted = TransactionException::NotEnoughCash;
			BOOST_THROW_EXCEPTION(NotEnoughCash() << RequirementError(totalCost, (bigint)m_s.balance(m_t.sender())) << errinfo_comment(m_t.sender().abridged()));
		}
		m_gasCost = (u256)gasCost;  // Convert back to 256-bit, safe now.
	}
}

bool Executive::execute()
{
	// Entry point for a user-executed transaction.

	// Pay...
	clog(StateDetail) << "Paying" << formatBalance(m_gasCost) << "from sender for gas (" << m_t.gas() << "gas at" << formatBalance(m_t.gasPrice()) << ")";
	m_s.subBalance(m_t.sender(), m_gasCost);

	if (m_t.isCreation())
		return create(m_t.sender(), m_t.value(), m_t.gasPrice(), m_t.gas() - (u256)m_baseGasRequired, &m_t.data(), m_t.sender());
	else
		return call(m_t.receiveAddress(), m_t.sender(), m_t.value(), m_t.gasPrice(), bytesConstRef(&m_t.data()), m_t.gas() - (u256)m_baseGasRequired);
}
