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

#include "EscapePointEvaluator.h"

template<class type>
class idEntityPtr;

class idAI;
class idEntity;
class tdmPathFlee;

/**
 * greebo: The algorithm type to be used for 
 *         evaluating the escape points.
 */
enum EscapePointAlgorithm
{
	FIND_ANY,
	FIND_GUARDED,
	FIND_FRIENDLY,
	FIND_FRIENDLY_GUARDED,
};

enum EscapeDistanceOption
{
	DIST_DONT_CARE,           // Don't care whether nearer or farther
	DIST_NEAREST,             // Find the nearest
	DIST_FARTHEST,            // Find the farthest escape point
	DIST_MINIMUM_FROM_THREAT, // Find a point that is reasonably far from the threating entity
};

struct EscapeConditions
{
	// The AI who is fleeing
	idEntityPtr<idAI> self;

	// The position to flee from
	idVec3 fromPosition;

	// The threatening entity to flee from
	idEntityPtr<idEntity> fromEntity;

	// The AAS the fleeing AI is using.
	idAAS* aas;

	// Whether the distance should be considered or not
	EscapeDistanceOption distanceOption;

	// The algorithm to use
	EscapePointAlgorithm algorithm;
};

/**
 * greebo: This represents one escape point in a given AAS grid. 
 */
struct EscapePoint
{
	// A unique ID for this escape point
	int id;

	// The actual entity this escape point is located in
	idEntityPtr<tdmPathFlee> pathFlee;

	// The AAS id of this point
	int aasId;

	// The actual origin of the entity
	idVec3 origin;

	// The AAS area number of the entity's origin.
	int areaNum;

	// The team this escape point is belonging to (default = 0, neutral)
	int team;

	// TRUE, if an armed AI is supposed to hang around at the escape point.
	bool isGuarded;

	// Constructor
	EscapePoint() :
		aasId(-1),
		areaNum(-1),
		isGuarded(false),
		team(0) // neutral
	{}
};

// This is a result structure delivered by the escape point manager
// containing information about how to get to an escape point 
struct EscapeGoal
{
	// The escape point ID (valid IDs are > 0)
	int escapePointId;

	// The distance to this escape point
	float distance;
};

class CEscapePointManager
{
	// A list of Escape Point entities
	typedef idList< idEntityPtr<tdmPathFlee> > EscapeEntityList;

	// The pointer-type for the list above
	typedef boost::shared_ptr<EscapeEntityList> EscapeEntityListPtr;

	// The list of AAS-specific escape points plus shared_ptr typedef.
	typedef idList<EscapePoint> EscapePointList;
	typedef boost::shared_ptr<EscapePointList> EscapePointListPtr;

	// A map associating an AAS to EscapePointLists.
	typedef std::map<idAAS*, EscapePointListPtr> AASEscapePointMap;

	// A map associating EscPointIds to EscPoints for fast lookup during runtime
	typedef std::map<int, EscapePoint*> EscapePointIndex;

	// This is the master list containing all the escape point entities in this map
	EscapeEntityListPtr _escapeEntities;

	// The map of escape points for each AAS type.
	AASEscapePointMap _aasEscapePoints;

	// A lookup table for EscapePoints (by unique ID)
	EscapePointIndex _aasEscapePointIndex;

	// The highest used escape point ID
	int _highestEscapePointId;

public:

	CEscapePointManager();
	~CEscapePointManager();

	void	Clear();

	void	Save( idSaveGame *savefile ) const;
	void	Restore( idRestoreGame *savefile );

	void	AddEscapePoint(tdmPathFlee* escapePoint);
	void	RemoveEscapePoint(tdmPathFlee* escapePoint);

	// Retrieve the escape point with the given unique ID.
	EscapePoint* GetEscapePoint(int id);

	/**
	 * greebo: Call this after the entities are spawned. This sets up the
	 *         AAS types for each pathFlee entity.
	 */
	void	InitAAS();

	/**
	 * greebo: Retrieve an escape goal for the given escape conditions.
	 */
	EscapeGoal GetEscapeGoal(const EscapeConditions& conditions);

	// Accessor to the singleton instance of this class
	static CEscapePointManager* Instance();
};

#endif /* ESCAPE_POINT_MANAGER__H */