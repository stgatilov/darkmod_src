/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 866 $
 * $Date: 2007-03-23 22:25:02 +0100 (Fr, 23 M�r 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/
#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: Response.cpp 870 2007-03-27 14:21:59Z greebo $", init_version);

#include "Response.h"
#include "Stim.h"

#include <algorithm>

/********************************************************************/
/*                   CResponse                                      */
/********************************************************************/
CResponse::CResponse(idEntity *e, int Type, int uniqueId)
: CStimResponse(e, Type, uniqueId)
{
	m_ScriptFunction = NULL;
	m_MinDamage = 0.0f;
	m_MaxDamage = 0;
	m_NumRandomEffects = 0;
}

CResponse::~CResponse(void)
{
	// Remove all the allocated response effects from the heap
	for (int i = 0; i < m_ResponseEffects.Num(); i++)
		delete m_ResponseEffects[i];
}

void CResponse::Save(idSaveGame *savefile) const
{
	CStimResponse::Save(savefile);

	savefile->WriteString(m_ScriptFunction.c_str());
	savefile->WriteFloat(m_MinDamage);
	savefile->WriteFloat(m_MaxDamage);
	savefile->WriteInt(m_NumRandomEffects);

	savefile->WriteInt(m_ResponseEffects.Num());
	for (int i = 0; i < m_ResponseEffects.Num(); i++)
	{
		m_ResponseEffects[i]->Save(savefile);
	}
}

void CResponse::Restore(idRestoreGame *savefile)
{
	CStimResponse::Restore(savefile);

	savefile->ReadString(m_ScriptFunction);
	savefile->ReadFloat(m_MinDamage);
	savefile->ReadFloat(m_MaxDamage);
	savefile->ReadInt(m_NumRandomEffects);

	int num;
	savefile->ReadInt(num);
	m_ResponseEffects.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		m_ResponseEffects[i] = new CResponseEffect(NULL, "", "", false);
		m_ResponseEffects[i]->Restore(savefile);
	}
}

void CResponse::TriggerResponse(idEntity *sourceEntity, CStim* stim)
{
	DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Response for Id %s triggered (Action: %s)\r", m_StimTypeName.c_str(), m_ScriptFunction.c_str());

	// Perform the probability check
	if (!checkChance()) {
		return;
	}

	const function_t *pScriptFkt = m_Owner.GetEntity()->scriptObject.GetFunction(m_ScriptFunction.c_str());
	if(pScriptFkt == NULL)
	{
		DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Action: %s not found in local space, checking for global.\r", m_ScriptFunction.c_str());
		pScriptFkt = gameLocal.program.FindFunction(m_ScriptFunction.c_str());
	}

	if(pScriptFkt)
	{
		DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Running ResponseScript\r");
		idThread *pThread = new idThread(pScriptFkt);
		int n = pThread->GetThreadNum();
		pThread->CallFunctionArgs(pScriptFkt, true, "eef", m_Owner.GetEntity(), sourceEntity, n);
		pThread->DelayedStart(0);
	}
	else
	{
		DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("ResponseActionScript not found! [%s]\r", m_ScriptFunction.c_str());
	}

	// Default magnitude (in case we have a NULL stim)
	float magnitude = 10;

	if (stim != NULL) {
		// We have a "real" stim causing this response, retrieve the properties

		// Calculate the magnitude of the stim based on the distance and the falloff model
		magnitude = stim->m_Magnitude;
		float distance = (m_Owner.GetEntity()->GetPhysics()->GetOrigin() - sourceEntity->GetPhysics()->GetOrigin()).LengthFast();
		
#ifdef __linux__
		// Import std::min for GCC, since global min() function is non-standard
		using std::min;
#endif
		
		float base = 1 - min(stim->m_Radius, distance) / stim->m_Radius;
		
		// Calculate the falloff value (the magnitude is between [0, magnitude] for positive falloff exponents)
		magnitude *= pow(base, stim->m_FallOffExponent);
	}
	
	//DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Available Response Effects: %u\r", m_ResponseEffects.Num());
	if (m_NumRandomEffects > 0) {
		// Random effect mode, choose exactly m_NumRandomEffects to fire
		for (int i = 1; i <= m_NumRandomEffects; i++) {
			// Get a random effectIndex in the range of [0, m_ResponseEffects.Num()[
			int effectIndex = gameLocal.random.RandomInt(m_ResponseEffects.Num());
			m_ResponseEffects[effectIndex]->runScript(m_Owner.GetEntity(), sourceEntity, magnitude);
		}
	}
	else {
		DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Iterating through ResponseEffects: %d\r", m_ResponseEffects.Num());
		// "Normal" mode, all the effects get fired in order
		for (int i = 0; i < m_ResponseEffects.Num(); i++) {
			m_ResponseEffects[i]->runScript(m_Owner.GetEntity(), sourceEntity, magnitude);
		}
	}
}

