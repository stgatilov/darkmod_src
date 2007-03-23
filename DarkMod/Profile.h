/******************************************************************************/
/*                                                                            */
/*               Profile (C) by Gerhard W. Gruber in Vienna 2003              */
/*                          All rights reserved                               */
/*                                                                            */
/******************************************************************************/

/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/*	This file provides a module to read and write profile entries similar to 
	Windows 3.x INI files. It is written in plain C, so it is possible to
	use it in environments where C++ is not usable (like Linux kernel code).

	The structure of such a file is:
	# comment
	[SectionName]
	key = value		# comment
	key = value
	...

	# comment
	[AnotherSection]	# comment
	key = value
	...

	...
*/

#ifndef _PROFILE_H
#define _PROFILE_H

#include "Misc.h"

typedef struct {
	char *Key;
	char *Value;
} PROFILE_MAP;

typedef struct {
	char *SectionName;
	PROFILE_MAP **MapEntry;
	ULONG MapEntries;	// size of array (not neccessarily the number of entries in the array)
} PROFILE_SECTION;

typedef struct {
	FILE *FileHandle;
	char *Path;
	ULONG BufferSize;
	PROFILE_SECTION **Section;	// size of array (not neccessarily the number of entries in the array)
	ULONG Sections;
	BOOL Case;			// TRUE if sectionhandling is casesensitive
} PROFILE_HANDLE;

/*
#ifdef __cplusplus
extern "C" {
#endif
*/

/**
 * Adds an new entries to an array of pointer.
 * The array is reallocated if not enough space is available.
 *
 * @param Entries [IN] Number of entries the array can hold.
 * @param New [IN] Number of entries the array should be extended
 * @param ArrayPtr [IN/OUT] Pointer to a pointerarry.
 *
 * @result Number of entries
 */
ULONG AddArrayEntry(ULONG *Entries, ULONG New, void **ArrayPtr[]);

/**
 * CompressArray will compress a pointerarray. This means that it moves down all pointer
 * which are not NULL so that there is a contigous array of pointers without NULLs
 * in between. The memory is only reallocated if ReAlloc is set to TRUE. In this case the new
 * array will have the same size as all pointers. The assumption is that all entries, which
 * are not in use, will be set to NULL,.
 */
void **CompressArray(void **Array, ULONG *Entries, BOOL ReAlloc);


/*****  Internal functions   *****/

/**
 * ParseFileBuffer reads the buffer which contains a profile
 * and constructs the sections in the PROFILE_HANDLE.
 */
BOOL ParseFilebuffer(PROFILE_HANDLE *Handle);

/**
 * CreateHandle will allocate and initialze a new handle structure and is basically
 * the constructor in C++.
 */
PROFILE_HANDLE *CreateHandle(void);
/**
 *  Allocates and copies a string.
 */
char *StrAlloc(char *str);



/**
 *	Opens a profile and initializes a PROFILE_HANDLE structure which is
 *	used in subsequent calls for reading/writing profile entries.
 *	If bCreate is true then the file is created if it doesn't exist.
 *
 * @param Path [in]			- path that points to the profile file.
 * @param bCreate [in]		- true if the file is to be created in case it doesn't exist.
 *							  In this case a non-NULL pointer is returned.
 * @return					- Returns a PROFILE_HANDLE pointer or NULL if not possible.
 *
 */
PROFILE_HANDLE *OpenProfile(char *Path, BOOL CaseSensitive, BOOL bCreate);
/**
 * This function will try to open the file for the profile.
 */
BOOL ProfOpenFile(PROFILE_HANDLE *h, BOOL bCreate);
BOOL ProfCloseFile(PROFILE_HANDLE *h);

/**
 *	Closes a profile and all the memory is destroyed. You may not use any memory
 *	returned by any Profile call after this point.
 *	If bDelete is true then the file is deleted.
 */
void CloseProfile(PROFILE_HANDLE *h);
/**
 * Writes the current profile to the file
 */
BOOL WriteProfile(PROFILE_HANDLE *h);
/**
 * CompressHandle will compress the array for the sections and all its subobjects.
 */
BOOL CompressProfile(PROFILE_HANDLE *, BOOL ReAlloc);
/**
 * DestroyHandle will free all allocated memory and also call deconstructors for
 * member objects.
 */
PROFILE_HANDLE *DestroyHandle(PROFILE_HANDLE *);
/**
 * Adds a section to a given handle structure. The Section is inserted at the 
 * end of the array. The name has to be provided, otherwise the call fails and NULL
 * is returned. If the section already exists, then pointer of the existing section
 * is returned instead. The number of sections are returned.
 * If CaseSensitive is TRUE then Sectionname != sectionname.
 */
ULONG AddSection(PROFILE_HANDLE *, char *SectionName, PROFILE_SECTION **s);
/**
 * RemoveSection will remove the given section. If more than one section exists
 * with the same name then the only the first one is removed. It is assumed that
 * only one section exists with a given name, but this is no requirement. The pointer
 * in the array is set to NULL. If ReAlloc is set to TRUE then the array will be
 * compressed. The number of sections are returned.
 */
