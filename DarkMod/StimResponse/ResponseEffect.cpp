/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 866 $
 * $Date: 2007-03-23 22:25:02 +0100 (Fr, 23 Mär 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/
#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: ResponseEffect.cpp 870 2007-03-27 14:21:59Z greebo $", init_version);

#include "ResponseEffect.h"

/********************************************************************/
/*                 CResponseEffect                                  */
/********************************************************************/

CResponseEffect::CResponseEffect(
		idEntity* scriptOwner,
		const function_t* scriptFunction, 
		const idStr& effectPostfix,
		const idStr& scriptName	) :
	_scriptFunction(scriptFunction),
	_scriptName(scriptName),
	_effectPostfix(effectPostfix),
	_scriptFunctionValid(true)
{
	_owner = scriptOwner;
}

void CResponseEffect::runScript(idEntity* owner, idEntity* stimEntity, float magnitude) {
	if (!_scriptFunctionValid)
	{
		_scriptFunctionValid = true;

		if (_owner.GetEntity() != NULL)	{
			// Local scriptfunction
			_scriptFunction = _owner.GetEntity()->scriptObject.GetFunction(_scriptName.c_str());
		}
		else {
			// Global Method
			_scriptFunction = gameLocal.program.FindFunction(_scriptName.c_str());
		}
	}

	if (_scriptFunction == NULL) return;

	DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Running ResponseEffect Script, effectPostfix = %s...\r", _effectPostfix.c_str());
	idThread *pThread = new idThread(_scriptFunction);
	int n = pThread->GetThreadNum();
	pThread->CallFunctionArgs(_scriptFunction, true, "eesff", owner, stimEntity, _effectPostfix.c_str(), magnitude, n);
	pThread->DelayedStart(0);
}

void CResponseEffect::Save(idSaveGame *savefile) const
{
	_owner.Save(savefile);
	savefile->WriteString(_effectPostfix.c_str());
	savefile->WriteString(_scriptName.c_str());
}

void CResponseEffect::Restore(idRestoreGame *savefile)
{
	_owner.Restore(savefile);
	savefile->ReadString(_effectPostfix);
	savefile->ReadString(_scriptName);

	// The script function pointer has to be restored after loading, set the dirty flag
	_scriptFunctionValid = false;
}
