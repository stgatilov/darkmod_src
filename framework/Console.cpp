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

#include "precompiled.h"
#pragma hdrstop



void SCR_DrawTextLeftAlign ( int &y, const char *text, ... ) id_attribute((format(printf,2,3)));
void SCR_DrawTextRightAlign( int &y, const char *text, ... ) id_attribute((format(printf,2,3)));


#define CONSOLE_POOL			(2097152/8) // PoT / sizeof(char) - while sizeof works in most preprocs, we cant assume that
#define	LINE_WIDTH				(SCREEN_WIDTH / SMALLCHAR_WIDTH -1) // if there are more chars than space to display, they will not wrap
#define	TOTAL_LINES				(CONSOLE_POOL / LINE_WIDTH) // number of lines in the console buffer
#define	CON_TEXTSIZE			(TOTAL_LINES * LINE_WIDTH)

#define	NUM_CON_TIMES			4 // used for printing when the console is up (con_noPrint 0)
#define CONSOLE_FIRSTREPEAT		200 // delay before initial key repeat
#define CONSOLE_REPEAT			100 // delay between repeats - i.e typematic rate
#define	COMMAND_HISTORY			64 // number of console commands kept in history buffer

// the console will query the cvar and command systems for
// command completion information

class idConsoleLocal : public idConsole {
public:
	virtual	void		Init( void );
	virtual void		Shutdown( void );
	virtual	void		LoadGraphics( void );
	virtual	bool		ProcessEvent( const sysEvent_t *event, bool forceAccept );
	virtual	bool		Active( void );
	virtual	void		ClearNotifyLines( void );
	virtual	void		Close( void );
	virtual void		Open( const float frac );
	virtual	void		Print( const char *text );
	virtual	void		Draw( bool forceFullScreen );

	// #3947: Add an optional "unwrap" keyword to Dump() that causes full lines to be continued by
	// the succeeding line without a line break. It's not possible to recover where the original line 
	// breaks were from the console text, but this optional keyword will fix the problem of file paths 
	// being broken up in the output.  
	void				Dump( const char *toFile, const bool unwrap );
	void				Clear();

	//============================

	const idMaterial *	charSetShader;

private:
	void				KeyDownEvent( int key );

	void				Linefeed();

	void				PageUp();
	void				PageDown();
	void				Top();
	void				Bottom();

	void				DrawInput();
	void				DrawNotify();
	void				DrawSolidConsole( float frac );

	void				Scroll();
	void				SetDisplayFraction( float frac );
	void				UpdateDisplayFraction( void );

    virtual void		SaveHistory();
    virtual void		LoadHistory();

	//============================

	bool				keyCatching;

	short				text[CON_TEXTSIZE];
	int					current;		// line where next message will be printed
	int					x;				// offset in current line for next print
	int					display;		// bottom of console displays this line
	int					lastKeyEvent;	// time of last key event for scroll delay
	int					nextKeyEvent;	// keyboard repeat rate

	float				displayFrac;	// approaches finalFrac at scr_conspeed
	float				finalFrac;		// 0.0 to 1.0 lines of console to display
	int					fracTime;		// time of last displayFrac update

	int					vislines;		// in scanlines

	int					times[NUM_CON_TIMES];	// cls.realtime time the line was generated

	idVec4				color;

	idEditField			historyEditLines[COMMAND_HISTORY];

	int					nextHistoryLine;// the last line in the history buffer, not masked
	int					historyLine;	// the line being displayed from history buffer
										// will be <= nextHistoryLine

	idEditField			consoleField;

	static idCVar		con_speed;
	static idCVar		con_notifyTime;
	static idCVar		con_noPrint;

	const idMaterial *	whiteShader;
	const idMaterial *	consoleShader;
};

static idConsoleLocal localConsole;
idConsole	*console = &localConsole;

idCVar idConsoleLocal::con_speed( "con_speed", "3", CVAR_SYSTEM, "speed at which the console moves up and down" );
idCVar idConsoleLocal::con_notifyTime( "con_notifyTime", "3", CVAR_SYSTEM, "time messages are displayed onscreen when console is pulled up" );
#ifdef DEBUG
idCVar idConsoleLocal::con_noPrint( "con_noPrint", "0", CVAR_BOOL|CVAR_SYSTEM|CVAR_NOCHEAT, "print on the console but not onscreen when console is pulled up" );
#else
idCVar idConsoleLocal::con_noPrint( "con_noPrint", "1", CVAR_BOOL|CVAR_SYSTEM|CVAR_NOCHEAT, "print on the console but not onscreen when console is pulled up" );
#endif


/*
=============================================================================

	Misc stats

=============================================================================
*/

