/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
 $Revision$ (Revision of last commit) 
 $Date$ (Date of last commit)
 $Author$ (Author of last commit)
 
******************************************************************************/
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

// Used to find the highest revision of all .cpp files calling this
inline bool RegisterVersionedFile(const char* str)
{
	// greebo: Add the revision to the RevisionTracker class
	RevisionTracker::ParseSVNIdString(str);

	return true;
}

#endif /* __TDM_REVISION_TRACKER_H__ */
