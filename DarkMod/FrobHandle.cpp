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

#include "../game/game_local.h"
#include "DarkModGlobals.h"
#include "BinaryFrobMover.h"
#include "FrobHandle.h"

//===============================================================================
// CFrobHandle
//===============================================================================
const idEventDef EV_TDM_Handle_Tap( "Tap", NULL );

CLASS_DECLARATION( CBinaryFrobMover, CFrobHandle )
	EVENT( EV_TDM_Handle_Tap,	CFrobHandle::Event_Tap )
END_CLASS

CFrobHandle::CFrobHandle() :
	m_FrobMaster(NULL),
	m_IsMasterHandle(true),
	m_FrobLock(false)
{}

void CFrobHandle::Save(idSaveGame *savefile) const
{
	savefile->WriteObject(m_FrobMaster);
	savefile->WriteBool(m_IsMasterHandle);
	savefile->WriteBool(m_FrobLock);
}

void CFrobHandle::Restore( idRestoreGame *savefile )
{
	savefile->ReadObject(reinterpret_cast<idClass*&>(m_FrobMaster));
	savefile->ReadBool(m_IsMasterHandle);
	savefile->ReadBool(m_FrobLock);
}

void CFrobHandle::Spawn()
{
	// Dorhandles are always non-interruptable
	m_bInterruptable = false;

	// greebo: The handle itself must never locked, otherwise it can't move in Tap()
	m_Lock.SetLocked(false);
}

void CFrobHandle::Event_Tap()
{
	Tap();
}

void CFrobHandle::SetFrobbed(bool val)
{
	if (m_FrobLock == false)		// Prevent an infinte loop here.
	{
		m_FrobLock = true;

		idEntity::SetFrobbed(val);

		if (m_FrobMaster != NULL)
		{
			m_FrobMaster->SetFrobbed(val);
		}

		m_FrobLock = false;
	}

	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("CFrobHandle [%s] %08lX is frobbed\r", name.c_str(), this);
}

bool CFrobHandle::IsFrobbed()
{
	return (m_FrobMaster != NULL) ? m_FrobMaster->IsFrobbed() : idEntity::IsFrobbed();
}

void CFrobHandle::SetFrobMaster(idEntity* frobMaster)
{
	m_FrobMaster = frobMaster;
}

idEntity* CFrobHandle::GetFrobMaster()
{
	return m_FrobMaster;
}

bool CFrobHandle::CanBeUsedBy(const CInventoryItemPtr& item, bool isFrobUse)
{
	// Pass the call to the master, if we have one, otherwise let the base class handle it
	return (m_FrobMaster != NULL) ? m_FrobMaster->CanBeUsedBy(item, isFrobUse) : idEntity::CanBeUsedBy(item, isFrobUse);
}

bool CFrobHandle::UseBy(EImpulseState impulseState, const CInventoryItemPtr& item)
{
	// Pass the call to the master, if we have one, otherwise let the base class handle it
	return (m_FrobMaster != NULL) ? m_FrobMaster->UseBy(impulseState, item) : idEntity::UseBy(impulseState, item);
}

void CFrobHandle::AttackAction(idPlayer* player)
{
	if (m_FrobMaster != NULL)
	{
		m_FrobMaster->AttackAction(player);
	}
}

void CFrobHandle::FrobAction(bool bMaster)
{
	if (m_FrobMaster != NULL)
	{
		m_FrobMaster->FrobAction(bMaster);
	}
}

void CFrobHandle::ToggleLock() 
{}

bool CFrobHandle::IsMasterHandle()
{
	return m_IsMasterHandle;
}

void CFrobHandle::SetMasterHandle(bool isMaster)
{
	m_IsMasterHandle = isMaster;
}

void CFrobHandle::Tap()
{
	// Default action: Trigger the handle movement
	ToggleOpen();
}
