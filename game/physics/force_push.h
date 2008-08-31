/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/
#ifndef __FORCE_PUSH_H__
#define __FORCE_PUSH_H__

#include "force.h"
#include <boost/shared_ptr.hpp>

/**
 * greebo: This class should represent the push force as applied by the player
 * to large game world objects.
 */
class CForcePush : 
	public idForce
{
public:
	CLASS_PROTOTYPE( CForcePush );

						CForcePush();

	void				SetOwner(idEntity* ownerEnt);

	// Set physics object which is about to be pushed
	void				SetPushEntity(idEntity* pushEnt, int id = -1);

	// Set the push parameters for the next evaluation frame
	void				SetContactInfo(const trace_t& contactInfo, const idVec3& impactVelocity);

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

public: // common force interface
	virtual void		Evaluate( int time );

private:
	idEntity*			pushEnt;		// entity being pushed
	idEntity*			lastPushEnt;	// the entity we pushed last frame

	int					id;				// clip model id of physics object
	trace_t				contactInfo;	// the contact info of the object we're pushing
	idVec3				impactVelocity;	// the velocity the owner had at impact time
	int					startPushTime;	// the time we started to push the physics object

	idEntity*			owner;			// the owning entity
};
typedef boost::shared_ptr<CForcePush> CForcePushPtr;

#endif /* __FORCE_PUSH_H__ */
