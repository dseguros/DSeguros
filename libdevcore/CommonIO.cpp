#include "CommonIO.h"
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <stdio.h>
#if defined(_WIN32)
#include <windows.h>
#else
#include <termios.h>
#endif
#include <boost/filesystem.hpp>
#include "Exceptions.h"

using namespace std;
using namespace dev;

string dev::memDump(bytes const& _bytes, unsigned _width, bool _html)
{
    stringstream ret;
	if (_html)
		ret << "<pre style=\"font-family: Monospace,Lucida Console,Courier,Courier New,sans-serif; font-size: small\">";
	for (unsigned i = 0; i < _bytes.size(); i += _width)
	{
		ret << hex << setw(4) << setfill('0') << i << " ";
		for (unsigned j = i; j < i + _width; ++j)
			if (j < _bytes.size())
				if (_bytes[j] >= 32 && _bytes[j] < 127)
					if ((char)_bytes[j] == '<' && _html)
						ret << "&lt;";
					else if ((char)_bytes[j] == '&' && _html)
						ret << "&amp;";
					else
						ret << (char)_bytes[j];
				else
					ret << '?';
			else
				ret << ' ';
		ret << " ";
		for (unsigned j = i; j < i + _width && j < _bytes.size(); ++j)
			ret << setfill('0') << setw(2) << hex << (unsigned)_bytes[j] << " ";
		ret << "\n";
	}
	if (_html)
		ret << "</pre>";
	return ret.str();
}

template <typename _T>
inline _T contentsGeneric(std::string const& _file)
{
	_T ret;
	size_t const c_elementSize = sizeof(typename _T::value_type);
	std::ifstream is(_file, std::ifstream::binary);
	if (!is)
		return ret;

	// get length of file:
	is.seekg(0, is.end);
	streamoff length = is.tellg();
	if (length == 0)
		return ret; // do not read empty file (MSVC does not like it)
	is.seekg(0, is.beg);

	ret.resize((length + c_elementSize - 1) / c_elementSize);
	is.read(const_cast<char*>(reinterpret_cast<char const*>(ret.data())), length);
	return ret;
}

bytes dev::contents(string const& _file)
{
	return contentsGeneric<bytes>(_file);
}