/*
==================
SCR_DrawTextLeftAlign
==================
*/
void SCR_DrawTextLeftAlign( int &y, const char *text, ... ) {
	char string[MAX_STRING_CHARS];
	va_list argptr;

	va_start( argptr, text );
	idStr::vsnPrintf( string, sizeof( string ), text, argptr );
	va_end( argptr );

	renderSystem->DrawSmallStringExt( 0, y + 2, string, colorWhite, true, localConsole.charSetShader );
	y += SMALLCHAR_HEIGHT + 4;
}

/*
==================
SCR_DrawTextRightAlign
==================
*/
void SCR_DrawTextRightAlign( int &y, const char *text, ... ) {
	char string[MAX_STRING_CHARS];
	va_list argptr;

	va_start( argptr, text );
	const int i = idStr::vsnPrintf( string, sizeof( string ), text, argptr );
	va_end( argptr );

	renderSystem->DrawSmallStringExt( SCREEN_WIDTH - i * SMALLCHAR_WIDTH, y + 2, string, colorWhite, true, localConsole.charSetShader );
	y += SMALLCHAR_HEIGHT + 4;
}

/*
==================
SCR_DrawFPS
==================
*/
int showFPS_currentValue;	//for automation
int SCR_DrawFPS( int y ) {
	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	static int previous;
	int t = Sys_Milliseconds();
	int frameTime = t - previous;
	previous = t;

	static int previousTimes[128], index;
	int avgCnt = com_showFPSavg.GetInteger();
	previousTimes[index % avgCnt] = frameTime;
	index++;

	static int prevAvgCnt = 0;
	if (prevAvgCnt != avgCnt) {
		//com_showFPSavg was changed --- clear history
		prevAvgCnt = avgCnt;
		for (int i = 0; i < avgCnt; i++)
			previousTimes[i] = -1000000;
	}

	// average multiple frames together to smooth changes out a bit
	int total = 0;
	for (int i = 0; i < avgCnt; i++)
		total += previousTimes[i];
	int fps = (1000 * avgCnt) / idMath::Imax(total, 1);
	if (total < 0)
		fps = -1;
	char *s = va("%ifps", fps);
	showFPS_currentValue = fps;

    renderSystem->DrawBigStringExt(SCREEN_WIDTH - static_cast<int>(strlen(s)*BIGCHAR_WIDTH), y + 2, s, colorWhite, true, localConsole.charSetShader);

	return (y + BIGCHAR_HEIGHT + 4);
}

/*
==================
SCR_DrawMemoryUsage
==================
*/
int SCR_DrawMemoryUsage( int y ) {
	memoryStats_t allocs, frees;
	
	Mem_GetStats( allocs );
	SCR_DrawTextRightAlign( y, "total allocated memory: %4d, %4dkB", allocs.num, allocs.totalSize>>10 );

	Mem_GetFrameStats( allocs, frees );
	SCR_DrawTextRightAlign( y, "frame alloc: %4d, %4dkB  frame free: %4d, %4dkB", allocs.num, allocs.totalSize>>10, frees.num, frees.totalSize>>10 );

	Mem_ClearFrameStats();

	return y;
}

