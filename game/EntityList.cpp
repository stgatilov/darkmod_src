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
#include "precompiled.h"

#include "EntityList.h"
#include "Entity.h"

idEntityList::idEntityList(int idEntity::*idxMember) : idxMember(idxMember) {}
idEntityList::~idEntityList() {
	Clear();
}

void idEntityList::Clear() {
	for (int i = 0; i < order.Num(); i++)
		if (order[i])
			order[i]->*idxMember = -1;
	order.Clear();
}

void idEntityList::AddToEnd(idEntity *ent) {
	assert(!order.Find(ent));
	int idx = order.Append(ent);
	ent->*idxMember = idx;
}

bool idEntityList::Remove(idEntity *ent) {
	int idx = ent->*idxMember;
	if (idx < 0) {
		assert(!order.Find(ent));
		return false;
	}
	assert(order[idx] == ent);
	order[idx] = nullptr;
	ent->*idxMember = -1;
	return true;
}

const idList<idEntity*> &idEntityList::ToList() {
	int k = 0;
	for (int i = 0; i < order.Num(); i++)
		if (order[i])
			order[k++] = order[i];
	order.SetNum(k, false);

	for (int i = 0; i < order.Num(); i++)
		order[i]->*idxMember = i;

	return order;
}
void idEntityList::FromList(idList<idEntity*> &arr) {
	Clear();
	order.Swap(arr);
	for (int i = 0; i < order.Num(); i++)
		order[i]->*idxMember = i;
}
