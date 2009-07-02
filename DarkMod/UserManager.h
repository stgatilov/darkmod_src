/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 2501 $
 * $Date: 2008-06-15 13:32:37 +0200 (So, 15 Jun 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef USER_MANAGER_H
#define USER_MANAGER_H

class UserManager
{
	
public:

	int GetNumUsers();

	// Adds user to the list, sorted by alert level
	void AddUser(idActor*);

	void RemoveUser(idActor*);

	idActor* GetMasterUser();

	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);

private:

	idList< idEntityPtr<idActor> >			m_users;



};

#endif /* USER_MANAGER_H */