/*
==================
SCR_DrawAsyncStats
==================
*/
#ifdef MULTIPLAYER
int SCR_DrawAsyncStats( int y ) {
	int outgoingRate, incomingRate;
	float outgoingCompression, incomingCompression;

	if ( idAsyncNetwork::server.IsActive() ) {

		SCR_DrawTextRightAlign( y, "server delay = %d msec", idAsyncNetwork::server.GetDelay() );
		SCR_DrawTextRightAlign( y, "total outgoing rate = %d KB/s", idAsyncNetwork::server.GetOutgoingRate() >> 10 );
		SCR_DrawTextRightAlign( y, "total incoming rate = %d KB/s", idAsyncNetwork::server.GetIncomingRate() >> 10 );

		for ( int i = 0; i < MAX_ASYNC_CLIENTS; i++ ) {

			outgoingRate = idAsyncNetwork::server.GetClientOutgoingRate( i );
			incomingRate = idAsyncNetwork::server.GetClientIncomingRate( i );
			outgoingCompression = idAsyncNetwork::server.GetClientOutgoingCompression( i );
			incomingCompression = idAsyncNetwork::server.GetClientIncomingCompression( i );

			if ( outgoingRate != -1 && incomingRate != -1 ) {
				SCR_DrawTextRightAlign( y, "client %d: out rate = %d B/s (% -2.1f%%), in rate = %d B/s (% -2.1f%%)",
											i, outgoingRate, outgoingCompression, incomingRate, incomingCompression );
			}
		}

		idStr msg;
		idAsyncNetwork::server.GetAsyncStatsAvgMsg( msg );
		SCR_DrawTextRightAlign( y, msg.c_str() );

	} else if ( idAsyncNetwork::client.IsActive() ) {

		outgoingRate = idAsyncNetwork::client.GetOutgoingRate();
		incomingRate = idAsyncNetwork::client.GetIncomingRate();
		outgoingCompression = idAsyncNetwork::client.GetOutgoingCompression();
		incomingCompression = idAsyncNetwork::client.GetIncomingCompression();

		if ( outgoingRate != -1 && incomingRate != -1 ) {
			SCR_DrawTextRightAlign( y, "out rate = %d B/s (% -2.1f%%), in rate = %d B/s (% -2.1f%%)",
										outgoingRate, outgoingCompression, incomingRate, incomingCompression );
		}

		SCR_DrawTextRightAlign( y, "packet loss = %d%%, client prediction = %d",
									(int)idAsyncNetwork::client.GetIncomingPacketLoss(), idAsyncNetwork::client.GetPrediction() );

		SCR_DrawTextRightAlign( y, "predicted frames: %d", idAsyncNetwork::client.GetPredictedFrames() );

	}

	return y;
}
#endif
/*
==================
SCR_DrawSoundDecoders
==================
*/
int SCR_DrawSoundDecoders( int y ) {
	unsigned int localTime, sampleTime, percent;
	int index = -1;
	int numActiveDecoders = 0;
	soundDecoderInfo_t decoderInfo;

	while( ( index = soundSystem->GetSoundDecoderInfo( index, decoderInfo ) ) != -1 ) {
		localTime = decoderInfo.current44kHzTime - decoderInfo.start44kHzTime;
		sampleTime = decoderInfo.num44kHzSamples / decoderInfo.numChannels;

		if ( localTime > sampleTime ) {
			if ( decoderInfo.looping ) {
				percent = ( localTime % sampleTime ) * 100 / sampleTime;
			} else {
				percent = 100;
			}
		} else {
			percent = localTime * 100 / sampleTime;
		}
		SCR_DrawTextLeftAlign( y, "%2d:%c%3d%% (%1.2f) %s: %s (%dkB)",
			numActiveDecoders, (decoderInfo.looping ? 'L' : ' '), percent, decoderInfo.lastVolume, 
			decoderInfo.format.c_str(), decoderInfo.name.c_str(), decoderInfo.numBytes >> 10 );
		numActiveDecoders++;
	}

	return y;
}

//=========================================================================

/*
==============
Con_Clear_f
==============
*/
static void Con_Clear_f( const idCmdArgs &args ) {
	localConsole.Clear();
}

/*
==============
Con_Dump_f
==============
*/
static void Con_Dump_f( const idCmdArgs &args ) {
	// #3947: added the "unwrap" logic. See declaration of idConsoleLocal::Dump. 
	bool badargs = false, unwrap = false;
	const int argc = args.Argc();
	if ( argc < 2 || argc > 3 ) {
		badargs = true;
	}

	if ( !badargs && argc == 3) {
		if ( idStr::Icmp( args.Argv( 1 ), "unwrap" ) == 0 ) {
			unwrap = true;
		} else {
			badargs = true;
		}
	}

	if ( badargs ) 
	{
		common->Printf( "usage: conDump [unwrap] <filename>\n\nunwrap prevents line breaks being added "
			"to the dump for full lines in the\nconsole. Fix for long file paths being broken up in the output.\n" );
		return;
	}

	idStr fileName = args.Argv( argc - 1 );
	fileName.DefaultFileExtension(".txt");

	common->Printf( "Dumped console text to %s.\n", fileName.c_str() );

	localConsole.Dump( fileName.c_str(), unwrap );
}

/*
==============
idConsoleLocal::Init
==============
*/
void idConsoleLocal::Init( void ) {
	keyCatching = false;

	lastKeyEvent = -1;
	nextKeyEvent = CONSOLE_FIRSTREPEAT;

	consoleField.Clear();
	consoleField.SetWidthInChars( LINE_WIDTH );

	for ( int i = 0 ; i < COMMAND_HISTORY ; i++ ) {
		historyEditLines[i].Clear();
		historyEditLines[i].SetWidthInChars( LINE_WIDTH );
	}

	cmdSystem->AddCommand( "clear", Con_Clear_f, CMD_FL_SYSTEM, "clears the console" );
	cmdSystem->AddCommand( "conDump", Con_Dump_f, CMD_FL_SYSTEM, "dumps the console text to a file" );
}

/*
==============
idConsoleLocal::Shutdown
==============
*/
void idConsoleLocal::Shutdown( void ) {
	cmdSystem->RemoveCommand( "clear" );
	cmdSystem->RemoveCommand( "conDump" );
}

