/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 3079 $
 * $Date: 2008-12-06 09:28:50 +0100 (Sa, 06 Dez 2008) $
 * $Author: angua $
 *
 ***************************************************************************/

#ifndef BLOODMARKER_H
#define BLOODMARKER_H

#pragma hdrstop

// Includes
#include "../game/entity.h"

class CBloodMarker : public idEntity
{
public:
	CLASS_PROTOTYPE( CBloodMarker );

protected:
	idStr					_bloodSplat;
	float					_angle;
	float					_size;

public:
	void					Init(const idStr& splat, float size);
	void					Event_GenerateBloodSplat();

	// Save and restore
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );
};

// End of header wrapper
#endif
