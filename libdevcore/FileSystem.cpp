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

void dev::setIpcPath(string const& _ipcDir)
{
	s_ethereumIpcPath = _ipcDir;
}

string dev::getIpcPath()
{
	if (s_ethereumIpcPath.empty())
		return string(getDataDir());
	else
	{
		size_t socketPos = s_ethereumIpcPath.rfind("geth.ipc");
		if (socketPos != string::npos)
			return s_ethereumIpcPath.substr(0, socketPos);
		return s_ethereumIpcPath;
	}
}

string dev::getDataDir(string _prefix)
{
	if (_prefix.empty())
		_prefix = "ethereum";
	if (_prefix == "ethereum" && !s_ethereumDatadir.empty())
		return s_ethereumDatadir;
	return getDefaultDataDir(_prefix);
}