/*
==============
LoadGraphics

Can't be combined with init, because init happens before
the renderSystem is initialized
==============
*/
void idConsoleLocal::LoadGraphics() {
	charSetShader = declManager->FindMaterial( "textures/consolefont" );
	whiteShader = declManager->FindMaterial( "_white" );
	consoleShader = declManager->FindMaterial( "console" );
}

/*
================
idConsoleLocal::Active
================
*/
bool idConsoleLocal::Active( void ) {
	return keyCatching;
}

/*
================
idConsoleLocal::ClearNotifyLines
================
*/
void idConsoleLocal::ClearNotifyLines() {
	for ( int i = 0 ; i < NUM_CON_TIMES ; i++ ) {
		times[i] = 0;
	}
}

/*
================
idConsoleLocal::Close
================
*/
void idConsoleLocal::Close() {
	keyCatching = false;
	SetDisplayFraction( 0 );
	displayFrac = 0;	// don't scroll to that point, go immediately

	ClearNotifyLines();
	cv_tdm_creep_toggle.SetBool( 0 );
}

/*
================
idConsoleLocal::Open
================
*/
void idConsoleLocal::Open(const float frac) {
	consoleField.Clear();
	keyCatching = true;
	SetDisplayFraction( frac );
	displayFrac = frac;	// don't scroll to that point, go immediately
}

/*
================
idConsoleLocal::Clear
================
*/
void idConsoleLocal::Clear() {
	for ( int i = 0 ; i < CON_TEXTSIZE ; i++ ) {
		text[i] = (idStr::ColorIndex(C_COLOR_CYAN)<<8) | ' ';
	}

	Bottom();		// go to end
}

/*
================
idConsoleLocal::Dump

Save the console contents out to a file
================
*/
void idConsoleLocal::Dump( const char *fileName, const bool unwrap ) {
	int		l, x, i;
	short	*line;
	idFile	*f;
	char	buffer[LINE_WIDTH + 3];

	f = fileSystem->OpenFileWrite( fileName, "fs_devpath", "" );
	if ( !f ) {
		common->Warning( "Couldn't open %s", fileName );
		return;
	}

	// skip empty lines
	l = current - TOTAL_LINES + 1;
	if ( l < 0 ) {
		l = 0;
	}

	for ( ; l <= current ; l++ ) {
		line = text + ( l % TOTAL_LINES ) * LINE_WIDTH;
		for ( x = 0; x < LINE_WIDTH; x++ )
			if ( ( line[x] & 0xff ) > ' ' ) {
				break;
			}
		if ( x != LINE_WIDTH ) {
			break;
		}
	}

	// write the remaining lines
	for ( ; l <= current; l++ ) {
		line = text + ( l % TOTAL_LINES ) * LINE_WIDTH;
		for( i = 0; i < LINE_WIDTH; i++ ) {
			buffer[i] = line[i] & 0xff;
		}
		for ( x = LINE_WIDTH-1; x >= 0; x-- ) {
			if ( buffer[x] <= ' ' ) {
				buffer[x] = 0;
			} else {
				break;
			}
		}
		if ( unwrap && x == LINE_WIDTH - 1 ) {
			// # 3947: We don't add a line break for a full line, but clip off any trailing line break left 
			// over from previous line writes.
			buffer[x+1] = 0;
		} else {
			buffer[x+1] = '\r';
			buffer[x+2] = '\n';
			buffer[x+3] = 0;
		}
        f->Write(buffer, static_cast<int>(strlen(buffer)));
	}

	fileSystem->CloseFile( f );
}

void idConsoleLocal::SaveHistory()
{
    idFile *f = fileSystem->OpenFileWrite("consolehistory.dat");
    for (int i = 0; i < COMMAND_HISTORY; ++i) {
        // make sure the history is in the right order
        int line = (nextHistoryLine + i) % COMMAND_HISTORY;
        const char *s = historyEditLines[line].GetBuffer();
        if (s && s[0]) {
            f->WriteString(s);
        }
    }
    fileSystem->CloseFile(f);
}

void idConsoleLocal::LoadHistory()
{
    idFile *f = fileSystem->OpenFileRead("consolehistory.dat");
    if (f == NULL) // file doesn't exist
        return;

    historyLine = 0;
    idStr tmp;
    for (int i = 0; i < COMMAND_HISTORY; ++i) {
        if (f->Tell() >= f->Length()) {
            break; // EOF is reached
        }
        f->ReadString(tmp);
        historyEditLines[i].SetBuffer(tmp.c_str());
        ++historyLine;
    }
    nextHistoryLine = historyLine;
    fileSystem->CloseFile(f);
}

/*
================
idConsoleLocal::PageUp
================
*/
void idConsoleLocal::PageUp( void ) {
	display -= 2;
	if ( current - display >= TOTAL_LINES ) {
		display = current - TOTAL_LINES + 1;
	}
}

