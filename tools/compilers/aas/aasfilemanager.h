/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.1  2004/10/30 15:52:37  sparhawk
 * Initial revision
 *
 ***************************************************************************/

// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __AASFILEMANAGER_H__
#define __AASFILEMANAGER_H__

/*
===============================================================================

	AAS File Manager

===============================================================================
*/

class idAASFileManager {
public:
	virtual						~idAASFileManager( void ) {}

	virtual idAASFile *			LoadAAS( const char *fileName, unsigned int mapFileCRC ) = 0;
	virtual void				FreeAAS( idAASFile *file ) = 0;
};

extern idAASFileManager *		AASFileManager;

#endif /* !__AASFILEMANAGER_H__ */
