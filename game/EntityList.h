/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
#ifndef __GAME_ENTITY_LIST_H__
#define __GAME_ENTITY_LIST_H__

#include "Entity.h"

// stgatilov: dynamic list of entities
// mainly used for storing the sequence of all "active" entities
class idEntityList {
	// this member of idEntity contains index into currList where it is located
	int idEntity::*idxMember;
	// currently used list of entities, possibly with NULLs
	idList<idEntity*> order;

public:
	idEntityList(int idEntity::*idxMember);
	~idEntityList();

	void Clear();

	struct Iterator {
		int pos;
		idEntity *entity;

		ID_FORCE_INLINE explicit operator bool() const { return entity != nullptr; }
	};
	ID_FORCE_INLINE Iterator Begin() const {
		Iterator iter = {-1, NULL};
		Next(iter);
		return iter;
	}
	ID_FORCE_INLINE void Next(Iterator &iter) const {
		while (1) {
			++iter.pos;
			if (iter.pos >= order.Num()) {
				iter.entity = nullptr;
				break;
			}
			if (idEntity* ent = order[iter.pos]) {
				iter.entity = ent;
				break;
			}
		}
	}

	void AddToEnd(idEntity *ent);
	bool Remove(idEntity *ent);

	// warning: modifies internal members, don't use while iterating!
	const idList<idEntity*> &ToList();
	void FromList(idList<idEntity*> &arr);
};

#endif