/*
================
idConsoleLocal::PageDown
================
*/
void idConsoleLocal::PageDown( void ) {
	display += 2;
	if ( display > current ) {
		display = current;
	}
}

/*
================
idConsoleLocal::Top
================
*/
void idConsoleLocal::Top( void ) {
	display = 0;
}

/*
================
idConsoleLocal::Bottom
================
*/
void idConsoleLocal::Bottom( void ) {
	display = current;
}


/*
=============================================================================

CONSOLE LINE EDITING

==============================================================================
*/

/*
====================
KeyDownEvent

Handles history and console scrollback
====================
*/
void idConsoleLocal::KeyDownEvent( int key ) {
	
	// Execute F key bindings
	if ( key >= K_F1 && key <= K_F12 ) {
		idKeyInput::ExecKeyBinding( key );
		return;
	}

	// ctrl-L clears screen
	if ( key == 'l' && idKeyInput::IsDown( K_CTRL ) ) {
		Clear();
		return;
	}

	// enter finishes the line
	if ( key == K_ENTER || key == K_KP_ENTER ) {

		common->Printf ( "]%s\n", consoleField.GetBuffer() );

		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, consoleField.GetBuffer() ); // valid command
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "\n" );

        // copy line to history buffer, if it isn't the same as the last command
        if (idStr::Cmp(consoleField.GetBuffer(),
            historyEditLines[(nextHistoryLine + COMMAND_HISTORY - 1) % COMMAND_HISTORY].GetBuffer()) != 0)
        {
            historyEditLines[nextHistoryLine % COMMAND_HISTORY] = consoleField;
            nextHistoryLine++;
        }

        historyLine = nextHistoryLine;
        // clear the next line from old garbage, else the oldest history entry turns up when pressing DOWN
        historyEditLines[nextHistoryLine % COMMAND_HISTORY].Clear();

		consoleField.Clear();
		consoleField.SetWidthInChars( LINE_WIDTH );

		session->UpdateScreen(); // force an update, because the command may take some time

		return;
	}

	// command completion
	if ( key == K_TAB ) {
		consoleField.AutoComplete();
		return;
	}

	// command history (ctrl-p ctrl-n for unix style, scroll up/down command history)
	if ( key == K_UPARROW || ( key == 'p' && idKeyInput::IsDown( K_CTRL ) ) ) {
		if ( nextHistoryLine - historyLine < COMMAND_HISTORY && historyLine > 0 ) {
			historyLine--;
		}
		consoleField = historyEditLines[ historyLine % COMMAND_HISTORY ];
		return;
	}

	if ( key == K_DOWNARROW || ( key == 'n' && idKeyInput::IsDown( K_CTRL ) ) ) {
		if ( historyLine == nextHistoryLine ) {
			return;
		}
		historyLine++;
		consoleField = historyEditLines[ historyLine % COMMAND_HISTORY ];
		return;
	}

	// console scrolling
	if ( key == K_PGDN ) {
		PageDown();
		lastKeyEvent = eventLoop->Milliseconds();
		nextKeyEvent = CONSOLE_FIRSTREPEAT;
		return;
	}

	if ( key == K_PGUP ) {
		PageUp();
		lastKeyEvent = eventLoop->Milliseconds();
		nextKeyEvent = CONSOLE_FIRSTREPEAT;
		return;
	}

	if ( key == K_MWHEELDOWN ) {
		PageDown();
		return;
	}

	if ( key == K_MWHEELUP ) {
		PageUp();
		return;
	}

	// ctrl-home = top of console
	if ( key == K_HOME && idKeyInput::IsDown( K_CTRL ) ) {
		Top();
		return;
	}

	// ctrl-end = bottom of console
	if ( key == K_END && idKeyInput::IsDown( K_CTRL ) ) {
		Bottom();
		return;
	}

	// pass to the normal editline routine
	consoleField.KeyDownEvent( key );
}

/*
==============
Scroll
deals with scrolling text because we don't have key repeat
duzenko #4409 - fix the last/next keyevnt system for variable frame time
==============
*/
void idConsoleLocal::Scroll( ) {

	if (lastKeyEvent == -1 || (lastKeyEvent + nextKeyEvent) > eventLoop->Milliseconds()) {
		return;
	}

	// console scrolling
	else if ( idKeyInput::IsDown( K_PGUP ) ) {
		PageUp();
		lastKeyEvent = eventLoop->Milliseconds();
		nextKeyEvent = CONSOLE_REPEAT;
		return;
	}

	else if ( idKeyInput::IsDown( K_PGDN ) ) {
		PageDown();
		lastKeyEvent = eventLoop->Milliseconds();
		nextKeyEvent = CONSOLE_REPEAT;
		return;
	}
}

