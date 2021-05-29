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

#ifndef __DARKMOD_DECLTDM_MATINFO_H__
#define __DARKMOD_DECLTDM_MATINFO_H__

/// A TDM-specific material information declaration class.
/** Currently, the only info it contains is surfacetype.
 */
class tdmDeclTDM_MatInfo : public idDecl
{
public:
	tdmDeclTDM_MatInfo();
	~tdmDeclTDM_MatInfo();

	virtual size_t			Size( void ) const;
	virtual const char *	DefaultDefinition( void ) const;
	virtual void			FreeData( void );
	virtual bool			Parse( const char *text, const int textLength );

	/// Used to cache the TDM_MatInfos for all the materials applied to surfaces of a map.
	static void precacheMap( idMapFile *map );
	static void precacheModel( idRenderModel *model );

	/// The surface type of the material.
	idStr	surfaceType;
};

#endif /* __DARKMOD_DECLTDM_MATINFO_H__ */
