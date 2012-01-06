// vim:ts=4:sw=4:cindent
/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
 $Revision$ (Revision of last commit) 
 $Date$ (Date of last commit)
 $Author$ (Author of last commit)
 
******************************************************************************/

/*
===============================================================================

  I18N (Internationalization) - manages translations of strings, including FM-
  specific translations and secondary dictionaries.

  This class is a singleton and initiated/destroyed from gameLocal.

===============================================================================
*/

#include "precompiled_game.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "I18N.h"
#include "sourcehook/sourcehook.h"

// uncomment to have debug printouts
//#define M_DEBUG 1
// uncomment to have each Translate() call printed
//#define T_DEBUG 1

extern SourceHook::ISourceHook *g_SHPtr;
extern int g_PLID;

// Declare the detour hook: HOOK0: zero parameters, return value const idLangDict*
SH_DECL_HOOK0(idCommon, GetLanguageDict, SH_NOATTRIB, 0, const idLangDict*);

/*
===============
CI18N::CI18N
===============
*/
CI18N::CI18N ( void ) {
	// some default values, the object becomes only fully usable after Init(), tho:
	m_lang = cvarSystem->GetCVarString( "tdm_lang" );
	m_bMoveArticles = (m_lang != "polish" && m_lang != "italian") ? true : false;

	m_Dict.Clear();

	// build the reverse dictionary for TemplateFromEnglish
	// TODO: Do this by looking them up in lang/english.lang?
	// inventory categories
	m_ReverseDict.Set( "Lockpicks",	"#str_02389" );
	m_ReverseDict.Set( "Maps", 		"#str_02390" );
	m_ReverseDict.Set( "Readables",	"#str_02391" );
	m_ReverseDict.Set( "Keys",		"#str_02392" );
	m_ReverseDict.Set( "Potions",	"#str_02393" );

	// inventory item names used in keybindings
	m_ReverseDict.Set( "Mine",			"#str_02202" );
	m_ReverseDict.Set( "Lantern",		"#str_02395" );
	m_ReverseDict.Set( "Spyglass",		"#str_02396" );
	m_ReverseDict.Set( "Compass",		"#str_02397" );
	m_ReverseDict.Set( "Health Potion",	"#str_02398" );
	m_ReverseDict.Set( "Breath Potion",	"#str_02399" );
	m_ReverseDict.Set( "Holy Water",	"#str_02400" );
	m_ReverseDict.Set( "Flashbomb",		"#str_02438" );
	m_ReverseDict.Set( "Flashmine",		"#str_02439" );
	m_ReverseDict.Set( "Explosive Mine","#str_02440" );
	
	// The article prefixes, with the suffix to use instead
	m_ArticlesDict.Set( "A ",	", A" );	// English, Portuguese
	m_ArticlesDict.Set( "An ",	", An" );	// English
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

	m_Remap.Empty();						// by default, no remaps
}

CI18N::~CI18N()
{
	SH_REMOVE_HOOK_MEMFUNC(idCommon, GetLanguageDict, common, this, &CI18N::GetLanguageDict, false);
	Print();
	Shutdown();
}

/*
===============
CI18N::Init
===============
*/
void CI18N::Init ( void ) {
	// Create the correct dictionary
	SetLanguage( cvarSystem->GetCVarString( "tdm_lang" ), true );

	// false => pre hook
	SH_ADD_HOOK_MEMFUNC(idCommon, GetLanguageDict, common, this, &CI18N::GetLanguageDict, false);
}

/*
===============
CI18N::GetLanguageDict

Returns the language dict, so that D3 can use it instead of the system dict. Should
not be called directly.
===============
*/
const idLangDict* CI18N::GetLanguageDict ( void ) const {
	RETURN_META_VALUE(MRES_OVERRIDE, &m_Dict);
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
	common->Printf( "I18N::Restore()\n" );
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
	common->Printf( "I18N::Clear()\n" );
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
	common->Printf( "I18N: Shutdown.\n" );
	m_lang = "";
	m_ReverseDict.Clear();
	m_ArticlesDict.Clear();
	m_Dict.Clear();
}

