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

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);
#pragma warning( push )
#pragma warning( disable: 4245 )
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WINDOWS_
#include <io.h>
#endif

#include <fcntl.h>

#include "Profile.h"
#include "Misc.h"

char *StrAlloc(const char *s)
{
	char *rc = NULL;

	if(s == NULL)
		goto Quit;

	if((rc = (char *)malloc((strlen(s)+1)*sizeof(char))) == NULL)
		goto Quit;

	strcpy(rc, s);

Quit:
	return(rc);
}


ULONG AddArrayEntry(ULONG *Entries, ULONG New, void **p[])
{
	ULONG rc = -1;
	ULONG l;
	void **m;

	if(p == NULL || Entries == NULL)
		goto Quit;

	l = (sizeof(void **)*New);

	m = *p;
	if(m == NULL)
	{
		if((m = (void **)malloc(l)) == NULL)
			goto Quit;
	}
	else
	{
		if((m = (void **)realloc(m, l + (sizeof(void *)*(*Entries)))) == NULL)
			goto Quit;
	}

	memset(&m[*Entries], 0, l);
	rc = *Entries + New;
	*Entries = rc;
	*p = m;

Quit:
	return(rc);
}


/**********************************************/
/* ---===<* Handle related functions *>===--- */
/**********************************************/
PROFILE_HANDLE *CreateHandle(void)
{
	PROFILE_HANDLE *rc;

	if((rc = (PROFILE_HANDLE *)malloc(sizeof(PROFILE_HANDLE))) == NULL)
		goto Quit;

	rc->Path = NULL;
	rc->FileHandle = NULL;
	rc->Section = NULL;
	rc->Sections = 0L;
	rc->Case = FALSE;

Quit:
	return(rc);
}

PROFILE_HANDLE *DestroyHandle(PROFILE_HANDLE *h)
{
	ULONG i, n;

	if(!h)
		goto Quit;

	if(h->Path)
		free(h->Path);

	if(h->FileHandle)
		fclose(h->FileHandle);

	if(h->Section)
	{
		n = h->Sections;

		for(i = 0; i < n; i++)
			DestroySection(h->Section[i]);

		free(h->Section);
	}

	free(h);

Quit:
	return(NULL);
}


PROFILE_HANDLE *OpenProfile(const char *Path, BOOL CaseSensitive, BOOL bCreate)
{
	PROFILE_HANDLE *rc = NULL, *h = NULL;

	if(Path == NULL)
		goto Quit;

	if((h = CreateHandle()) == NULL)
		goto Quit;

	if((h->Path = StrAlloc(Path)) == NULL)
		goto Quit;

	if(ProfOpenFile(h, bCreate) == FALSE)
		goto Quit;

	h->Case = CaseSensitive;
	ParseFilebuffer(h);
	fclose(h->FileHandle);
	h->FileHandle = NULL;

	rc = h;

Quit:
	/* In case something went wrong we destroy the structure. */
	if(h != NULL && rc == NULL)
		rc = DestroyHandle(h);

	return(rc);
}

BOOL ProfOpenFile(PROFILE_HANDLE *h, BOOL bCreate)
{
	BOOL rc = FALSE;

	if(h == NULL)
		goto Quit;

	if((h->FileHandle = fopen(h->Path, "r+b")) == NULL)
	{
		if(bCreate)
		{
			if((h->FileHandle = fopen(h->Path, "w+b")) == NULL)
				goto Quit;
		}
		else
			goto Quit;
	}

	rc = TRUE;

Quit:
	return(rc);
}

BOOL ProfCloseFile(PROFILE_HANDLE *h)
{
	BOOL rc = FALSE;

	if(h == NULL)
		goto Quit;

	if(h->FileHandle != NULL)
		fclose(h->FileHandle);

	h->FileHandle = NULL;
	rc = TRUE;

Quit:
	return(rc);
}

void CloseProfile(PROFILE_HANDLE *h)
{
	DestroyHandle(h);
}

BOOL WriteProfile(PROFILE_HANDLE *h)
{
	BOOL rc = FALSE;
	ULONG ih, nh, is, ns;
	FILE *f;
	PROFILE_SECTION *s;
	PROFILE_MAP *m;
	char str[512];

	if(h == NULL)
		goto Quit;

	if(h->FileHandle == NULL && ProfOpenFile(h, TRUE) == FALSE)
		goto Quit;

	f = h->FileHandle;
	fseek(f, 0, SEEK_SET);
	chsize(fileno(f), 0);

	nh = h->Sections;
	for(ih = 0; ih < nh; ih++)
	{
		// ignore empty entries.
		if((s = h->Section[ih]) == NULL)
			continue;
		
		// Check for bufferoverflow
		if((strlen(s->SectionName)+10) >= sizeof(str))
			goto Quit;

		sprintf(str, "[%s]\n", s->SectionName);
		if(fprintf(f, "%s", str) != (int)strlen(str))
			goto Quit;

		ns = s->MapEntries;
		for(is = 0; is < ns; is++)
		{
			m = s->MapEntry[is];

			// Check for bufferoverflow
			if((strlen(m->Key)+strlen(m->Value)+10) >= sizeof(str))
				goto Quit;

			sprintf(str, "%s=%s\n", m->Key, m->Value);
			if(fprintf(f, "%s", str) != (int)strlen(str))
				goto Quit;
		}
	}

	rc = TRUE;

Quit:
	ProfCloseFile(h);

	return(rc);
}

