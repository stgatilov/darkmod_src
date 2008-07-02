/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 2501 $
 * $Date: 2008-06-15 13:32:37 +0200 (So, 15 Jun 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop


#include "UserManager.h"

int UserManager::GetNumUsers()
{
	return m_users.Num();
}

void UserManager::AddUser(idActor* actor)
{
	idEntityPtr<idActor> actorPtr;
	actorPtr = actor;

	m_users.AddUnique(actorPtr);
}

void UserManager::RemoveUser(idActor* actor)
{
	idEntityPtr<idActor> actorPtr;
	actorPtr = actor;

	int index = m_users.FindIndex(actorPtr);
	if (index >= 0)
	{
		m_users.RemoveIndex(index);
	}
}

idActor* UserManager::GetMasterUser()
{
	if (GetNumUsers() > 0)
	{
		return m_users[0].GetEntity();
	}
	return NULL;
}

void UserManager::Save(idSaveGame* savefile) const
{
	int num = m_users.Num();
	savefile->WriteInt(num);
	for (int i = 0; i < num; i++)
	{
		m_users[i].Save(savefile);
	}

}

void UserManager::Restore(idRestoreGame* savefile)
{
	int num;
	savefile->ReadInt(num);
	for (int i = 0; i < num; i++)
	{
		idEntityPtr<idActor> actor;
		actor.Restore(savefile);
		m_users.AddUnique(actor);
	}
}

