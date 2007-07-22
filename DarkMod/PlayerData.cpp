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

#include "../DarkMod/DarkModGlobals.h"
#include "../DarkMod/PlayerData.h"

CDarkModPlayer::CDarkModPlayer(void)
{
	m_FrobEntity = NULL;
	m_FrobJoint = INVALID_JOINT;
	m_FrobID = 0;
	m_FrobEntityPrevious = NULL;
	m_LightgemValue = 0;

	// greebo: Initialise the frob trace contact material to avoid 
	// crashing during map save when nothing has been frobbed yet
	m_FrobTrace.c.material = NULL;

	// TODO: Spawn grabber from a .def file (maybe?)
	this->grabber = new CGrabber();
}

CDarkModPlayer::~CDarkModPlayer(void)
{
	// remove grabber object	
	this->grabber->PostEventSec( &EV_Remove, 0 );
}

void CDarkModPlayer::Save( idSaveGame *savefile ) const
{
	grabber->Save(savefile);

	// idEntity		*m_FrobEntity;
	savefile->WriteJoint(m_FrobJoint);
	savefile->WriteInt(m_FrobID);
	savefile->WriteTrace(m_FrobTrace);
	//idEntity	*m_FrobEntityPrevious;
	savefile->WriteInt(m_LightgemValue);
	savefile->WriteFloat(m_fColVal);
	//idList<idLight *>			m_LightList;
}

void CDarkModPlayer::Restore( idRestoreGame *savefile )
{
	// TODO

	grabber->Restore(savefile);

	// idEntity		*m_FrobEntity;
	savefile->ReadJoint(m_FrobJoint);
	savefile->ReadInt(m_FrobID);
	savefile->ReadTrace(m_FrobTrace);
	//idEntity	*m_FrobEntityPrevious;
	savefile->ReadInt(m_LightgemValue);
	savefile->ReadFloat(m_fColVal);
	//idList<idLight *>			m_LightList;
}

unsigned long CDarkModPlayer::AddLight(idLight *light)
{
	if(light)
	{
		m_LightList.Append(light);
		DM_LOG(LC_FUNCTION, LT_DEBUG)LOGSTRING("%08lX [%s] %lu added to LightList\r", light, light->name.c_str(), m_LightList.Num());
	}

	return m_LightList.Num();
}

unsigned long CDarkModPlayer::RemoveLight(idLight *light)
{
	int n;

	if(light)
	{
		if((n = m_LightList.FindIndex(light)) != -1)
		{
			m_LightList.RemoveIndex(n);
			DM_LOG(LC_FUNCTION, LT_DEBUG)LOGSTRING("%08lX [%s] %lu removed from LightList\r", light, light->name.c_str(), m_LightList.Num());
		}
	}

	return m_LightList.Num();
}