void CResponse::SetResponseAction(idStr const &action)
{
	m_ScriptFunction = action;
}

CResponseEffect* CResponse::addResponseEffect(const idStr& effectEntityDef, 
											  const idStr& effectPostfix, 
											  const idDict *args)
{
	CResponseEffect* returnValue = NULL;
	
	DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Seeking EffectEntity [%s]\r", effectEntityDef.c_str());
	// Try to locate the specified entity definition
	const idDict* dict = gameLocal.FindEntityDefDict(effectEntityDef.c_str());

	if (effectEntityDef == "effect_script")
	{
		// We have a script effect, this is a special case
		idStr key;
		sprintf(key, "sr_effect_%s_arg1", effectPostfix.c_str());
		
		// Get the script argument from the entity's spawnargs
		idStr scriptStr = args->GetString(key);

		if (scriptStr != "")
		{
			bool isLocalScript = true;

			const function_t* scriptFunc = m_Owner.GetEntity()->scriptObject.GetFunction(scriptStr.c_str());
			if (scriptFunc == NULL)
			{
				DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("CResponse: %s not found in local space, checking for global.\r", scriptStr.c_str());
				scriptFunc = gameLocal.program.FindFunction(scriptStr.c_str());
				isLocalScript = false;
			}

			if (scriptFunc != NULL)
			{
				DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("CResponse: %s SCRIPTFUNC.\r", scriptFunc->Name());
				// Allocate a new effect object
				CResponseEffect* newEffect = new CResponseEffect(scriptFunc, effectPostfix, scriptStr, isLocalScript);
				// Add the item to the list
				m_ResponseEffects.Append(newEffect);

				returnValue = newEffect;
			}
			else
			{
				gameLocal.Printf("Warning: Script not found: %s!\n", scriptStr.c_str());
			}
		}
		else {
			gameLocal.Printf("Warning: Script argument not found!\n");
		}
	}
	else if (dict != NULL)
	{
		idStr scriptStr = dict->GetString("script");

		const function_t* scriptFunc = gameLocal.program.FindFunction(scriptStr.c_str());
		if (scriptFunc != NULL)
		{
			// Allocate a new effect object
			CResponseEffect* newEffect = new CResponseEffect(scriptFunc, effectPostfix, scriptStr, false);
			
			// Add the item to the list
			m_ResponseEffects.Append(newEffect);

			returnValue = newEffect;
		}
		else {
			DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Warning: Script Function not found: [%s]\r", scriptStr.c_str());
		}
	}
	else
	{
		// Entity not found, emit a warning
		gameLocal.Printf("Warning: EffectEntityDef not found: %s.\r", effectEntityDef.c_str());
		DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Warning: EffectEntityDef not found: %s\r", effectEntityDef.c_str());
	}

	//DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Items in the list: %u\r", m_ResponseEffects.Num());
	
	return returnValue;
}
