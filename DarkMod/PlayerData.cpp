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

	m_FrobEntity.Save(savefile);
	savefile->WriteJoint(m_FrobJoint);
	savefile->WriteInt(m_FrobID);
	savefile->WriteTrace(m_FrobTrace);
	m_FrobEntityPrevious.Save(savefile);
	savefile->WriteInt(m_LightgemValue);
	savefile->WriteFloat(m_fColVal);

	savefile->WriteInt(m_LightList.Num());
	for (int i = 0; i < m_LightList.Num(); i++)
	{
		m_LightList[i].Save(savefile);
	}
}

void CDarkModPlayer::Restore( idRestoreGame *savefile )
{
	grabber->Restore(savefile);

	m_FrobEntity.Restore(savefile);
	savefile->ReadJoint(m_FrobJoint);
	savefile->ReadInt(m_FrobID);
	savefile->ReadTrace(m_FrobTrace);
	m_FrobEntityPrevious.Restore(savefile);
	savefile->ReadInt(m_LightgemValue);
	savefile->ReadFloat(m_fColVal);
	
	int num;
	savefile->ReadInt(num);
	m_LightList.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		m_LightList[i].Restore(savefile);
	}
}

unsigned long CDarkModPlayer::AddLight(idLight *light)
{
	if(light)
	{
		idEntityPtr<idLight> lightPtr;
		lightPtr = light;
		m_LightList.Append(lightPtr);
		DM_LOG(LC_FUNCTION, LT_DEBUG)LOGSTRING("%08lX [%s] %lu added to LightList\r", light, light->name.c_str(), m_LightList.Num());
	}

	return m_LightList.Num();
}

unsigned long CDarkModPlayer::RemoveLight(idLight *light)
{
	if(light)
	{
		for (int i = 0; i < m_LightList.Num(); i++)
		{
			if (m_LightList[i].GetEntity() == light) 
			{
				// Light found, remove it
				m_LightList.RemoveIndex(i);
				DM_LOG(LC_FUNCTION, LT_DEBUG)LOGSTRING("%08lX [%s] %lu removed from LightList\r", light, light->name.c_str(), m_LightList.Num());
				break;
			}
		}
	}

	return m_LightList.Num();
}
