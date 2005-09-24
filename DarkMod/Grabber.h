/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __GRABBER_H__
#define __GRABBER_H__

#include "../Game/Entity.h"
//#include "Force_Grab.h"

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

	protected:
		void					ManipulateObject( idPlayer *player );
		
		void					AddToClipList( idEntity *ent );
		void					RemoveFromClipList( int index );

	private:
		idEntityPtr<idEntity>	dragEnt;			// entity being dragged
		jointHandle_t			joint;				// joint being dragged
		int						id;					// id of body being dragged
		idVec3					localEntityPoint;	// dragged point in entity space
		idVec3					localPlayerPoint;	// dragged point in player space
		idStr					bodyName;			// name of the body being dragged

		idPlayer				*player;
		idForce_Drag			drag;

		idVec3					rotatePosition;		// how much to rotate the object
		idVec2					mousePosition;		// mouse position when user pressed BUTTON_ZOOM
		int						rotationAxis;		// 0 = none, 1 = x, 2 = y, 3 = z

		idList<CGrabbedEnt>		clipList;

		void					StopDrag( void );
		
		void					Event_CheckClipList( void );
};


#endif /* !__GRABBER_H__ */