/*
==============
SetDisplayFraction

Causes the console to start opening the desired amount.
==============
*/
void idConsoleLocal::SetDisplayFraction( float frac ) {
	finalFrac = frac;
	fracTime = com_frameTime;
}

/*
==============
UpdateDisplayFraction

Scrolls the console up or down based on conspeed
==============
*/
void idConsoleLocal::UpdateDisplayFraction( void ) {
	const float consolespeed = con_speed.GetFloat();

	if ( consolespeed < 0.1f ) {
		fracTime = com_frameTime;
		displayFrac = finalFrac;
		return;
	} else if ( finalFrac < displayFrac ) { // scroll towards the destination height
		displayFrac -= consolespeed * ( com_frameTime - fracTime ) * 0.001f;
		if ( finalFrac > displayFrac ) {
			displayFrac = finalFrac;
		}
		fracTime = com_frameTime;
	} else if ( finalFrac > displayFrac ) {
		displayFrac += consolespeed * ( com_frameTime - fracTime ) * 0.001f;
		if ( finalFrac < displayFrac ) {
			displayFrac = finalFrac;
		}
		fracTime = com_frameTime;
	}
}

/*
==============
ProcessEvent
==============
*/
bool idConsoleLocal::ProcessEvent( const sysEvent_t *event, bool forceAccept ) {
	bool consoleKey;
	consoleKey = event->evType == SE_KEY && ( event->evValue == Sys_GetConsoleKey( false ) || event->evValue == Sys_GetConsoleKey( true ) );

#if ID_CONSOLE_LOCK
	// If the console's not already down, and we have it turned off, check for ctrl+alt
	if ( !keyCatching && !com_allowConsole.GetBool() && ( !idKeyInput::IsDown( K_CTRL ) || !idKeyInput::IsDown( K_ALT ) ) ) {
			consoleKey = false;
	}
#endif

	// we always catch the console key event
	if ( !forceAccept && consoleKey ) {
		// ignore up events
		if ( event->evValue2 == 0 ) {
			return true;
		}

		consoleField.ClearAutoComplete();

		// a down event will toggle the destination lines
		if ( keyCatching ) {
			Close();
			Sys_GrabMouseCursor( true );
			cvarSystem->SetCVarBool( "ui_chat", false );
		} else {
			consoleField.Clear();
			keyCatching = true;
			if ( idKeyInput::IsDown( K_SHIFT ) ) {
				// if the shift key is down, don't open the console as much
				SetDisplayFraction( 0.2f );
			} else {
				SetDisplayFraction( 0.5f );
			}
			cvarSystem->SetCVarBool( "ui_chat", true );
		}
		return true;
	}

	// if we aren't key catching, dump all the other events
	if ( !forceAccept && !keyCatching ) {
		return false;
	}

	// handle key and character events
	if ( event->evType == SE_CHAR ) {
		// never send the console key as a character
		if ( event->evValue != Sys_GetConsoleKey( false ) && event->evValue != Sys_GetConsoleKey( true ) ) {
			consoleField.CharEvent( event->evValue );
		}
		return true;
	}

	if ( event->evType == SE_KEY ) {
		// ignore up key events
		if ( event->evValue2 == 0 ) {
			return true;
		} else {
			KeyDownEvent( event->evValue );
			return true;
		}
	}

	// we don't handle things like mouse, joystick, and network packets
	return false;
}

/*
==============================================================================

PRINTING

==============================================================================
*/

/*
===============
Linefeed
===============
*/
void idConsoleLocal::Linefeed() {

	// mark time for transparent overlay
	if ( current > 0 ) {
		times[current % NUM_CON_TIMES] = com_frameTime;
	}

	if ( display == current ) {
		display++;
	}

	x = 0;
	current++;
	for (int i = 0; i < LINE_WIDTH; i++ ) {
		text[(current % TOTAL_LINES) * LINE_WIDTH + i] = ( ( idStr::ColorIndex(C_COLOR_CYAN)<<8 ) | ' ');
	}
}


