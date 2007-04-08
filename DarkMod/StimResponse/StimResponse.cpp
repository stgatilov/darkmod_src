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

static bool init_version = FileVersionList("$Id: StimResponse.cpp 870 2007-03-27 14:21:59Z greebo $", init_version);

#include "StimResponse.h"

// If default stims are to be added, this array also must be
// updated. USER and UNDEFINED are not to be added though, as
// they have special meanings. This array allows us to reuse
// the name in the key in the entity definition for the 
// predefined stim/responses instead of their numerical values.
char *cStimType[] = {
	"STIM_FROB",
	"STIM_FIRE",
	"STIM_WATER",
	"STIM_DAMAGE",
	"STIM_SHIELD",
	"STIM_HEALING",
	"STIM_HOLY",
	"STIM_MAGIC",
	"STIM_TOUCH",
	"STIM_KNOCKOUT",
	"STIM_KILL",
	"STIM_RESTORE",
	"STIM_LIGHT",
	"STIM_SOUND",
	"STIM_VISUAL",
	"STIM_INVITE",
	"STIM_READ",
	"STIM_RANDOM",
	"STIM_TIMER",
	"STIM_COMMUNICATION",
	"STIM_GAS",
	"STIM_TRIGGER",
	"STIM_TARGET_REACHED",
	NULL
};

/********************************************************************/
/*                    CStimResponse                                 */
/********************************************************************/
CStimResponse::CStimResponse(idEntity *Owner, int Type)
{
	m_StimTypeId = Type;
	m_Owner = Owner;
	m_State = SS_DISABLED;
	m_Removable = true;
	m_Default = false;
	m_Duration = 0;
	m_EnabledTimeStamp = 0;
}

CStimResponse::~CStimResponse(void)
{
}

void CStimResponse::EnableSR(bool bEnable)
{
	if(bEnable == true)
	{
		m_State = SS_ENABLED;
		m_EnabledTimeStamp = gameLocal.time;
	}
	else
		m_State = SS_DISABLED;
}