BOOL CompressProfile(PROFILE_HANDLE *h, BOOL ReAlloc)
{
	BOOL rc = FALSE;

	if(h == NULL)
		goto Quit;

Quit:
	return(rc);
}

ULONG AddSection(PROFILE_HANDLE *h, char *sn, PROFILE_SECTION **ps)
{
	ULONG rc = -1;
	ULONG si;
	PROFILE_SECTION *s, *ds;

	// In case we have a NULL pointer here, we assign a dummypointer
	// so we don't have to check always if it is NULL.
	if(ps == NULL)
		ps = &ds;

	// Check if the section already exists.
	if((si = FindSection(h, sn, &ds)) != -1)
	{
		rc = si;
		*ps = ds;

		goto Quit;
	}

	if((s = CreateSection(sn)) == NULL)
		goto Quit;

	rc = AddArrayEntry(&h->Sections, 1, (void ***)&h->Section);
	h->Section[h->Sections-1] = s;
	*ps = s;

Quit:
	return(rc);
}


ULONG RemoveSection(PROFILE_HANDLE *h, char *sn, BOOL ReAlloc)
{
	ULONG rc = -1;
	ULONG si;
	PROFILE_SECTION *s;

	if((si = FindSection(h, sn, &s)) == -1)
		goto Quit;

	DestroySection(s);
	h->Section[si] = NULL;
	rc = si;

Quit:
	return(rc);
}


/**********************************************/
/* ---===<* Section related functions *>===--- */
/**********************************************/
PROFILE_SECTION *CreateSection(char *SectionName)
{
	PROFILE_SECTION *rc = NULL, *s = NULL;

	if(SectionName == NULL || SectionName[0] == 0)
		goto Quit;

	if((s = (PROFILE_SECTION *)malloc(sizeof(PROFILE_SECTION))) == NULL)
		goto Quit;

	if((s->SectionName = StrAlloc(SectionName)) == NULL)
		goto Quit;

	s->MapEntries = 0;
	s->MapEntry = NULL;

	rc = s;

Quit:
	if(s != NULL && rc == NULL)
		DestroySection(s);

	return(rc);
}

PROFILE_SECTION *DestroySection(PROFILE_SECTION *s)
{
	ULONG i, n;

	if(s == NULL)
		goto Quit;

	if(s->SectionName != NULL)
		free(s->SectionName);

	n = s->MapEntries;
	for(i = 0; i < n; i++)
		DestroyMap(s->MapEntry[i]);

	if(s->MapEntry != NULL)
		free(s->MapEntry);

	free(s);

Quit:
	return(NULL);
}

BOOL RenameSection(PROFILE_SECTION *s, char *sn)
{
	BOOL rc = FALSE;

	if(s == NULL || sn == NULL || sn[0] == 0)
		goto Quit;

	if(strlen(sn) == strlen(s->SectionName))
		strcpy(s->SectionName, sn);
	else
	{
		free(s->SectionName);
		s->SectionName = StrAlloc(sn);
	}

Quit:
	return(rc);
}

ULONG AddMap(PROFILE_SECTION *s, PROFILE_MAP *m)
{
	ULONG rc = -1;

	if(s == NULL || m == NULL)
		goto Quit;

	rc = AddArrayEntry(&s->MapEntries, 1, (void ***)&s->MapEntry);
	s->MapEntry[s->MapEntries-1] = m;

Quit:
	return(rc);
}


ULONG AddMapEntry(PROFILE_SECTION *s, char *k, char *v, BOOL CaseSensitive, PROFILE_MAP **pm)
{
	ULONG rc = -1, mi;
	PROFILE_MAP *m = NULL, *dm;

	if(s == NULL || k == NULL)
		goto Quit;

	// In case we have a NULL pointer we assign our dummy pointer
	// so we dont'have to check always if it is NULL.
	if(pm == NULL)
		pm = &dm;

	*pm = NULL;

	// If the key already exists, we replace the value.
	if((mi = FindMap(s, k, CaseSensitive, &dm)) != -1)
	{
		rc = mi;
		*pm = dm;
		free(dm->Value);
		dm->Value = StrAlloc(v);
		goto Quit;
	}

	if((m = CreateMap(k, v)) == NULL)
		goto Quit;

	if(AddMap(s, m) == -1)
		goto Quit;

	*pm = m;
	m = NULL;

	rc = s->MapEntries;

Quit:
	if(m != NULL)
		DestroyMap(m);

	return(rc);
}

