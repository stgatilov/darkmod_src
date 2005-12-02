/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.1  2005/12/02 18:21:29  lloyd
 * Initial release
 *
 * Revision 1.1.1.1  2005/09/22 15:52:33  Lloyd
 * Initial release
 *
 ***************************************************************************/


#ifndef __FORCE_GRAB_H__
#define __FORCE_GRAB_H__

#include "../Game/Physics/Force.h"

/*
===============================================================================

	Grab force

===============================================================================
*/

class CForce_Grab : public idForce {
	public:
		CLASS_PROTOTYPE( CForce_Grab );

							CForce_Grab( void );
		virtual				~CForce_Grab( void );
							// initialize the drag force
		void				Init( float damping );
							// set physics object being dragged
		void				SetPhysics( idPhysics *physics, int id, const idVec3 &p );
							// set position to drag towards
		void				SetDragPosition( const idVec3 &pos );
							// get the position dragged towards
		const idVec3 &		GetDragPosition( void ) const;
							// get the position on the dragged physics object
		const idVec3		GetDraggedPosition( void ) const;

							// Gets the center of mass of the grabbed object
		idVec3				GetCenterOfMass( void ) const;
							// rotates p about the center of mass of the grabbed object
		void				Rotate( const idVec3 &vec, float angle );

	public: // common force interface
		virtual void		Evaluate( int time );
		virtual void		RemovePhysics( const idPhysics *phys );

	private:

		// properties
		float				damping;

		// physics object properties
		idVec3				centerOfMass;

		// positioning
		idPhysics *			physics;		// physics object
		idVec3				p;				// position on clip model
		int					id;				// clip model id of physics object
		idVec3				dragPosition;	// drag towards this position
};

#endif /* !__FORCE_GRAB_H__ */
