/***************************************************************************
 *
 * PROJECT: The Dark Mod - Updater
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace tdm
{

class SvnClient;
typedef boost::shared_ptr<SvnClient> SvnClientPtr;

/**
 * An object providing a few SVN client methods.
 * This is used to query file states. The implementation is
 * dependent on the preprocessor variables. In Linux the tdm_update
 * project compiles with a dummy implementation to reduce linker deps.
 */
class SvnClient
{
public:
	virtual ~SvnClient() {}

	// Activate/Deacticate the client. Deactivated clients will return true in FileIsUnderVersionControl().
	virtual void SetActive(bool active) = 0;

	// Returns true if the given file is under version control, false in all other cases
	virtual bool FileIsUnderVersionControl(const fs::path& path) = 0;

	// Factory method to retrieve an implementation of this class
	static SvnClientPtr Create();
};

// Dummy implementation returning false on all queries / doing nothing
class SvnClientDummy :
	public SvnClient
{
public:
	// Activate/Deacticate the client. Deactivated clients will return true in FileIsUnderVersionControl().
	void SetActive(bool active)
	{}

	// Returns true if the given file is under version control, false in all other cases
	bool FileIsUnderVersionControl(const fs::path& path)
	{
		return false;
	}
};

} // namespace
