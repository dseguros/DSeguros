#pragma once

#include <string>
#include "CommonIO.h"

namespace dev
{

/// Sets the data dir for the default ("ethereum") prefix.
void setDataDir(std::string const& _dir);
/// @returns the path for user data.
std::string getDataDir(std::string _prefix = "ethereum");
/// @returns the default path for user data, ignoring the one set by `setDataDir`.
std::string getDefaultDataDir(std::string _prefix = "ethereum");
/// Sets the ipc socket dir
void setIpcPath(std::string const& _ipcPath);
/// @returns the ipc path (default is DataDir)
std::string getIpcPath();

}
