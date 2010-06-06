/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
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
	idStr					_bloodSplatFading;
	float					_angle;
	float					_size;

	// True if this bloodsplat is in the process of disappearing
	bool					_isFading;

public:
	void					Init(const idStr& splat, const idStr& splatFading, float size);
	void					Event_GenerateBloodSplat();

	/**
	 * greebo: Overrides the OnStim method of the base class to check
	 * for water stims.
	 */
	void					OnStim(const CStimPtr& stim, idEntity* stimSource);

	// Save and restore
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );
};

// End of header wrapper
#endif
