#pragma once

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <list>
#include <memory>
#include <vector>
#include <array>
#include <sstream>
#include <string>
#include <iostream>
#include <chrono>
#include "Common.h"
#include "CommonData.h"
#include "Base64.h"

namespace dev
{

/// Requests the user to enter a password on the console.
std::string getPassword(std::string const& _prompt);

/// Retrieve and returns the contents of the given file.
/// If the file doesn't exist or isn't readable, returns an empty container / bytes.
bytes contents(std::string const& _file);
/// Secure variation.
bytesSec contentsSec(std::string const& _file);
/// Retrieve and returns the contents of the given file as a std::string.
/// If the file doesn't exist or isn't readable, returns an empty container / bytes.
std::string contentsString(std::string const& _file);
/// Retrieve and returns the allocated contents of the given file; if @_dest is given, don't allocate, use it directly.
/// If the file doesn't exist or isn't readable, returns bytesRef(). Don't forget to delete [] the returned value's data when finished.
bytesRef contentsNew(std::string const& _file, bytesRef _dest = bytesRef());

}

