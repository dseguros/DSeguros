#pragma once

#include "VMFace.h"

namespace dev
{
namespace eth
{

enum class VMKind
{
	Interpreter,
	JIT,
	Smart
};

class VMFactory
{
public:
	VMFactory() = delete;

	/// Creates a VM instance of global kind (controlled by setKind() function).
	static std::unique_ptr<VMFace> create();

	/// Creates a VM instance of kind provided.
	static std::unique_ptr<VMFace> create(VMKind _kind);

	/// Set global VM kind
	static void setKind(VMKind _kind);
};

}
}