/*
===============
CI18N::Print
===============
*/
void CI18N::Print( void ) const {
	common->Printf("I18N: Current language: %s\n", m_lang.c_str() );
	common->Printf("I18N: Move articles to back: %s\n", m_bMoveArticles ? "Yes" : "No");
	common->Printf(" Main " );
	m_Dict.Print();
	common->Printf(" Reverse dict   : " );
	m_ReverseDict.PrintMemory();
	common->Printf(" Articles dict  : " );
	m_ArticlesDict.PrintMemory();
	common->Printf(" Remapped chars : %i\n", m_Remap.Length() / 2 );
}

/*
===============
CI18N::Translate
===============
*/
const char* CI18N::Translate( const char* in ) {
#ifdef T_DEBUG
	gameLocal.Printf("I18N: Translating '%s'.\n", in == NULL ? "(NULL)" : in);
#endif
	return m_Dict.GetString( in );				// if not found here, do warn
}

/*
===============
CI18N::Translate
===============
*/
const char* CI18N::Translate( const idStr &in ) {
#ifdef T_DEBUG
	gameLocal.Printf("I18N: Translating '%s'.\n", in == NULL ? "(NULL)" : in.c_str());
#endif
	return m_Dict.GetString( in.c_str() );			// if not found here, do warn
}

/*
===============
CI18N::TemplateFromEnglish

If the string is not a template, but an English string, returns a template
like "#str_01234" from the input. Works only for a limited number of strings
that appear in the reverse dict.
===============
*/
const char* CI18N::TemplateFromEnglish( const char* in ) {
#ifdef M_DEBUG
//	common->Printf( "I18N::TemplateFromEnglish(%s)", in );
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
CI18N::LoadCharacterMapping

Loads the character remap table, defaulting to "default.map" if "LANGUAGE.map"
is not found. This is used to fix bug #2812.
===============
*/
int CI18N::LoadCharacterMapping( idStr& lang ) {

	m_Remap.Empty();		// clear the old mapping

	const char *buffer = NULL;
	idLexer src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );

	idStr file = "strings/"; file += lang + ".map";
	int len = idLib::fileSystem->ReadFile( file, (void**)&buffer );

	// does not exist
	if (len <= 0)
	{
		file = "strings/default.map";
		len = idLib::fileSystem->ReadFile( file, (void**)&buffer );
	}
	if (len <= 0)
	{
		common->Warning("Cannot find character remapping for language %s.", lang.c_str() );
		return 0;
	}
	
	src.LoadMemory( buffer, strlen( buffer ), file );
	if ( !src.IsLoaded() ) {
		common->Warning("Cannot load character remapping %s.", file.c_str() );
		return 0;
	}

	idToken tok, tok2;
	// format:
	/*
	{
		"ff"	"b2"			// remap character 0xff to 0xb2
	}
	*/
	int i = 0;
	src.ExpectTokenString( "{" );
	while ( src.ReadToken( &tok ) ) {
		if ( tok == "}" ) {
			break;
		}
		if ( src.ReadToken( &tok2 ) ) {
			if ( tok2 == "}" ) {
				common->Warning("Expected a byte, but got } while parsing %s", file.c_str() );
				break;
			}
			// add the two numbers
			//	common->Warning("got '%s' '%s'\n", tok.c_str(), tok2.c_str() );
			m_Remap.Append( (char) tok.GetIntValue() );
			m_Remap.Append( (char) tok2.GetIntValue() );
//			common->Printf("Mapping %i (0x%02x) to %i (0x%02x)\n", tok.GetIntValue(), tok.GetIntValue(), tok2.GetIntValue(), tok2.GetIntValue() );
		}
	}

	return m_Remap.Length() / 2;
}

