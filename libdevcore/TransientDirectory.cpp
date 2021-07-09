
#include <thread>
#include <boost/filesystem.hpp>
#include "Exceptions.h"
#include "TransientDirectory.h"
#include "CommonIO.h"
#include "Log.h"
using namespace std;
using namespace dev;
namespace fs = boost::filesystem;

TransientDirectory::TransientDirectory():
	TransientDirectory((boost::filesystem::temp_directory_path() / "eth_transient" / toString(FixedHash<4>::random())).string())
{}

TransientDirectory::TransientDirectory(std::string const& _path):
	m_path(_path)
{
	// we never ever want to delete a directory (including all its contents) that we did not create ourselves.
	if (boost::filesystem::exists(m_path))
		BOOST_THROW_EXCEPTION(FileError());

	if (!fs::create_directories(m_path))
		BOOST_THROW_EXCEPTION(FileError());
	DEV_IGNORE_EXCEPTIONS(fs::permissions(m_path, fs::owner_all));
}