/*
================
Print

Handles cursor positioning, line wrapping, etc
================
*/
void idConsoleLocal::Print( const char *txt ) {
	int		c, l, y;
	int		color;

#ifdef ID_ALLOW_TOOLS
	RadiantPrint( txt );

	if( com_editors & EDITOR_MATERIAL ) {
		MaterialEditorPrintConsole(txt);
	}
#endif

	color = idStr::ColorIndex( C_COLOR_CYAN );

	while ( (c = *(const unsigned char*)txt) != 0 ) {
		if ( idStr::IsColor( txt ) ) {
			if ( *(txt+1) == C_COLOR_DEFAULT ) {
				color = idStr::ColorIndex( C_COLOR_CYAN );
			} else {
				color = idStr::ColorIndex( *(txt+1) );
			}
			txt += 2;
			continue;
		}

		y = current % TOTAL_LINES;

		// if we are about to print a new word, check to see
		// if we should wrap to the new line
		if ( c > ' ' && ( x == 0 || text[y*LINE_WIDTH+x-1] <= ' ' ) ) {
			// count word length
			for (l=0 ; l< LINE_WIDTH ; l++) {
				if ( txt[l] <= ' ') {
					break;
				}
			}

			// word wrap
			if (l != LINE_WIDTH && (x + l >= LINE_WIDTH) ) {
				Linefeed();
			}
		}

		txt++;

		switch( c ) {
			case '\n':
				Linefeed ();
				break;
			case '\t':
				do {
					text[y*LINE_WIDTH+x] = (color << 8) | ' ';
					x++;
					if ( x >= LINE_WIDTH ) {
						Linefeed();
						x = 0;
					}
				} while ( x & 3 );
				break;
			case '\r':
				x = 0;
				break;
			default:	// display character and advance
				text[y*LINE_WIDTH+x] = (color << 8) | c;
				x++;
				if ( x >= LINE_WIDTH ) {
					Linefeed();
					x = 0;
				}
				break;
		}
	}


	// mark time for transparent overlay
	if ( current >= 0 ) {
		times[current % NUM_CON_TIMES] = com_frameTime;
	}
}


/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
DrawInput

Draw the editline after a ] prompt
================
*/
void idConsoleLocal::DrawInput() {

	const int y = vislines - ( SMALLCHAR_HEIGHT * 2 );

	if ( consoleField.GetAutoCompleteLength() != 0 ) {
        const int autoCompleteLength = static_cast<int>(strlen(consoleField.GetBuffer())) - consoleField.GetAutoCompleteLength();

		if ( autoCompleteLength > 0 ) {
			renderSystem->SetColor4( 0.8f, 0.2f, 0.2f, 0.45f );

			renderSystem->DrawStretchPic( 2 * SMALLCHAR_WIDTH + consoleField.GetAutoCompleteLength() * SMALLCHAR_WIDTH,
							y + 2, autoCompleteLength * SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT - 2, 0, 0, 0, 0, whiteShader );
		}
	}

	renderSystem->SetColor( idStr::ColorForIndex( C_COLOR_CYAN ) );

	renderSystem->DrawSmallChar( 1 * SMALLCHAR_WIDTH, y, '>', localConsole.charSetShader );

	consoleField.Draw(2 * SMALLCHAR_WIDTH, y, SCREEN_WIDTH - 3 * SMALLCHAR_WIDTH, true, charSetShader );
}


