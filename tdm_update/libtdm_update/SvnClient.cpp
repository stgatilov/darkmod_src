#include "SvnClient.h"

#include <svn_client.h>
#include <svn_pools.h>
#include <svn_fs.h>
#include <svn_config.h>
#include <svn_cmdline.h>

#include "TraceLog.h"

namespace tdm
{

SvnClient::SvnClient() :
	_isActive(true)
{
	// Initialize the app.  Send all error messages to nothing
	if (svn_cmdline_init("minimal_client", NULL) != EXIT_SUCCESS)
	{
		throw std::runtime_error("Could not initialise SVN commandline.");
	}

	// Create our main pool
	_pool = svn_pool_create(NULL);

	/* Initialize the FS library. */
	svn_error_t* err = svn_fs_initialize(_pool);

	if (err)
	{
		throw std::runtime_error("Could not initialise SVN filesystem library.");
	}

	/* Make sure the ~/.subversion run-time config files exist */
	err = svn_config_ensure(NULL, _pool);

	if (err)
	{
		throw std::runtime_error("Could not initialise SVN configuration.");
	}

	// Initialize and allocate the client_ctx object. 
	if ((err = svn_client_create_context(&_context, _pool)))
	{
		throw std::runtime_error("Could not create SVN context.");
	}

	// Load the run-time config file into a hash
	if ((err = svn_config_get_config (&(_context->config), NULL, _pool)))
	{
		throw std::runtime_error("Could not load SVN config.");
	}

#ifdef WIN32
	// Set the working copy administrative directory name.
	if (getenv("SVN_ASP_DOT_NET_HACK"))
	{
		err = svn_wc_set_adm_dir("_svn", _pool);
		
		if (err)
		{
			throw std::runtime_error("Could not set SVN admin directory.");
		}
	}
#endif	
}

SvnClient::~SvnClient()
{
	svn_pool_destroy(_pool);
}

void SvnClient::SetActive(bool active)
{
	if (!active)
	{
		TraceLog::WriteLine(LOG_STANDARD, "Deactivating SVN client checks.");
	}

	_isActive = active;
}

svn_error_t* info_receiver_dummy(void* baton, const char* path, const svn_info_t* info, apr_pool_t* pool)
{
	return SVN_NO_ERROR;
}

bool SvnClient::FileIsUnderVersionControl(const fs::path& path)
{
	if (!_isActive) return true; // deactivated clients will return true on all cases

	apr_pool_t* subpool = svn_pool_create(_pool);

	svn_error_t* err = svn_client_info2(path.string().c_str(),
		NULL,	// peg_revision
		NULL,	// revision
		info_receiver_dummy, // receiver
		NULL,	// receiver_baton
		svn_depth_empty,
		NULL,	// changelists
		_context,
		subpool
	);

	bool isUnderVersionControl = (err == NULL);

	svn_pool_destroy(subpool);

	return isUnderVersionControl;
}

} // namespace
