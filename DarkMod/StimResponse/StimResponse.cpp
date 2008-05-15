/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/
#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

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
	"STIM_PLAYER",
	"STIM_FLASH",
	"STIM_BLIND",
	NULL
};

/********************************************************************/
/*                    CStimResponse                                 */
/********************************************************************/
CStimResponse::CStimResponse(idEntity *Owner, int Type, int uniqueId)
{
	m_UniqueId = uniqueId;
	m_StimTypeId = Type;
	m_Owner = Owner;
	m_State = SS_DISABLED;
	m_Removable = true;
	m_Default = false;
	m_Duration = 0;
	m_EnabledTimeStamp = 0;
	m_Chance = 1.0f;
	m_ChanceTimer = -1;
	m_NextChanceTime = -1;
}

CStimResponse::~CStimResponse(void)
{
}

int	CStimResponse::getUniqueId() const
{
	return m_UniqueId;
}

void CStimResponse::Save(idSaveGame *savefile) const
{
	savefile->WriteInt(m_UniqueId);
	savefile->WriteInt(m_StimTypeId);
	savefile->WriteString(m_StimTypeName.c_str());
	savefile->WriteBool(m_Removable);
	savefile->WriteInt(static_cast<int>(m_State));
	savefile->WriteFloat(m_Chance);
	savefile->WriteInt(m_ChanceTimer);
	savefile->WriteInt(m_NextChanceTime);
	savefile->WriteBool(m_Default);
	savefile->WriteInt(m_EnabledTimeStamp);
	savefile->WriteInt(m_Duration);
	m_Owner.Save(savefile);
}

void CStimResponse::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt(m_UniqueId);
	savefile->ReadInt(m_StimTypeId);
	savefile->ReadString(m_StimTypeName);
	savefile->ReadBool(m_Removable);

	int tempInt;
	savefile->ReadInt(tempInt);
	m_State = static_cast<StimState>(tempInt);

	savefile->ReadFloat(m_Chance);
	savefile->ReadInt(m_ChanceTimer);
	savefile->ReadInt(m_NextChanceTime);
	savefile->ReadBool(m_Default);
	savefile->ReadInt(m_EnabledTimeStamp);
	savefile->ReadInt(m_Duration);
	m_Owner.Restore(savefile);
}

StimType CStimResponse::getStimType(const idStr& stimName) {
	int i = 0;
	
	while (cStimType[i] != NULL)	{
		if (stimName == cStimType[i]) {
			return static_cast<StimType>(i);
		}
		i++;
	}

	// Not found
	return ST_DEFAULT;
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

bool CStimResponse::checkChance()
{
	if (m_Chance < 1.0f)
	{
		// Chance timer still active?
		if (m_NextChanceTime > gameLocal.GetTime()) {
			DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("CStimResponse::checkChance: Timeout still active.\r");

			// Next chance time is still in the future, return false
			return false;
		}

		// The stim only fires if the (hopefully uniformly distributed)
		// random variable is within the interval (0, m_Chance]
		if (gameLocal.random.RandomFloat() <= m_Chance) {
			// Reset the next chance time
			m_NextChanceTime = -1;
			return true;
		}
		else {
			DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("CStimResponse::checkChance: Chance test failed.\r");
			// Test failed, should we use a timeout?
			if (m_ChanceTimer > 0) {
				// Save the earliest time of the next response chance
				m_NextChanceTime = gameLocal.GetTime() + m_ChanceTimer;
			}
			return false;
		}
	}
	else {
		// 100% chance => return true
		return true;
	}
}
