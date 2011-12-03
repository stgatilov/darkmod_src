/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/
#ifndef __TDM_REVISION_TRACKER_H__
#define __TDM_REVISION_TRACKER_H__

/**
 * greebo: This class is a simple singleton keeping track of the highest and 
 * lowest SVN revision numbers. 
 *
 * The global function FileRevisionList() is taking care of registering 
 * the revision numbers to this class.
 */
class RevisionTracker
{
	// Some stats
	int _highestRevision;
	int _lowestRevision;
	int _numFiles;

public:
	/**
	 * Constructor is taking care of initialising the members.
	 */
	RevisionTracker();

	/**
	 * greebo: Accessor methods. Retrieves the highest and lowest revision
	 * and the number of registered files.
	 */
	int GetHighestRevision() const;
	int GetLowestRevision() const;
	int GetNumFiles() const;

	/**
	 * Use this method to register a new revision.
	 */
	void AddRevision(int revision);

	/**
	 * Entry point for FileVersionList(). Take this method to
	 * parse an SVN id string and retrieve the revision.
	 * This methods passes the call to AddRevision.
	 */
	static void ParseSVNIdString(const char* input);

	// Accessor to the singleton
	static RevisionTracker& Instance();
};

#endif /* __TDM_REVISION_TRACKER_H__ */
