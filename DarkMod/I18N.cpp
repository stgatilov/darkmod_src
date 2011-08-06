// vim:ts=4:sw=4:cindent
/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 4831 $
 * $Date: 2011-05-02 19:22:30 +0200 (Mon, 02 May 2011) $
 * $Author: tels $
 *
 ***************************************************************************/

/*
===============================================================================

  I18N (Internationalization) - manages translations of strings, including FM-
  specific translations and secondary dictionaries.

  This class is a singleton and initiated/destroyed from gameLocal.

===============================================================================
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: I18N.cpp 4831 2011-08-04 20:22:30Z tels $", init_version);

#include "I18N.h"

// uncomment to have debug printouts
//#define M_DEBUG 1
// uncomment to have each Translate() call printed
//#define T_DEBUG 1

/*
===============
CI18N::CI18N
===============
*/
CI18N::CI18N ( void ) {
	// some default values, the object becomes only fully usable after Init(), tho:
	m_SystemDict = common->GetLanguageDict();
	m_lang = cvarSystem->GetCVarString( "tdm_lang" );

	// build the reverse dictionary for TemplateFromEnglish
	// TODO: Do this by looking them up in lang/english.lang?
	// inventory categories
	m_ReverseDict.Set( "Lockpicks",	"#str_02389" );
	m_ReverseDict.Set( "Maps", 		"#str_02390" );
	m_ReverseDict.Set( "Readables",	"#str_02391" );
	m_ReverseDict.Set( "Keys",		"#str_02392" );
	m_ReverseDict.Set( "Potions",	"#str_02393" );

	// The article prefixes, with the suffix to use instead
	m_ArticlesDict.Set( "A ",	", A" );	// English, Portuguese
	m_ArticlesDict.Set( "An ",	", An" );
	m_ArticlesDict.Set( "Der ",	", Der" );	// German
	m_ArticlesDict.Set( "Die ",	", Die" );	// German
	m_ArticlesDict.Set( "Das ",	", Das" );	// German
	m_ArticlesDict.Set( "De ",	", De" );	// Dutch, Danish
	m_ArticlesDict.Set( "El ",	", El" );	// Spanish
	m_ArticlesDict.Set( "Het ",	", Het" );	// Dutch
	m_ArticlesDict.Set( "Il ",	", Il" );	// Italian
	m_ArticlesDict.Set( "La ",	", La" );	// French, Italian
	m_ArticlesDict.Set( "Las ",	", Las" );	// Spanish
	m_ArticlesDict.Set( "Le ",	", Le" );	// French
	m_ArticlesDict.Set( "Les ",	", Les" );	// French
	m_ArticlesDict.Set( "Los ",	", Los" );	// Spanish
	m_ArticlesDict.Set( "Os ",	", Os" );	// Portuguese
	m_ArticlesDict.Set( "The ",	", The" );	// English
}

CI18N::~CI18N()
{
	Print();
	Shutdown();
}

/*
===============
CI18N::Init
===============
*/
void CI18N::Init ( void ) {
	// force a reloading (to force other languages in D3), so the GUI looks correct
	SetLanguage( cvarSystem->GetCVarString( "tdm_lang" ) );
}

/*
===============
CI18N::Save
===============
*/
void CI18N::Save( idSaveGame *savefile ) const {
	//savefile->WriteStr(m_lang);
}

/*
===============
CI18N::Restore
===============
*/
void CI18N::Restore( idRestoreGame *savefile ) {
#ifdef M_DEBUG
	idLib::common->Printf( "I18N::Restore()\n" );
#endif
	// We do nothing here, as the dictionary should not change when
	// you load a save game.
}

/*
===============
CI18N::Clear
===============
*/
void CI18N::Clear( void ) {
#ifdef M_DEBUG
	idLib::common->Printf( "I18N::Clear()\n" );
#endif
	// Do not clear the dictionary, even tho we are outside a map,
	// because it might be used for changing menu or HUD strings:
}

/*
===============
CI18N::Shutdown
===============
*/
void CI18N::Shutdown( void ) {
	idLib::common->Printf( "I18N: Shutdown.\n" );
	m_lang = "";
	m_ReverseDict.Clear();
	m_ArticlesDict.Clear();
	m_SystemDict = NULL;
}

/*
===============
CI18N::Print
===============
*/
void CI18N::Print( void ) const {
	idLib::common->Printf("I18N: Current language: %s\n", m_lang.c_str() );
	if (m_SystemDict)
	{
		idLib::common->Printf(" System " );
  		m_SystemDict->Print();
	}
	idLib::common->Printf(" Reverse dict     : " );
	m_ReverseDict.PrintMemory();
	idLib::common->Printf(" Articles dict    : " );
	m_ArticlesDict.PrintMemory();
}

/*
===============
CI18N::Translate
===============
*/
const char* CI18N::Translate( const char* in ) {

#ifdef T_DEBUG
	idLib::common->Printf("I18N: Translating '%s'.\n", in == NULL ? "(NULL)" : in);
#endif
	if (m_SystemDict)
	{
		return m_SystemDict->GetString( in );		// if not found here, do warn
	}
	return in;
}

/*
===============
CI18N::Translate
===============
*/
const char* CI18N::Translate( const idStr &in ) {

	return Translate( in.c_str() );
}

