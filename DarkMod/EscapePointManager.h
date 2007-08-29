/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 866 $
 * $Date: 2007-03-23 22:25:02 +0100 (Fr, 23 Mar 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/
#ifndef ESCAPE_POINT_MANAGER__H
#define ESCAPE_POINT_MANAGER__H

#include "../idlib/precompiled.h"
#include <map>
#include <boost/shared_ptr.hpp>

template<class type>
class idEntityPtr;

class idAI;
class idEntity;
class tdmPathFlee;

typedef struct EscapeConditions_t
{
	// The AI who is fleeing
	idEntityPtr<idAI> self;

	// The position to flee from
	idVec3 fromPosition;

	// The threatening entity to flee from
	idEntityPtr<idEntity> fromEntity;

	// The AAS the fleeing AI is using.
	idAAS* aas;

	// The maximum distance
	float maxDistance;
} EscapeConditions;

// This is a result structure delivered by the escape point manager
// containing information about how to get to an escape point 
typedef struct EscapeGoal_t
{
	// The escape point entity
	idEntityPtr<tdmPathFlee> escapePoint;

	// The distance to this escape point
	float distance;
} EscapeGoal;

class CEscapePointManager
{
	// A list of Escape Point entities
	typedef idList< idEntityPtr<tdmPathFlee> > EscapePointList;

	// The pointer-type for the list above
	typedef boost::shared_ptr<EscapePointList> EscapePointListPtr;

	// A map associating an AAS to EscapePointLists.
	typedef std::map<idAAS*, EscapePointListPtr> AASEscapePointMap;

	// This is the master list containing all the escape point entities in this map
	EscapePointListPtr _escapePoints;

	// The map of escape points for each AAS type.
	// Each of the lists is a subset of the master list above
	AASEscapePointMap _aasEscapePoints;

public:

	CEscapePointManager();
	~CEscapePointManager();

	void	Clear();

	void	Save( idSaveGame *savefile ) const;
	void	Restore( idRestoreGame *savefile );

	void	AddEscapePoint(tdmPathFlee* escapePoint);
	void	RemoveEscapePoint(tdmPathFlee* escapePoint);

	/**
	 * greebo: Call this after the entities are spawned. This sets up the
	 *         AAS types for each pathFlee entity.
	 */
	void	InitAAS();

	/**
	 * greebo: Retrieve an escape point for the given escape conditions.
	 */
	EscapeGoal GetEscapePoint(const EscapeConditions& conditions);

	// Accessor to the singleton instance of this class
	static CEscapePointManager* Instance();
};

#endif /* ESCAPE_POINT_MANAGER__H */