ULONG FindSection(PROFILE_HANDLE *h, const char *sn, PROFILE_SECTION **ps)
{
	ULONG rc = -1;
	ULONG i, n, v;
	PROFILE_SECTION *s;

	if(h == NULL || sn == NULL)
		goto Quit;

	n = h->Sections;
	for(i = 0; i < n; i++)
	{
		v = 2;
		s = h->Section[i];
		
		if(s == NULL)
			continue;

		if(h->Case == TRUE)
			v = strcmp(s->SectionName, sn);
		else
			v = stricmp(s->SectionName, sn);

		if(v == 0)
		{
			if(ps != NULL)
				*ps = h->Section[i];

			rc = i;
			break;
		}
	}

Quit:
	return(rc);
}

ULONG FindMap(PROFILE_SECTION *s, const char *k, BOOL CaseSensitive, PROFILE_MAP **m)
{
	ULONG rc = -1;
	ULONG i, n, v;

	if(s == NULL || k == NULL)
		goto Quit;

	n = s->MapEntries;
	for(i = 0; i < n; i++)
	{
		v = 2;
		if(CaseSensitive == TRUE)
			v = strcmp(s->MapEntry[i]->Key, k);
		else
			v = stricmp(s->MapEntry[i]->Key, k);

		if(v == 0)
		{
			if(m != NULL)
				*m = s->MapEntry[i];

			rc = i;
			break;
		}
	}

Quit:
	return(rc);
}


/**********************************************/
/*  ---===<* Map related functions *>===---   */
/**********************************************/
PROFILE_MAP *CreateMap(char *Key, char *Value)
{
	PROFILE_MAP *rc = NULL, *m = NULL;

	if(Key == NULL || Key[0] == 0)
		goto Quit;

	if((m = (PROFILE_MAP *)malloc(sizeof(PROFILE_MAP))) == NULL)
		goto Quit;

	m->Key = NULL;
	m->Value = NULL;

	if((m->Key = StrAlloc(Key)) == NULL)
		goto Quit;

	if(Value != NULL && (m->Value = StrAlloc(Value)) == NULL)
		goto Quit;

	rc = m;

Quit:
	if(m != NULL && rc == NULL)
		DestroyMap(m);

	return(rc);
}


PROFILE_MAP *DestroyMap(PROFILE_MAP *m)
{
	if(m == NULL)
		goto Quit;

	if(m->Key != NULL)
		free(m->Key);

	if(m->Value != NULL)
		free(m->Value);

	free(m);

Quit:
	return(NULL);
}


BOOL ParseFilebuffer(PROFILE_HANDLE *h)
{
	BOOL rc = FALSE;
	FILE *f;
	UBYTE buffer[1024], *str;
	long n, b0, b1, e;
	PROFILE_SECTION *s = NULL;
//	PROFILE_MAP *m = NULL;

	if(h == NULL || h->FileHandle == NULL)
		goto Quit;

	f = h->FileHandle;

	while((n = ReadLine(f, buffer, 1, sizeof(buffer), 8)) != -1)
	{
		// EOF?
		if(n == 0)
			goto Quit;

		str = buffer;

		n = strlen((const char *)str);
		str = Strip((UBYTE *)str, FALSE+TRUE+1);

		// empty line
		if(*str == 0)
			continue;

		b0 = FindChar(str, (UBYTE *)"[", FALSE, TRUE, '\\');
		b1 = FindChar(str, (UBYTE *)"]", FALSE, TRUE, '\\');
		e = FindChar(str, (UBYTE *)"=", FALSE, TRUE, '\\');

		/* Invalid formats will be ignored. */

		/* Brackets are in the wrong order ][ or an '=' is on the first position */
		if((b0 != -1 && b1 != -1 && b0 >= b1) || e == 0)
			continue;

		// if we have an euqal sign in the line, this indicates a
		// key/value pair.
		if(e == -1)
		{
			str = StrStrip(str, FALSE+TRUE+1, "[]");
			s = CreateSection((char *)str);
			AddArrayEntry(&h->Sections, 1, (void ***)&h->Section);
			h->Section[h->Sections-1] = s;
		}
		else
		{
			// Key/value pair outside a section. This should only be 
			// possible at the start of the file. All other pairs would
			// be assigned to the current section, as there is no indicator
			// when a section ends, the section only ends when the next one
			// starts.
			if(s == NULL)
				continue;

			str[e] = 0;
			AddMapEntry(s, (char *)str, (char *)&str[e+1], FALSE, NULL);
		}
	}

	rc = TRUE;

Quit:
	return(rc);
}

#pragma warning( pop )