/*
===============
CI18N::TemplateFromEnglish
===============
*/
const char* CI18N::TemplateFromEnglish( const char* in ) {
#ifdef M_DEBUG
//	idLib::common->Printf( "I18N::TemplateFromEnglish(%s)", in );
#endif
	return m_ReverseDict.GetString( in, in );
}

/*
===============
CI18N::GetCurrentLanguage
===============
*/
const idStr* CI18N::GetCurrentLanguage( void ) const {
	return &m_lang;
}

/*
===============
CI18N::SetLanguage

Change the language. Does not check the language here, as to not restrict
ourselves to a limited support of languages.
===============
*/
void CI18N::SetLanguage( const char* lang ) {
	if (lang == NULL)
	{
		return;
	}
#ifdef M_DEBUG
	idLib::common->Printf("I18N: SetLanguage: '%s'.\n", lang);
#endif

	// store the new setting
	m_lang = lang;

	m_SystemDict = common->GetLanguageDict();

	// set sysvar tdm_lang
	cv_tdm_lang.SetString( lang );

	// For some reason, "english", "german", "french" and "spanish" share
	// the same font, but "polish" and "russian" get their own font. But
	// since "polish" is actually a copy of the normal western font, use
	// "english" instead to trick D3 into loading the correct font. The
	// dictionary below will be polish, regardless.
	idStr newLang = idStr(lang);
	if (newLang == "polish")
	{
		newLang = "english";
	}
	// set sysvar sys_lang (if not possible, D3 will revert to english)
	cvarSystem->SetCVarString( "sys_lang", newLang.c_str() );

	// If sys_lang differs from lang, the language was not supported, so
	// we will load it ourselves.
	if ( newLang != cvarSystem->GetCVarString( "sys_lang" ) )
	{
		idLib::common->Printf("I18N: Language '%s' not supported by D3, forcing it.\n", lang);
	}

	// to get around the const preventing changing the system dictionary
	idLangDict *m_forcedDict = const_cast<idLangDict*> (common->GetLanguageDict());

	if (m_forcedDict != NULL)
	{
		// Always forcefully reload the language
		idStr file = "strings/"; file += m_lang + ".lang";
		if ( fileSystem->FindFile( file ) != FIND_NO )
		{
			// can load the language (we expect this, actually), so do sneakily force reload it
			m_forcedDict->Load( file, true );
		}
		else
		{
			idLib::common->Printf("I18N: '%s' not found.\n", file.c_str() );
		}

		idLangDict *FMDict = new idLangDict;
		file = "strings/fm/"; file += m_lang + ".lang";
		if ( !FMDict->Load( file, false ) )
		{
			idLib::common->Printf("I18N: '%s' not found.\n", file.c_str() );
		}
		else
		{
			// else fold the newly loaded strings into the system dict
			int num = FMDict->GetNumKeyVals( );
			const idLangKeyValue*  kv;
			for (int i = 0; i < num; i++)
			{	
				kv = FMDict->GetKeyVal( i );
				if (kv != NULL)
				{
#ifdef M_DEBUG
					idLib::common->Printf("I18N: Folding '%s' ('%s') into main dictionary.\n", kv->key.c_str(), kv->value.c_str() );
#endif
					m_forcedDict->AddKeyVal( kv->key.c_str(), kv->value.c_str() );
				}
			}
		}

		// With FM strings it can happen that one translation is missing or incomplete,
		// so fall back to the english version by folding these in, too:

		file = "strings/fm/english.lang";
		if ( !FMDict->Load( file, true ) )
		{
			idLib::common->Printf("I18N: '%s' not found, skipping it.\n", file.c_str() );
		}
		else
		{
			// else fold the newly loaded strings into the system dict unless they exist already
			int num = FMDict->GetNumKeyVals( );
			const idLangKeyValue*  kv;
			for (int i = 0; i < num; i++)
			{	
				kv = FMDict->GetKeyVal( i );
				if (kv != NULL)
				{
					const char *oldEntry = m_forcedDict->GetString( kv->key.c_str(), false);
					// if equal, the entry was not found
					if (oldEntry == kv->key.c_str())
					{
#ifdef M_DEBUG
						idLib::common->Printf("I18N: Folding '%s' ('%s') into main dictionary as fallback.\n", kv->key.c_str(), kv->value.c_str() );
#endif
						m_forcedDict->AddKeyVal( kv->key.c_str(), kv->value.c_str() );
					}
				}
			}
		}
	}
	else
	{
		idLib::common->Printf("I18N: System dictionary is NULL!\n" );
	}

	// finally reload the GUI so it appears in the new language
	uiManager->Reload( true );		// true => reload all

	// TODO: switch to the Video Settings page, so the user is not confused
	// gui::settingspage" SETTINGS_PAGE_VIDEO
	// resetTime "SettingsPageSelect" 0;
}

/*
===============
CI18N::MoveArticlesToBack

Changes "A little House" to "Little House, A", supporting multiple languages
like English, German, French etc.
===============
*/
void CI18N::MoveArticlesToBack(idStr& title)
{
	// find index of first " "
	int spaceIdx = title.Find(' ');
	// no space, nothing to do
	if (spaceIdx == -1)
	{
		return;
	}

	idStr Prefix = title.Left( spaceIdx + 1 );

	// see if we have Prefix in the dictionary
	const char* suffix = m_ArticlesDict.GetString( Prefix.c_str(), NULL );
	if (suffix != NULL)
	{
		// found, remove prefix and append suffix
		title.StripLeadingOnce( Prefix.c_str() );
		title += suffix;
	}
}

