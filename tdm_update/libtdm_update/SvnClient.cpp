/***************************************************************************
 *
 * PROJECT: The Dark Mod - Updater
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#include "SvnClient.h"

#include "SvnClientImpl.h"

namespace tdm
{

// Factory method to retrieve an implementation of this class
SvnClientPtr SvnClient::Create()
{
#ifdef USE_LIBSVN
	// libsvn enabled, return implementation using actual svn calls
	return SvnClientPtr(new SvnClientImpl);
#else
	// No libsvn included, use dummy object
	return SvnClientPtr(new SvnClientDummy);
#endif
}

} // namespace
