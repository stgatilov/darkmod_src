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
		const function_t* scriptFunction,
		const idStr& effectPostfix,
		const idStr& scriptName	,
		bool localScript) :
	_scriptFunction(scriptFunction),
	_scriptName(scriptName),
	_effectPostfix(effectPostfix),
	_localScript(localScript),
	_scriptFunctionValid(true)
{}

void CResponseEffect::runScript(idEntity* owner, idEntity* stimEntity, float magnitude) {
	if (!_scriptFunctionValid)
	{
		if (owner == NULL)
		{
			DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Cannot restore scriptfunction, owner is NULL: %s\r", _scriptName.c_str());
			return;
		}

		_scriptFunctionValid = true;

		if (_localScript)	{
			// Local scriptfunction
			_scriptFunction = owner->scriptObject.GetFunction(_scriptName.c_str());
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
	savefile->WriteString(_effectPostfix.c_str());
	savefile->WriteString(_scriptName.c_str());
	savefile->WriteBool(_localScript);
}

void CResponseEffect::Restore(idRestoreGame *savefile)
{
	savefile->ReadString(_effectPostfix);
	savefile->ReadString(_scriptName);
	savefile->ReadBool(_localScript);

	// The script function pointer has to be restored after loading, set the dirty flag
	_scriptFunctionValid = false;
}
