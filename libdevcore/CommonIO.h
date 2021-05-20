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

/// Write the given binary data into the given file, replacing the file if it pre-exists.
/// Throws exception on error.
/// @param _writeDeleteRename useful not to lose any data: If set, first writes to another file in
/// the same directory and then moves that file.
void writeFile(std::string const& _file, bytesConstRef _data, bool _writeDeleteRename = false);
/// Write the given binary data into the given file, replacing the file if it pre-exists.
inline void writeFile(std::string const& _file, bytes const& _data, bool _writeDeleteRename = false) { writeFile(_file, bytesConstRef(&_data), _writeDeleteRename); }
inline void writeFile(std::string const& _file, std::string const& _data, bool _writeDeleteRename = false) { writeFile(_file, bytesConstRef(_data), _writeDeleteRename); }

/// Nicely renders the given bytes to a string, optionally as HTML.
/// @a _bytes: bytes array to be rendered as string. @a _width of a bytes line.
std::string memDump(bytes const& _bytes, unsigned _width = 8, bool _html = false);

}

