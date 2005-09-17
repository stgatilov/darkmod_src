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

#include "../game/GameEdit.h"
#include "../game/Entity.h"

class CGrabber : idEntity {
	public:
								CGrabber( void );
								~CGrabber( void );

		void					Clear();
		void					Update( idPlayer *player, bool hold = false );

		idEntity *				GetSelected( void ) const { return dragEnt.GetEntity(); }

	protected:
		void					ManipulateObject( idPlayer *player );

	private:
		idEntityPtr<idEntity>	dragEnt;			// entity being dragged
		jointHandle_t			joint;				// joint being dragged
		int						id;					// id of body being dragged
		idVec3					localEntityPoint;	// dragged point in entity space
		idVec3					localPlayerPoint;	// dragged point in player space
		idStr					bodyName;			// name of the body being dragged
		idCursor3D *			cursor;				// cursor entity
		idEntityPtr<idEntity>	selected;			// last dragged entity

		idVec3					rotatePosition;
		idVec2					mousePosition;

		void					StopDrag( void );
};


#endif /* !__GRABBER_H__ */