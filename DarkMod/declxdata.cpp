#include "../idlib/precompiled.h"
#pragma hdrstop

#include "declxdata.h"

tdmDeclXData::tdmDeclXData()
{
}

tdmDeclXData::~tdmDeclXData()
{
	FreeData();
}

size_t tdmDeclXData::Size() const
{
	return sizeof(tdmDeclXData);
}

const char *tdmDeclXData::DefaultDefinition() const
{
	return "{}";
}

void tdmDeclXData::FreeData()
{
	m_data.Clear();
}

bool tdmDeclXData::Parse( const char *text, const int textLength )
{
	idLexer		src;
	idToken		tKey;
	idToken		tVal;

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS & ~LEXFL_NOSTRINGESCAPECHARS );
	src.SkipUntilString( "{" );

	bool		precache = false;
	idStr		value;

	while (1)
	{
		// Quit upon EOF or closing brace.
		if ( !src.ReadToken( &tKey ) ||
			(tKey.type == TT_PUNCTUATION && tKey.subtype == P_BRACECLOSE) ) {
			break;
		}

		if ( tKey.type != TT_STRING && tKey.type != TT_NAME ) {
			src.Warning( "Invalid key or command: %s", tKey.c_str() );
			MakeDefault();
			return false;
		}

		// Are we parsing a key:value pair or parsing a command?
		int notEOF = src.ReadToken( &tVal );
		if ( notEOF &&
			 tVal.type == TT_PUNCTUATION && tVal.subtype == P_COLON ) {

			// We're parsing a key/value pair.
			if ( !src.ReadToken( &tVal ) ) {
				src.Warning("Unexpected end of file in key:value pair.");
				MakeDefault();
				return false;
			}

			if ( m_data.FindKey( tKey.c_str() ) ) {
				src.Warning( "Redefinition of key: %s", tKey.c_str() );
			}

			if ( tVal.type == TT_STRING ) {

				// Set the key:value pair.
				m_data.Set( tKey.c_str(), tVal.c_str() );

			} else if ( tVal.type == TT_PUNCTUATION && tVal.subtype == P_BRACEOPEN ) {

				value = "";

				while (1) {
					if ( !src.ReadToken( &tVal ) ) {
						src.Warning("EOF encounter inside value block.");
						MakeDefault();
						return false;
					}

					if ( tVal.type == TT_PUNCTUATION && tVal.subtype == P_BRACECLOSE ) {
						break;
					}

					if ( tVal.type != TT_STRING ) {
						src.Warning( "Non-string encountered in value block: %s", tVal.c_str() );
						MakeDefault();
						return false;
					}

					value += tVal + "\n";
				}

				// Set the key:value pair.
				m_data.Set( tKey.c_str(), value.c_str() );

			} else {
				src.Warning( "Invalid value: %s", tVal.c_str() );
				MakeDefault();
				return false;
			}

		} else {

			// We're parsing a command.

			// We're not parsing a key:value pair, so
			// let's pretend we never read that far.
			if (notEOF) {
				src.UnreadToken( &tVal );
			}

			if (tKey.type == TT_STRING ) {
				// unfinished key:value pair?
				src.Warning( "Lone key encountered: %s", tKey.c_str() );
				MakeDefault();
				return false;
			}

			if ( tKey.Icmp("precache") == 0 ) {
				precache = true;
			} else {
				src.Warning( "Unrecognized command: %s", tKey.c_str() );
				MakeDefault();
				return false;
			}
		}
	}

	if (precache) {
		gameLocal.CacheDictionaryMedia( &m_data );
	}

	//We return true to say we parsed it okay
	return true;
}
