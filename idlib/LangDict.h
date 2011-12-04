// vim:ts=4:sw=4:cindent
/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __LANGDICT_H__
#define __LANGDICT_H__

/*
===============================================================================

	Simple dictionary specifically for the localized string tables.

===============================================================================
*/

class idLangKeyValue {
public:
	idStr					key;
	idStr					value;
};

class idLangDict {
public:
							idLangDict( void );
							~idLangDict( void );

	void					Clear( void );
	// Tels: #2812: if remapcount and remap are given, use these to replace characters (remap containts two entries for
	//				each "remapcount", the first is the one to replace, the second the replacement.
	bool					Load( const char *fileName, const bool clear = true, const unsigned int remapcount = 0, const char *remap = NULL );
	void					Save( const char *fileName );

	const char *			AddString( const char *str );
	const char *			GetString( const char *str, const bool dowarn = true ) const;

	/**
	* Tels: Print some statistics about memory usage.
	*/
	void				Print( void ) const;
							// adds the value and key as passed (doesn't generate a "#str_xxxxx" key or ensure the key/value pair is unique)
	void					AddKeyVal( const char *key, const char *val );

	int						GetNumKeyVals( void ) const;
	const idLangKeyValue *	GetKeyVal(const int i) const;

	void					SetBaseID(const int id) { baseID = id; };

private:
	idList<idLangKeyValue>	args;
	idHashIndex				hash;

	bool					ExcludeString( const char *str ) const;
	int						GetNextId( void ) const;
	int						GetHashKey( const char *str ) const;

	int						baseID;
};

#endif /* !__LANGDICT_H__ */
