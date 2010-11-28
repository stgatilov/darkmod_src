/***************************************************************************
 *
 * PROJECT: The Dark Mod - Updater
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#include "MirrorDownload.h"

#include "../TraceLog.h"

#ifdef max
#undef max
#endif

namespace tdm
{

// Init the static randomizer
boost::mt19937 MirrorDownload::_rand;

MirrorDownload::MirrorDownload(const HttpConnectionPtr& conn, 
							   const MirrorList& mirrors, 
							   const std::string& srcFilename,
							   const fs::path& destFilename) :
	Download(conn, "", destFilename) // empty URL for starters
{
	if (mirrors.empty())
	{
		throw std::runtime_error("No mirrors available, cannot continue.");
	}

	// Pick a random mirror
	float p = static_cast<float>(_rand()) / _rand.max();

	// Generate the list of URLs to use, starting with the "preferred"
	// mirror on top of the list. The "preferred" mirror is the one picked
	// by the random number generator, respecting the weights.
	std::list<std::string> urls;
	std::list<Mirror> orderedMirrors;

	// Re-sort the incoming mirrors into the URL list
	for (MirrorList::const_iterator m = mirrors.begin(); m != mirrors.end(); ++m)
	{
		if (p < m->weight)
		{
			// Preferred mirror, add at front
			p = 1000000.0f; // make sure the rest of the mirrors isn't pushed at the front
			orderedMirrors.push_front(*m);
			urls.push_front(m->url + srcFilename);
			TraceLog::WriteLine(LOG_VERBOSE, "Picking mirror " + m->displayName);
		}
		else
		{
			// Non-preferred mirror, add at the end of the list
			p -= m->weight;
			urls.push_back(m->url + srcFilename);
			orderedMirrors.push_back(*m);
		}
	}

	if (urls.empty())
	{
		throw std::runtime_error("No URLs available after mirror sort, cannot continue.");
	}

	// Push the sorted URLs into the protected member
	_urls = std::vector<std::string>(urls.begin(), urls.end());

	// Push the sorted Mirrors into our own _mirrors member
	_mirrors = std::vector<Mirror>(orderedMirrors.begin(), orderedMirrors.end());
}

std::string MirrorDownload::GetCurrentMirrorName()
{
	return (_curUrl >= 0 && _curUrl < _mirrors.size()) ? _mirrors[_curUrl].displayName : "";
}

void MirrorDownload::InitRandomizer()
{
	_rand.seed(static_cast<boost::uint32_t>(std::time(0)));
}

} // namespace
