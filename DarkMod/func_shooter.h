/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 866 $
 * $Date: 2007-03-23 22:25:02 +0100 (Fr, 23 Mär 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __GAME_FUNC_SHOOTER_H__
#define __GAME_FUNC_SHOOTER_H__

/**
* greebo: This entity fires projectiles in (periodic) intervals.
*		  All the key paramaters can be specified in the according entityDef.
*/
class idFuncShooter : 
	public idStaticEntity
{
public:
	CLASS_PROTOTYPE( idFuncShooter );

						idFuncShooter( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				Spawn( void );
	void				Event_Activate( idEntity *activator );

	virtual void		WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void		ReadFromSnapshot( const idBitMsgDelta &msg );

	// Overload the think function to get called each frame
	virtual void		Think();

	/**
	* Fires a projectile and sets the timer to gameLocal.time.
	*/
	virtual void		Fire();

private:
	bool				_active;

	// The time interval between fires in ms
	int					_fireInterval;

	// The last time the shooter fired a projectile
	int					_lastFireTime;
};

#endif /* !__GAME_FUNC_SHOOTER_H__ */