ULONG RemoveSection(PROFILE_HANDLE *, char *SectionName, BOOL ReAlloc);
/**
 * Returns the section for a given sectionname.
 * PROFILE_SECTION may be NULL if the pointer to the section is not needed.
 */
ULONG FindSection(PROFILE_HANDLE *, char *SectionName, PROFILE_SECTION **s);
/**
 * Returns a section with the given index or NULL if the indes doesn't exist.
 * This could be either because the entry at that indes is set to NULL, or the
 * indes is bigger then the array. To make sure you get no NULLs because of empty
 * entries, you should use CompressHandle() first.
 */
PROFILE_SECTION *GetSection(ULONG Index);



/**
 * The constructor for a new section object.
 */
PROFILE_SECTION *CreateSection(char *SectionName);
/**
 * Destructor for a section object. Always returns NULL.
 */
PROFILE_SECTION *DestroySection(PROFILE_SECTION *);
/**
 * Renames the section to the new name
 */
BOOL RenameSection(PROFILE_SECTION *s, char *NewName);
/**
 * Adds a mapping object to a section. 
 */
ULONG AddMap(PROFILE_SECTION *Section, PROFILE_MAP *);
/**
 * Create a new map and add it to the section.
 */
ULONG AddMapEntry(PROFILE_SECTION *Section, char *Key, char *Value, BOOL CaseSensitive, PROFILE_MAP **);
/**
 * Removes a map analog to RemoveSection.
 */
ULONG RemoveMap(PROFILE_SECTION *Section, PROFILE_MAP *, BOOL ReAlloc);
/**
 * Searches for a key in a given section.
 * PROFILE_MAP may be NULL if the pointer to the map is not needed.
 */
ULONG FindMap(PROFILE_SECTION *, char *Key, BOOL CaseSensitiv, PROFILE_MAP **Mapptr);





/**
 * Constrcutor for map objects
 */
PROFILE_MAP *CreateMap(char *Key, char *Value);
/**
 * Desctructor for map objects.
 */
PROFILE_MAP *DestroyMap(PROFILE_MAP *);

/**
 * ReadLine liest <Lines> Zeilen aus dem angegebenen File. Die Zeichen
 * werden in <Buffer> abgelegt. Falls in dem File weniger Zeilen
 * vorhanden sind als angegeben wird die Anzahl der bereits gelesenen
 * Zeilen zurueckgeliefert. errno wird durch die Systemfunktionen
 * entsprechend gesetzt und kann dazu benutzt werden festzustellen warum
 * nicht alle Zeilen gelesen werden konnten. Die Zeilen werden durch 0
 * getrennt. Somit kann einfach mit strlen() die Laenge einer Zeile fest-
 * gestellt werden. Wenn das File zu Ende ist (EOF) und bis zum letzten
 * Zeichen kein CR(LF) gefunden wurde so wird diese Zeile trotzdem als
 * Zeile gezaehlt. Falls eine Zeile laenger ist als in MaxLen angegeben,
 * Wird dieser Teil ebenfalls als eine Zeile angesehen und entsprechend
 * gezaehlt. <TabExpand> enthaelt entweder 0, dann werden Tabulatoren
 * als solche uebernommen. Wenn ein Wert ungleich 0 angegeben wird, dann
 * werden fuer jedes gefundene Tabulatorzeichen, entsprechend viele
 * Blanks eingesetzt. Es werden allerdings nur so viele Blanks eingesetzt
 * wie bis zur naechsten Tabstop Position benoetigt werden.
 *
 * INPUT:	stream	InputStream aus dem gelesen werden soll
 *			Buffer	Speicherblock in den eingelesen werden soll
 *			Lines	Anzahl der Zeilen die gelesen werden sollen
 *			MaxLen	Maximale Laenge einer Zeile.
 *
 * OUTPUT:	LONG		Anzahl der gelesenen Zeilen oder -1
 *				errno		Wenn LONG = -1 ist wird errno gesetzt
 *							EBADF Inputstream ist ungueltig
 *							ENOMEM Nicht genug Speicher vorhanden
 *								In diesem Fall eventuell die Zeilenlaenge
 *                              herabsetzen.
 *							errno kann auch andere Werte enthalten die
 *							durch die Systemfunktionen gesetzt werden.
 *
 * ACHTUNG:	Wenn LONG != -1 ist, ist errno undefiniert
 */
LONG ReadLine(FILE *strm, void *Buffer, LONG Lines, LONG MaxLen, WORD TabExpand);
BYTE ToUpper(UBYTE b);
LONG FindChar(UBYTE *s, UBYTE *p, BOOL Case, BOOL Find, UBYTE SkipChar);
UBYTE *Strip(UBYTE *s, int Start);
UBYTE *StrStrip(UBYTE *s, int Start, char *p);

/*
#ifdef __cplusplus
};
#endif
*/

#endif // _PROFILE_H
