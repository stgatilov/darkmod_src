/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.1  2004/10/30 15:52:31  sparhawk
 * Initial revision
 *
 ***************************************************************************/

// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __GAME_SOUND_H__
#define __GAME_SOUND_H__

/*
===============================================================================

  Generic sound emitter.

===============================================================================
*/

class idSound : public idEntity {
public:
	CLASS_PROTOTYPE( idSound );

					idSound( void );

	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

	virtual void	UpdateChangeableSpawnArgs( const idDict *source );

	void			Spawn( void );

	void			ToggleOnOff( idEntity *other, idEntity *activator );
	void			Think( void );
	void			SetSound( const char *sound, int channel = SND_CHANNEL_ANY );

	virtual void	ShowEditingDialog( void );

private:
	float			lastSoundVol;
	float			soundVol;
	float			random;
	float			wait;
	bool			timerOn;
	idVec3			shakeTranslate;
	idAngles		shakeRotate;
	int				playingUntilTime;

	void			Event_Trigger( idEntity *activator );
	void			Event_Timer( void );
	void			Event_On( void );
	void			Event_Off( void );
	void			DoSound( bool play );
};

#endif /* !__GAME_SOUND_H__ */
