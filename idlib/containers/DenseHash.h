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

#ifndef __DENSEHASH_H__
#define __DENSEHASH_H__


// This hash table works similarly to google dense map.
// Only POD elements work well within it.
// Should be used for large tables.

template<class Key, class Value, class HashFunction>
class idDenseHash {
public:
	struct Elem {
		Key key;
		Value value;
	};

	idDenseHash() : size1(-1), count(0) {}
	//note: must be called before any operation!
	void Init(Key _empty, int loadfactor = 0) {
		if (loadfactor <= 0 || loadfactor > 75)
			loadfactor = 75;
		maxLoadPercent = loadfactor;
		empty = _empty;
		size1 = 3;
		table.reset(new Elem[size1 + 1]);
		for (int i = 0; i <= size1; i++)
			table[i].key = empty;
	}
	void Reset() {
		size1 = -1;
		count = 0;
		table.reset();
	}

	//returns reference to the table cell with given key
	//if the key is empty (you can check it with IsEmpty), then key is absent in the table
	//in such case you can put your key/value to this cell to add it
	Elem &Find(const Key &key) {
		int hash = hashFunc(key);
		for (int curr = hash & size1; ; curr = (curr + 1) & size1)
			if (table[curr].key == key || table[curr].key == empty)
				return table[curr];
	}

	//checks whether specified cell is empty
	bool IsEmpty(const Elem &elem) const {
		return elem.key == empty;
	}

	//must be called immediately after you remove an element (found by Find)
	//it shifts elements back through the table (i.e. backshift paradigm instead of tombstones)
	void Erase(Elem &elem) {
		int last = &elem - table.get();
		for (int curr = (last + 1) & size1; ; curr = (curr + 1) & size1) {
			if (table[curr].key == empty)
				break;
			int wanted = (hashFunc(table[curr].key) & size1);
			unsigned offset = unsigned(wanted - last - 1) & size1;
			unsigned passed = unsigned(curr - last) & size1;
			if (offset < passed)
				continue;
			table[last] = table[curr];
			last = curr;
		}
		table[last].key = empty;
		count--;
	}

	//must be called immediately after you add an element (to empty cell returned by Find)
	//it checks load factor and may resize the whole table
	void Added(Elem &elem) {
		assert(elem.key != empty);
		count++;
		if (count * 100 > Size() * maxLoadPercent)
			Reallocate();
	}

	int Count() const { return count; }
	int Size() const { return size1 + 1; }
	bool IsDead() const { return size1 == -1; }

private:
	void Reallocate() {
		int oldSize1 = size1 * 2 + 1;
		std::unique_ptr<Elem[]> oldTable(new Elem[oldSize1 + 1]);
		for (int i = 0; i <= oldSize1; i++) oldTable[i].key = empty;
		int oldCount = 0;

		std::swap(table, oldTable);
		std::swap(size1, oldSize1);
		std::swap(count, oldCount);

		for (int i = 0; i <= oldSize1; i++) if (oldTable[i].key != empty) {
			Elem &place = Find(oldTable[i].key);
			place = oldTable[i];
			Added(place);
			assert(count <= oldCount);
		}
		assert(count == oldCount);
	}

	int maxLoadPercent;
	int size1;	//size - 1;
	int count;
	Key empty;
	HashFunction hashFunc;
	std::unique_ptr<Elem[]> table;
};

#endif
