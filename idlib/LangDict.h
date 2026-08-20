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
	bool					LoadLegacy( const char *fileName, const idStr &remap );
	void					Merge( const idLangDict &dict, bool override );

	const char *			GetString( const char *str, const bool dowarn = true ) const;

private:
	idList<idLangKeyValue>	args;
	idHashIndex				hash;

	int						GetHashKey( const char *str ) const;
};

#endif /* !__LANGDICT_H__ */
