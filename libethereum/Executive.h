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

class Executive
{
public:
	/// Simple constructor; executive will operate on given state, with the given environment info.
	Executive(State& _s, EnvInfo const& _envInfo, SealEngineFace const& _sealEngine, unsigned _level = 0): m_s(_s), m_envInfo(_envInfo), m_depth(_level), m_sealEngine(_sealEngine) {}

	/** Easiest constructor.
	 * Creates executive to operate on the state of end of the given block, populating environment
	 * info from given Block and the LastHashes portion from the BlockChain.
	 */
	Executive(Block& _s, BlockChain const& _bc, unsigned _level = 0);

/** LastHashes-split constructor.
	 * Creates executive to operate on the state of end of the given block, populating environment
	 * info accordingly, with last hashes given explicitly.
	 */
	Executive(Block& _s, LastHashes const& _lh = LastHashes(), unsigned _level = 0);

	/** Previous-state constructor.
	 * Creates executive to operate on the state of a particular transaction in the given block,
	 * populating environment info from the given Block and the LastHashes portion from the BlockChain.
	 * State is assigned the resultant value, but otherwise unused.
	 */
	Executive(State& _s, Block const& _block, unsigned _txIndex, BlockChain const& _bc, unsigned _level = 0);

	Executive(Executive const&) = delete;
	void operator=(Executive) = delete;

	/// Initializes the executive for evaluating a transaction. You must call finalize() at some point following this.
	void initialize(bytesConstRef _transaction) { initialize(Transaction(_transaction, CheckTransaction::None)); }
	void initialize(Transaction const& _transaction);
	/// Finalise a transaction previously set up with initialize().
	/// @warning Only valid after initialize() and execute(), and possibly go().
	void finalize();
	/// Begins execution of a transaction. You must call finalize() following this.
	/// @returns true if the transaction is done, false if go() must be called.
	bool execute();
	/// @returns the transaction from initialize().
	/// @warning Only valid after initialize().
	Transaction const& t() const { return m_t; }
	/// @returns the log entries created by this operation.
	/// @warning Only valid after finalise().
	LogEntries const& logs() const { return m_logs; }
	/// @returns total gas used in the transaction/operation.
	/// @warning Only valid after finalise().
	u256 gasUsed() const;

}

}
}

