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
