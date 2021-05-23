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

};
