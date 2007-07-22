/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 866 $
 * $Date: 2007-03-23 22:25:02 +0100 (Fr, 23 Mär 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/
#ifndef SR_RESPONSEEFFECT__H
#define SR_RESPONSEEFFECT__H

class CStim;

class CResponseEffect
{
protected:
	const function_t* _scriptFunction;

	// The name of the script
	idStr _scriptName;

	// The effect postfix, "1_1" for example
	// This is passed to the script along with the "owner" entity,
	// so that the script can lookup any arguments it might need.
	idStr _effectPostfix;

	// The owning entity of the script function. If this is empty, 
	// the response script function is global.
	idEntityPtr<idEntity> _owner;

	// This is set to FALSE after loading, so that the script function
	// gets resolved again.
	bool _scriptFunctionValid;

public:
	// Pass the scriptowner to this structure or NULL for global functions
	CResponseEffect(idEntity* scriptOwner,
					const function_t* scriptFunction,
					const idStr& scriptName,
					const idStr& effectPostfix);

	void Save(idSaveGame *savefile) const;
	void Restore(idRestoreGame *savefile);

	/**
	* Runs the attached response effect script 
	* (does nothing if the scriptfunc pointer is NULL)
	*
	* @owner: The entity this script is affecting
	* @stimEntity: The entity that triggered this response
	* @magnitude: the magnitude of the stim (min = 0, max = stim->magnitude)
	*/
	virtual void runScript(idEntity* owner, idEntity* stimEntity, float magnitude);
};

#endif /* SR_RESPONSEEFFECT__H */
