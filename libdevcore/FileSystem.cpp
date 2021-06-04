#include "FileSystem.h"
#include "Common.h"
#include "Log.h"

#if defined(_WIN32)
#include <shlobj.h>
#else
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <unistd.h>
#endif
#include <boost/filesystem.hpp>
using namespace std;
using namespace dev;

static_assert(BOOST_VERSION == 106300, "Wrong boost headers version");

// Should be written to only once during startup
static string s_ethereumDatadir;
static string s_ethereumIpcPath;

void dev::setDataDir(string const& _dataDir)
{
	s_ethereumDatadir = _dataDir;
}