/*
================
DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void idConsoleLocal::DrawNotify() {
	short	*text_p;
	int		time;
	int		currentColor;

	if ( con_noPrint.GetBool() ) {
		return;
	}

	currentColor = idStr::ColorIndex( C_COLOR_WHITE );
	renderSystem->SetColor( idStr::ColorForIndex( currentColor ) );

	int v = 0;
	for ( int i = current-NUM_CON_TIMES+1; i <= current; i++ ) {
		if ( i < 0 ) {
			continue;
		}
		time = times[i % NUM_CON_TIMES];
		if ( time == 0 ) {
			continue;
		}
		time = com_frameTime - time;
		if ( time > con_notifyTime.GetFloat() * 1000 ) {
			continue;
		}
		text_p = text + (i % TOTAL_LINES)*LINE_WIDTH;
		
		for ( int x = 0; x < LINE_WIDTH; x++ ) {
			if ( ( text_p[x] & 0xff ) == ' ' ) {
				continue;
			}
			if ( idStr::ColorIndex(text_p[x]>>8) != currentColor ) {
				currentColor = idStr::ColorIndex(text_p[x]>>8);
				renderSystem->SetColor( idStr::ColorForIndex( currentColor ) );
			}
			renderSystem->DrawSmallChar( (x+1)*SMALLCHAR_WIDTH, v, text_p[x] & 0xff, localConsole.charSetShader );
		}

		v += SMALLCHAR_HEIGHT;
	}

	renderSystem->SetColor( colorCyan );
}

/*
================
DrawSolidConsole

Draws the console with the solid background
================
*/
void idConsoleLocal::DrawSolidConsole( float frac ) {
	int				x;
	float			y;
	int				row, rows;
	short			*text_p;		
	int				lines;
	int				currentColor;

	lines = idMath::FtoiFast( SCREEN_HEIGHT * frac );
	if ( lines <= 0 ) {
		return;
	}

	else if ( lines > SCREEN_HEIGHT ) {
		lines = SCREEN_HEIGHT;
	}

	// draw the background
	y = frac * SCREEN_HEIGHT - 2;
	if ( y < 1.0f ) {
		y = 0.0f;
	} else {
		renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, y, 0, 1.0f - displayFrac, 1, 1, consoleShader );
	}

	renderSystem->SetColor( colorCyan );
	renderSystem->DrawStretchPic( 0, y, SCREEN_WIDTH, 2, 0, 0, 0, 0, whiteShader );
	//renderSystem->SetColor( colorWhite );

	// draw the version number
	renderSystem->SetColor( idStr::ColorForIndex( C_COLOR_CYAN ) );
	{
		// BluePill #4539 - show whether this is a 32-bit or 64-bit binary
		const idStr version = va("%s/%u #%d", ENGINE_VERSION, sizeof(void*) * 8, RevisionTracker::Instance().GetHighestRevision());
		const int vlen = version.Length();

		for ( x = 0; x < vlen; x++ ) {
			renderSystem->DrawSmallChar( SCREEN_WIDTH - ( vlen - x ) * SMALLCHAR_WIDTH, 
				(lines-SMALLCHAR_HEIGHT), version[x], localConsole.charSetShader );
		}
	}


	// draw the text
	vislines = lines;
	rows = (lines-SMALLCHAR_WIDTH)/SMALLCHAR_WIDTH;		// rows of text to draw
	y = lines - (SMALLCHAR_HEIGHT*3);

	// draw from the bottom up
	if ( display != current ) {
		// draw arrows to show the buffer is backscrolled
		renderSystem->SetColor( idStr::ColorForIndex( C_COLOR_CYAN ) );
		for ( x = 0; x < LINE_WIDTH; x += 4 ) {
			renderSystem->DrawSmallChar( (x+1)*SMALLCHAR_WIDTH, idMath::FtoiFast( y ), '^', localConsole.charSetShader );
		}
		y -= SMALLCHAR_HEIGHT;
		rows--;
	}
	
	row = (x == 0) ? (display -1) : display;

	currentColor = idStr::ColorIndex( C_COLOR_WHITE );
	renderSystem->SetColor( idStr::ColorForIndex( currentColor ) );

	for ( int i = 0; i < rows; i++, y -= SMALLCHAR_HEIGHT, row-- ) {
		if ( row < 0 ) {
			break;
		}
		if ( current - row >= TOTAL_LINES ) {
			// past scrollback wrap point
			continue;	
		}

		text_p = text + (row % TOTAL_LINES)*LINE_WIDTH;

		for ( x = 0; x < LINE_WIDTH; x++ ) {
			if ( ( text_p[x] & 0xff ) == ' ' ) {
				continue;
			}

			if ( idStr::ColorIndex(text_p[x]>>8) != currentColor ) {
				currentColor = idStr::ColorIndex(text_p[x]>>8);
				renderSystem->SetColor( idStr::ColorForIndex( currentColor ) );
			}
			renderSystem->DrawSmallChar( (x+1)*SMALLCHAR_WIDTH, idMath::FtoiFast( y ), text_p[x] & 0xff, localConsole.charSetShader );
		}
	}

	// draw the input prompt, user text, and cursor if desired
	DrawInput();

	renderSystem->SetColor( colorCyan );
}


/*
==============
Draw

ForceFullScreen is used by the editor
==============
*/
void idConsoleLocal::Draw( bool forceFullScreen ) {
#if 0
	//Is this even possible?
	if ( !charSetShader ) {
		return;
	}
#endif
	int y = 0; // Padding from the top of the screen for FPS display etc.

	if ( forceFullScreen ) {
		// if we are forced full screen because of a disconnect, 
		// we want the console closed when we go back to a session state
		Close();
		// we are however catching keyboard input
		keyCatching = true;
	}

	Scroll();

	UpdateDisplayFraction();

	if ( displayFrac ) {
		DrawSolidConsole( displayFrac );
	} else if ( forceFullScreen ) {
		DrawSolidConsole( 1.0f );
	} else if ( !con_noPrint.GetBool() ) {
		// draw the notify lines if the developer cvar is set (ie DEBUG build)
		DrawNotify();
	}

	if ( com_showFPS.GetBool() ) {
		y = SCR_DrawFPS( 4 ); // Initial padding from the top of the screen.
	}

	if ( com_showMemoryUsage.GetBool() ) {
		y = SCR_DrawMemoryUsage( y );
	}

	if ( com_showSoundDecoders.GetBool() ) {
		y = SCR_DrawSoundDecoders( y );
	}
	
#ifdef MULTIPLAYER
	if (com_showAsyncStats.GetBool()) {
		y = SCR_DrawAsyncStats( y );
	}
#endif
}
