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

// Copyright (C) 2011 Tels (Donated to The Dark Mod)

#ifndef __DARKMOD_I18N_H__
#define __DARKMOD_I18N_H__

/*
===============================================================================

  I18N (Internationalization) - manages translations of strings, including FM-
  specific translations and secondary dictionaries.

  This class is a singleton and initiated/destroyed from gameLocal.

===============================================================================
*/

class CI18N {
public:
	//CLASS_PROTOTYPE( CI18N );

						CI18N( void );

						~CI18N();

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	/**
	* Called by gameLocal at system startup.
	*/
	void				Init ( void );
	/**
    * Called by gameLocal at map shutdown.
    */
	void				Clear ( void );

	/**
	* Attempt to translate a string template in the form of "#str_12345" into
	* the current user selected language, using the FM specific dict first.
	*/
	const char*			Translate( const idStr &in );
	/**
	* The same, but with a const char*
	*/
	const char*			Translate( const char* in );

	/**
	* Returns the current active language.
	*/
	const idStr*		GetCurrentLanguage( void ) const;

	/**
	* Print memory usage info.
    */
	void				Print( void ) const;

	/**
	* Load a new character mapping based on the new language. Returns the
	* number of characters that should be remapped upon dictionary and
	* readable load time.
	*/
	int				LoadCharacterMapping( idStr& lang );

	/**
	* Set a new laguage (example: "english").
	*/
	void				SetLanguage( const char* lang, bool firstTime = false );

	/**
	* Given an English string like "Maps", returns the "#str_xxxxx" template
	* string that would result back in "Maps" under English. Can be used to
	* make translation work even for hard-coded English strings.
	*/
	const char*			TemplateFromEnglish( const char* in);

	/**
	* Changes the given string from "A little House" to "Little House, A",
	* supporting multiple languages like English, German, French etc.
	*/
	void				MoveArticlesToBack(idStr& title);

	/** 
	* To Intercepts calls to common->GetLanguageDict():
	*/
	const idLangDict*	GetLanguageDict(void) const;


private:
	// Called at the end of the game
	void				Shutdown();

	// current language
	idStr				m_lang;
	// depending on current language, move articles to back of Fm name for display?
	bool				m_bMoveArticles;

	// A dictionary consisting of the current language + the current FM dict.
	idLangDict			m_Dict;

	// reverse dictionary for TemplateFromEnglish
	idDict				m_ReverseDict;
	// dictionary to map "A ..." to "..., A" for MoveArticlesToBack()
	idDict				m_ArticlesDict;

	// A table remapping between characters. The string contains two bytes
	// for each remapped character, Length()/2 is the count.
	idStr				m_Remap;
};

#endif /* !__DARKMOD_I18N_H__ */