/*
===============
CI18N::SetLanguage

Change the language. Does not check the language here, as to not restrict
ourselves to a limited support of languages.
===============
*/
void CI18N::SetLanguage( const char* lang, bool firstTime ) {
	if (lang == NULL)
	{
		return;
	}
#ifdef M_DEBUG
	common->Printf("I18N: SetLanguage: '%s'.\n", lang);
#endif

	// store the new setting
	idStr oldLang = m_lang; m_lang = lang;

	// set sysvar tdm_lang
	cv_tdm_lang.SetString( lang );
	m_bMoveArticles = (m_lang != "polish" && m_lang != "italian") ? true : false;

	idStr newLang = idStr(lang);

	// If we need to remap some characters upon loading one of these languages:
	LoadCharacterMapping(newLang);

	// For some reason, "english", "german", "french" and "spanish" share
	// the same font, but "polish" and "russian" get their own font. But
	// since "polish" is actually a copy of the normal western font, use
	// "english" instead to trick D3 into loading the correct font. The
	// dictionary below will be polish, regardless.
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
		common->Printf("I18N: Language '%s' not supported by D3, forcing it.\n", lang);
	}

	// build our combined dictionary, first the TDM base dict
	idStr file = "strings/"; file += m_lang + ".lang";
	m_Dict.Load( file, true, m_Remap.Length() / 2, m_Remap.c_str() );				// true => clear before load

	idLangDict *FMDict = new idLangDict;
	file = "strings/fm/"; file += m_lang + ".lang";
	if ( !FMDict->Load( file, false, m_Remap.Length() / 2, m_Remap.c_str() ) )
	{
		common->Printf("I18N: '%s' not found.\n", file.c_str() );
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
				common->Printf("I18N: Folding '%s' ('%s') into main dictionary.\n", kv->key.c_str(), kv->value.c_str() );
#endif
				m_Dict.AddKeyVal( kv->key.c_str(), kv->value.c_str() );
			}
		}
	}

	// With FM strings it can happen that one translation is missing or incomplete,
	// so fall back to the english version by folding these in, too:

	file = "strings/fm/english.lang";
	if ( !FMDict->Load( file, true, m_Remap.Length() / 2, m_Remap.c_str() ) )
	{
		common->Printf("I18N: '%s' not found, skipping it.\n", file.c_str() );
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
				const char *oldEntry = m_Dict.GetString( kv->key.c_str(), false);
				// if equal, the entry was not found
				if (oldEntry == kv->key.c_str())
				{
#ifdef M_DEBUG
					common->Printf("I18N: Folding '%s' ('%s') into main dictionary as fallback.\n", kv->key.c_str(), kv->value.c_str() );
#endif
					m_Dict.AddKeyVal( kv->key.c_str(), kv->value.c_str() );
				}
			}
		}
	}

	idUserInterface *gui = uiManager->FindGui( "guis/mainmenu.gui", false, true, true );
	if ( gui && (!firstTime) && (oldLang != m_lang && (oldLang == "russian" || m_lang == "russian")))
	{
		// Restarting the game does not really work, the fonts are still broken
		// (for some reason) and if the user was in a game, this would destroy his session.
	    // this does not reload the fonts, either: cmdSystem->BufferCommandText( CMD_EXEC_NOW, "ReloadImages" );

		// So instead just pop-up a message box:
		gui->SetStateBool("MsgBoxVisible", true);

		// TODO: Switching to Russian will show these strings in Russian, but the font is not yet there
		//		 So translate these before loading the new dictionary, and the display them?
		gui->SetStateString("MsgBoxTitle", Translate("#str_02206") );	// Language changed
		gui->SetStateString("MsgBoxText", Translate("#str_02207") );	// You might need to manually restart the game to see the right characters.

		gui->SetStateBool("MsgBoxLeftButtonVisible", false);
		gui->SetStateBool("MsgBoxRightButtonVisible", false);
		gui->SetStateBool("MsgBoxMiddleButtonVisible", true);
		gui->SetStateString("MsgBoxMiddleButtonText", Translate("#str_04339"));

		gui->SetStateString("MsgBoxMiddleButtonCmd", "close_msg_box");
	}

	// finally reload the GUI so it appears in the new language
	uiManager->Reload( true );		// true => reload all

	// and switch back to the General Settings page
	if (gui)
	{
		// Tell the GUI that it was reloaded, so when it gets initialized the next frame,
		// it will land in the Video Settings page
		gameLocal.Printf("Setting reload\n");
		gui->SetStateInt("reload", 1);
	}
	else
	{
		gameLocal.Warning("Cannot find guis/mainmenu.gui");
	}

	// Cycle through all active entities and call "onLanguageChanged" on them
	// some scriptobjects may implement this function to react on language switching
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		idThread* thread = ent->CallScriptFunctionArgs("onLanguageChanged", true, 0, "e", ent);

		if (thread != NULL)
		{
			thread->Execute();
		}
	}
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
	// Do not move articles if the language is italian or polish:
	if ( !m_bMoveArticles )
	{
		return;
	}

	// find index of first " "
	int spaceIdx = title.Find(' ');
	// no space, nothing to do
	if (spaceIdx == -1) { return; }

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

