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
	m_ReverseDict = new idDict;
	m_SystemDict = common->GetLanguageDict();
	m_lang = cvarSystem->GetCVarString( "tdm_lang" );

	// build the reverse dictionary for TemplateFromEnglish

	// TODO: Do this by looking them up in lang/english.lang?
	// inventory categories
	m_ReverseDict->Set( "Lockpicks",	"#str_02389" );
	m_ReverseDict->Set( "Maps", 		"#str_02390" );
	m_ReverseDict->Set( "Readables",	"#str_02391" );
	m_ReverseDict->Set( "Keys",			"#str_02392" );
	m_ReverseDict->Set( "Potions",		"#str_02393" );
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
	if (m_ReverseDict) { delete m_ReverseDict; }
	m_ReverseDict = NULL;
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
	if (m_ReverseDict)
	{
		idLib::common->Printf(" Reverse dict     : " );
		m_ReverseDict->PrintMemory( );
	}
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

	if (in.c_str() == NULL )
	{
		return "";
	}
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
	if (m_ReverseDict)
	{
		return m_ReverseDict->GetString( in, in );
	}
	return in;
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
	// set sysvar sys_lang (if possible)
	cvarSystem->SetCVarString( "sys_lang", lang );

	// If sys_lang differs from lang, the language was not supported, so
	// we will load it ourselves.
	if ( idStr( cvarSystem->GetCVarString( "sys_lang" ) ) != idStr(lang) )
	{
		idLib::common->Printf("I18N: Language '%s' not supported by D3, forcing it.\n", lang);
	}

	// Always forcefully reload the language
	idStr file = "strings/"; file += m_lang + ".lang";
	if ( fileSystem->FindFile( file ) != FIND_NO )
	{
		// can load the language (we expect this, actually), so do it sneakily behind the scenes
		idLangDict *m_forcedDict = const_cast<idLangDict*> (common->GetLanguageDict());
		if (m_forcedDict != NULL)
		{
			// force reload it
			m_forcedDict->Load( file, true );
		}
		else
		{
			idLib::common->Printf("I18N: System dictionary is NULL!\n" );
		}
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
		idLangDict *m_forcedDict = const_cast<idLangDict*> (common->GetLanguageDict());
		if (m_forcedDict != NULL)
		{
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
	}

	// finally reload the GUI so it appears in the new language
	uiManager->Reload( true );		// true => reload all

	// TODO: switch to the Video Settings page, so the user is not confused
	// gui::settingspage" SETTINGS_PAGE_VIDEO
	// resetTime "SettingsPageSelect" 0;
}

