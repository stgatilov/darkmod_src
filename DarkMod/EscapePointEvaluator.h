/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 866 $
 * $Date: 2007-03-23 22:25:02 +0100 (Fr, 23 Mar 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/
#ifndef ESCAPE_POINT_EVALUATOR__H
#define ESCAPE_POINT_EVALUATOR__H

#include "../idlib/precompiled.h"

#include <boost/shared_ptr.hpp>

// Forward Declaration
class EscapePoint;
struct EscapeConditions;

/**
 * greebo: An EscapePointEvaluator gets fed with one EscapePoint after the
 *         other (by any algorithm) and inspects each of them. The most
 *         suitable one is stored internally and can be retrieved at any time
 *         by using GetBestEscapePoint().
 */
class EscapePointEvaluator
{
protected:
	const EscapeConditions& _conditions;

	// This holds the ID of the best escape point so far
	int _bestId;

	// The area number the AI starts to flee in
	int _startAreaNum;

		// The best travel time so far
	int _bestTime;

	// This is either -1 (find farthest) or 1 (find nearest)
	int _distanceMultiplier;

	idVec3 _threatPosition;

public:
	// Default Constructor
	EscapePointEvaluator(const EscapeConditions& conditions);

	/**
	 * greebo: Evaluate the given escape point.
	 *
	 * @returns: FALSE means that the evaluation can be stopped (prematurely),
	 *           no more EscapePoints need to be passed.
	 */
	virtual bool Evaluate(EscapePoint& escapePoint) = 0;

	/**
	 * greebo: Returns the ID of the best found escape point.
	 *
	 * @returns: an ID of -1 is returned if no point was found to be suitable.
	 */
	virtual inline int GetBestEscapePoint()
	{
		return _bestId;
	}

protected:
	/**
	 * Performs the distance check according to the escape conditions.
	 * If the given escapePoint is better, the _bestId is updated.
	 *
	 * @returns FALSE if the search is finished (DIST_DONT_CARE) or 
	 *          TRUE if the search can continue.
	 */
	bool	PerformDistanceCheck(EscapePoint& escapePoint);
};
typedef boost::shared_ptr<EscapePointEvaluator> EscapePointEvaluatorPtr;

/**
 * ==== EVALUATOR IMPLEMENTATIONS === 
 */

/**
 * greebo: This visitor returns the escape point which is nearest or farthest away
 *         from the threatening entity. This is determined by the algorithm in the
 *         EscapeConditions structure.
 */
class AnyEscapePointFinder :
	public EscapePointEvaluator
{
public:
	AnyEscapePointFinder(const EscapeConditions& conditions);

	virtual bool Evaluate(EscapePoint& escapePoint);
};

/**
 * greebo: This visitor tries to locate a guarded escape point.
 */
class GuardedEscapePointFinder :
	public EscapePointEvaluator
{
public:
	GuardedEscapePointFinder(const EscapeConditions& conditions);

	virtual bool Evaluate(EscapePoint& escapePoint);
};

/**
 * greebo: This visitor tries to locate a friendly escape point.
 *         Whether an escape point is friendly or not is determined
 *         by the "team" spawnarg on the PathFlee entity and is checked
 *         using the RelationsManager.
 */
class FriendlyEscapePointFinder :
	public EscapePointEvaluator
{
	// The team of the fleeing AI, which is evaluated against the
	// team of the escape point.
	int _team;

public:
	FriendlyEscapePointFinder(const EscapeConditions& conditions);

	virtual bool Evaluate(EscapePoint& escapePoint);
};

/**
 * greebo: This visitor tries to locate a friendly AND guarded escape point.
 *         Whether an escape point is friendly or not is determined
 *         by the "team" spawnarg on the PathFlee entity and is checked
 *         using the RelationsManager.
 */
class FriendlyGuardedEscapePointFinder :
	public EscapePointEvaluator
{
	// The team of the fleeing AI, which is evaluated against the
	// team of the escape point.
	int _team;

public:
	FriendlyGuardedEscapePointFinder(const EscapeConditions& conditions);

	virtual bool Evaluate(EscapePoint& escapePoint);
};

#endif /* ESCAPE_POINT_EVALUATOR__H */