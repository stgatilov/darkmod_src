/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.7  2006/08/07 06:52:08  ishtvan
 * *) added distance control
 *
 * *) Grabber now always grabs the center of mass
 *
 * *) StartGrab function added that may be called by the inventory to drop stuff to hands
 *
 * Revision 1.6  2006/08/04 10:53:26  ishtvan
 * preliminary grabber fixes
 *
 * Revision 1.5  2006/02/23 10:20:19  ishtvan
 * throw implemented
 *
 * Revision 1.4  2005/12/09 05:12:48  lloyd
 * Various bug fixes (AF grabbing, mouse deadzone, mouse sensitivty, ...)
 *
 * Revision 1.3  2005/12/02 18:21:04  lloyd
 * Objects start oriented with player
 *
 * Revision 1.1.1.1  2005/09/22 15:52:33  Lloyd
 * Initial release
 *
 ***************************************************************************/

#ifndef __GRABBER_H__
#define __GRABBER_H__

#include "../Game/Entity.h"
#include "Force_Grab.h"

class idPlayer;

extern const idEventDef EV_Grabber_CheckClipList;

class CGrabbedEnt {
	public: 
		idEntity	*ent;
		int			clipMask;

		bool operator==( const CGrabbedEnt &right ) const {
			if( right.ent == this->ent )
				return true;
			return false;
		}
};

class CGrabber : public idEntity {
public:
		CLASS_PROTOTYPE( CGrabber );


								CGrabber( void );
								~CGrabber( void );

		void					Clear( void );
		void					Update( idPlayer *player, bool hold = false );

		void					Spawn( void );

		idEntity *				GetSelected( void ) const { return dragEnt.GetEntity(); }

		bool					IsInClipList( idEntity *ent ) const;
		bool					HasClippedEntity( void ) const;

		/**
		* Clamp the current velocity to max velocity
		**/
		void					ClampVelocity( float maxLin, float maxAng, int idVal = 0 );

		/**
		* Increment the distance at which the held item is held.
		* Decrements the distance if the argument is false
		**/
		void					IncrementDistance( bool bIncrease );

		/**
		* Start grabbing an item.  Called internally and by the inventory
		**/
		void					StartGrab( idPlayer *player, idEntity *newEnt = NULL, int bodyID = 0 );

public:
		/**
		* Set to true if the grabbed entity is colliding this frame
		**/
		bool					m_bIsColliding;

		/**
		* Set to true if the grabbed entity collided in the previous frame
		**/
		bool					m_bPrevFrameCollided;

protected:

		/**
		* Performs object rotation
		* Also turns the item to face the player when they yaw their view.
		**/
		void					ManipulateObject( idPlayer *player );
		
		void					AddToClipList( idEntity *ent );
		void					RemoveFromClipList( int index );

		void					Event_CheckClipList( void );

		/**
		* Throws the current item.
		* Argument is the amount of time the throw button has been held,
		* used to determine strength of the throw
		**/
		void					Throw( int HeldTime );

protected:

		idEntityPtr<idEntity>	dragEnt;			// entity being dragged
		jointHandle_t			joint;				// joint being dragged
		int						id;					// id of body being dragged
		idVec3					localEntityPoint;	// dragged point in entity space
		idVec3					localPlayerPoint;	// dragged point in player space (not used anymore)

		idPlayer				*player;
		CForce_Grab				drag;

		idRotation				rotation;
		int						rotationAxis;		// 0 = none, 1 = x, 2 = y, 3 = z
		idVec3					rotatePosition;		// how much to rotate the object
		idVec2					mousePosition;		// mouse position when user pressed BUTTON_ZOOM

		idList<CGrabbedEnt>		clipList;

		void					StopDrag( void );
		bool					DeadMouse( void );	// returns true if the mouse is inside the dead zone

		/**
		* Set to true if the attack button has been pressed (used by throwing)
		**/
		bool					m_bAttackPressed;
		/**
		* Timestamp of when attack button was last pressed (used by throwing)
		**/
		int						m_ThrowTimer;

		/**
		* Int storing the distance increments for held item distance.
		* When this is equal to m_MaxDistCount, it is held at the maximum
		* distance (the frob distance).
		**/
		int						m_DistanceCount;

		/**
		* Maximum distance increments at which the item can be held.
		* Corresponds to 1.0 * the max distance.
		**/
		int						m_MaxDistCount;

		/**
		* Minimum held distance (can be unique to each entity)
		* NOTE: The maximum held distance is always the frob distance of that ent
		**/
		int						m_MinHeldDist;

		/**
		* When colliding, the held distance gets locked to +/- two increments
		* around this value.  This is to prevent the player from going way
		* past the collision point and having to increment all the way back
		* before they see any response.
		**/
		int						m_LockedHeldDist;
};


#endif /* !__GRABBER_H__ */