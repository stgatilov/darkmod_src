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


#include <Windowsx.h>
#include "win_local.h"
#include "../../renderer/tr_local.h"

#define DINPUT_BUFFERSIZE           256

#define CHAR_FIRSTREPEAT 200
#define CHAR_REPEAT 100

typedef struct MYDATA {
	LONG  lX;                   // X axis goes here
	LONG  lY;                   // Y axis goes here
	LONG  lZ;                   // Z axis goes here
	BYTE  bButtonA;             // One button goes here
	BYTE  bButtonB;             // Another button goes here
	BYTE  bButtonC;             // Another button goes here
	BYTE  bButtonD;             // Another button goes here
} MYDATA;

static DIOBJECTDATAFORMAT rgodf[] = {
  { &GUID_XAxis,    FIELD_OFFSET(MYDATA, lX),       DIDFT_AXIS | DIDFT_ANYINSTANCE,   0,},
  { &GUID_YAxis,    FIELD_OFFSET(MYDATA, lY),       DIDFT_AXIS | DIDFT_ANYINSTANCE,   0,},
  { &GUID_ZAxis,    FIELD_OFFSET(MYDATA, lZ),       0x80000000 | DIDFT_AXIS | DIDFT_ANYINSTANCE,   0,},
  { 0,              FIELD_OFFSET(MYDATA, bButtonA), DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
  { 0,              FIELD_OFFSET(MYDATA, bButtonB), DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
  { 0,              FIELD_OFFSET(MYDATA, bButtonC), 0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
  { 0,              FIELD_OFFSET(MYDATA, bButtonD), 0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
};

//==========================================================================

static const unsigned char s_scantokey[256] = { 
//  0            1       2          3          4       5            6         7
//  8            9       A          B          C       D            E         F
	0,           27,    '1',       '2',        '3',    '4',         '5',      '6', 
	'7',        '8',    '9',       '0',        '-',    '=',          K_BACKSPACE, 9, // 0
	'q',        'w',    'e',       'r',        't',    'y',         'u',      'i', 
	'o',        'p',    '[',       ']',        K_ENTER,K_CTRL,      'a',      's',   // 1
	'd',        'f',    'g',       'h',        'j',    'k',         'l',      ';', 
	'\'',       '`',    K_SHIFT,   '\\',       'z',    'x',         'c',      'v',   // 2
	'b',        'n',    'm',       ',',        '.',    '/',         K_SHIFT,  K_KP_STAR, 
	K_ALT,      ' ',    K_CAPSLOCK,K_F1,       K_F2,   K_F3,        K_F4,     K_F5,  // 3
	K_F6,       K_F7,   K_F8,      K_F9,       K_F10,  K_PAUSE,     K_SCROLL, K_HOME, 
	K_UPARROW,  K_PGUP, K_KP_MINUS,K_LEFTARROW,K_KP_5, K_RIGHTARROW,K_KP_PLUS,K_END, // 4
	K_DOWNARROW,K_PGDN, K_INS,     K_DEL,      0,      0,           0,        K_F11, 
	K_F12,      0,      0,         K_LWIN,     K_RWIN, K_MENU,      0,        0,     // 5
	0,          0,      0,         0,          0,      0,           0,        0, 
	0,          0,      0,         0,          0,      0,           0,        0,     // 6
	0,          0,      0,         0,          0,      0,           0,        0, 
	0,          0,      0,         0,          0,      0,           0,        0,      // 7
// shifted
	0,           27,    '!',       '@',        '#',    '$',         '%',      '^', 
	'&',        '*',    '(',       ')',        '_',    '+',          K_BACKSPACE, 9, // 0
	'q',        'w',    'e',       'r',        't',    'y',         'u',      'i', 
	'o',        'p',    '[',       ']',        K_ENTER,K_CTRL,      'a',      's',   // 1
	'd',        'f',    'g',       'h',        'j',    'k',         'l',      ';', 
	'\'',       '~',    K_SHIFT,   '\\',       'z',    'x',         'c',      'v',   // 2
	'b',        'n',    'm',       ',',        '.',    '/',         K_SHIFT,  K_KP_STAR, 
	K_ALT,      ' ',    K_CAPSLOCK,K_F1,       K_F2,   K_F3,        K_F4,     K_F5,  // 3
	K_F6,       K_F7,   K_F8,      K_F9,       K_F10,  K_PAUSE,     K_SCROLL, K_HOME, 
	K_UPARROW,  K_PGUP, K_KP_MINUS,K_LEFTARROW,K_KP_5, K_RIGHTARROW,K_KP_PLUS,K_END, // 4
	K_DOWNARROW,K_PGDN, K_INS,     K_DEL,      0,      0,           0,        K_F11, 
	K_F12,      0,      0,         K_LWIN,     K_RWIN, K_MENU,      0,        0,     // 5
	0,          0,      0,         0,          0,      0,           0,        0, 
	0,          0,      0,         0,          0,      0,           0,        0,     // 6
	0,          0,      0,         0,          0,      0,           0,        0, 
	0,          0,      0,         0,          0,      0,           0,        0      // 7
}; 

static const unsigned char s_scantokey_german[256] = {
//  0            1       2          3          4       5            6         7
//  8            9       A          B          C       D            E         F
	0,           27,    '1',       '2',        '3',    '4',         '5',      '6', 
	'7',        '8',    '9',       '0',        '?',    '\'',        K_BACKSPACE, 9,  // 0
	'q',        'w',    'e',       'r',        't',    'z',         'u',      'i', 
	'o',        'p',    '=',       '+',        K_ENTER,K_CTRL,      'a',      's',   // 1
	'd',        'f',    'g',       'h',        'j',    'k',         'l',      '[', 
	']',        '`',    K_SHIFT,   '#',        'y',    'x',         'c',      'v',   // 2
	'b',        'n',    'm',       ',',        '.',    '-',         K_SHIFT,  K_KP_STAR, 
	K_ALT,      ' ',    K_CAPSLOCK,K_F1,       K_F2,   K_F3,        K_F4,     K_F5,  // 3
	K_F6,       K_F7,   K_F8,      K_F9,       K_F10,  K_PAUSE,     K_SCROLL, K_HOME, 
	K_UPARROW,  K_PGUP, K_KP_MINUS,K_LEFTARROW,K_KP_5, K_RIGHTARROW,K_KP_PLUS,K_END, // 4
	K_DOWNARROW,K_PGDN, K_INS,     K_DEL,      0,      0,           '<',      K_F11, 
	K_F12,      0,      0,         K_LWIN,     K_RWIN, K_MENU,      0,        0,     // 5
	0,          0,      0,         0,          0,      0,           0,        0, 
	0,          0,      0,         0,          0,      0,           0,        0,     // 6
	0,          0,      0,         0,          0,      0,           0,        0, 
	0,          0,      0,         0,          0,      0,           0,        0,      // 7
// shifted
	0,           27,    '1',       '2',        '3',    '4',         '5',      '6', 
	'7',        '8',    '9',       '0',        '?',    '\'',        K_BACKSPACE, 9,  // 0
	'q',        'w',    'e',       'r',        't',    'z',         'u',      'i', 
	'o',        'p',    '=',       '+',        K_ENTER,K_CTRL,      'a',      's',   // 1
	'd',        'f',    'g',       'h',        'j',    'k',         'l',      '[', 
	']',        '`',    K_SHIFT,   '#',        'y',    'x',         'c',      'v',   // 2
	'b',        'n',    'm',       ',',        '.',    '-',         K_SHIFT,  K_KP_STAR, 
	K_ALT,      ' ',    K_CAPSLOCK,K_F1,       K_F2,   K_F3,        K_F4,     K_F5,  // 3
	K_F6,       K_F7,   K_F8,      K_F9,       K_F10,  K_PAUSE,     K_SCROLL, K_HOME, 
	K_UPARROW,  K_PGUP, K_KP_MINUS,K_LEFTARROW,K_KP_5, K_RIGHTARROW,K_KP_PLUS,K_END, // 4
	K_DOWNARROW,K_PGDN, K_INS,     K_DEL,      0,      0,           '<',      K_F11, 
	K_F12,      0,      0,         K_LWIN,     K_RWIN, K_MENU,      0,        0,     // 5
	0,          0,      0,         0,          0,      0,           0,        0, 
	0,          0,      0,         0,          0,      0,           0,        0,     // 6
	0,          0,      0,         0,          0,      0,           0,        0, 
	0,          0,      0,         0,          0,      0,           0,        0      // 7
}; 

static const unsigned char s_scantokey_french[256] = {
//  0            1       2          3          4       5            6         7
//  8            9       A          B          C       D            E         F
	0,           27,    '1',       '2',        '3',    '4',         '5',      '6', 
	'7',        '8',    '9',       '0',        ')',    '=',         K_BACKSPACE, 9, // 0 
	'a',        'z',    'e',       'r',        't',    'y',         'u',      'i', 
	'o',        'p',    '^',       '$',        K_ENTER,K_CTRL,      'q',      's',      // 1 
	'd',        'f',    'g',       'h',        'j',    'k',         'l',      'm', 
	'ù',        '`',    K_SHIFT,   '*',        'w',    'x',         'c',      'v',      // 2 
	'b',        'n',    ',',       ';',        ':',    '!',         K_SHIFT,  K_KP_STAR,
	K_ALT,      ' ',    K_CAPSLOCK,K_F1,       K_F2,   K_F3,        K_F4,     K_F5,  // 3
	K_F6,       K_F7,   K_F8,      K_F9,       K_F10,  K_PAUSE,     K_SCROLL, K_HOME, 
	K_UPARROW,  K_PGUP, K_KP_MINUS,K_LEFTARROW,K_KP_5, K_RIGHTARROW,K_KP_PLUS,K_END, // 4
	K_DOWNARROW,K_PGDN, K_INS,     K_DEL,      0,      0,           '<',      K_F11, 
	K_F12,      0,      0,         K_LWIN,     K_RWIN, K_MENU,      0,        0,     // 5
	0,          0,      0,         0,          0,      0,           0,        0, 
	0,          0,      0,         0,          0,      0,           0,        0,     // 6
	0,          0,      0,         0,          0,      0,           0,        0, 
	0,          0,      0,         0,          0,      0,           0,        0,      // 7
// shifted
	0,           27,    '&',       'é',        '\"',    '\'',         '(',      '-', 
	'è',        '_',    'ç',       'à',        '°',    '+',         K_BACKSPACE, 9, // 0 
	'a',        'z',    'e',       'r',        't',    'y',         'u',      'i', 
	'o',        'p',    '^',       '$',        K_ENTER,K_CTRL,      'q',      's',      // 1 
	'd',        'f',    'g',       'h',        'j',    'k',         'l',      'm', 
	'ù',        0,    K_SHIFT,   '*',        'w',    'x',         'c',      'v',      // 2 
	'b',        'n',    ',',       ';',        ':',    '!',         K_SHIFT,  K_KP_STAR,
	K_ALT,      ' ',    K_CAPSLOCK,K_F1,       K_F2,   K_F3,        K_F4,     K_F5,  // 3
	K_F6,       K_F7,   K_F8,      K_F9,       K_F10,  K_PAUSE,     K_SCROLL, K_HOME, 
	K_UPARROW,  K_PGUP, K_KP_MINUS,K_LEFTARROW,K_KP_5, K_RIGHTARROW,K_KP_PLUS,K_END, // 4
	K_DOWNARROW,K_PGDN, K_INS,     K_DEL,      0,      0,           '<',      K_F11, 
	K_F12,      0,      0,         K_LWIN,     K_RWIN, K_MENU,      0,        0,     // 5
	0,          0,      0,         0,          0,      0,           0,        0, 
	0,          0,      0,         0,          0,      0,           0,        0,     // 6
	0,          0,      0,         0,          0,      0,           0,        0, 
	0,          0,      0,         0,          0,      0,           0,        0      // 7
}; 

static const unsigned char s_scantokey_spanish[256] = { 
//  0            1       2          3          4       5            6         7
//  8            9       A          B          C       D            E         F
	0,           27,    '1',       '2',        '3',    '4',         '5',      '6', 
	'7',        '8',    '9',       '0',        '\'',   '¡',         K_BACKSPACE, 9,  // 0 
	'q',        'w',    'e',       'r',        't',    'y',         'u',      'i', 
	'o',        'p',    '`',       '+',        K_ENTER,K_CTRL,      'a',      's',   // 1 
	'd',        'f',    'g',       'h',        'j',    'k',         'l',      'ñ', 
	'´',        'º',    K_SHIFT,   'ç',        'z',    'x',         'c',      'v',   // 2 
	'b',        'n',    'm',       ',',        '.',    '-',         K_SHIFT,  K_KP_STAR, 
	K_ALT,      ' ',    K_CAPSLOCK,K_F1,       K_F2,   K_F3,        K_F4,     K_F5,  // 3
	K_F6,       K_F7,   K_F8,      K_F9,       K_F10,  K_PAUSE,     K_SCROLL, K_HOME, 
	K_UPARROW,  K_PGUP, K_KP_MINUS,K_LEFTARROW,K_KP_5, K_RIGHTARROW,K_KP_PLUS,K_END, // 4
	K_DOWNARROW,K_PGDN, K_INS,     K_DEL,      0,      0,           '<',      K_F11, 
	K_F12,      0,      0,         K_LWIN,     K_RWIN, K_MENU,      0,        0,     // 5
	0,          0,      0,         0,          0,      0,           0,        0, 
	0,          0,      0,         0,          0,      0,           0,        0,     // 6
	0,          0,      0,         0,          0,      0,           0,        0, 
	0,          0,      0,         0,          0,      0,           0,        0,      // 7
// shifted
	0,           27,    '!',       '\"',        '·',    '$',         '%',      '&', 
	'/',        '(',    ')',       '=',        '?',   '¿',         K_BACKSPACE, 9,  // 0 
	'q',        'w',    'e',       'r',        't',    'y',         'u',      'i', 
	'o',        'p',    '^',       '*',        K_ENTER,K_CTRL,      'a',      's',   // 1 
	'd',        'f',    'g',       'h',        'j',    'k',         'l',      'Ñ', 
	'¨',        'ª',    K_SHIFT,   'Ç',        'z',    'x',         'c',      'v',   // 2 
	'b',        'n',    'm',       ',',        '.',    '-',         K_SHIFT,  K_KP_STAR, 
	K_ALT,      ' ',    K_CAPSLOCK,K_F1,       K_F2,   K_F3,        K_F4,     K_F5,  // 3
	K_F6,       K_F7,   K_F8,      K_F9,       K_F10,  K_PAUSE,     K_SCROLL, K_HOME, 
	K_UPARROW,  K_PGUP, K_KP_MINUS,K_LEFTARROW,K_KP_5, K_RIGHTARROW,K_KP_PLUS,K_END, // 4
	K_DOWNARROW,K_PGDN, K_INS,     K_DEL,      0,      0,           '<',      K_F11, 
	K_F12,      0,      0,         K_LWIN,     K_RWIN, K_MENU,      0,        0,     // 5
	0,          0,      0,         0,          0,      0,           0,        0, 
	0,          0,      0,         0,          0,      0,           0,        0,     // 6
	0,          0,      0,         0,          0,      0,           0,        0, 
	0,          0,      0,         0,          0,      0,           0,        0      // 7
}; 

static const unsigned char s_scantokey_italian[256] = { 
//  0            1       2          3          4       5            6         7
//  8            9       A          B          C       D            E         F
		0,           27,    '1',       '2',        '3',    '4',         '5',      '6', 
		'7',        '8',    '9',       '0',        '\'',   'ì',         K_BACKSPACE, 9,  // 0 
		'q',        'w',    'e',       'r',        't',    'y',         'u',      'i', 
		'o',        'p',    'è',       '+',        K_ENTER,K_CTRL,      'a',      's',   // 1 
		'd',        'f',    'g',       'h',        'j',    'k',         'l',      'ò', 
		'à',        '\\',    K_SHIFT,   'ù',        'z',    'x',         'c',      'v',   // 2 
		'b',        'n',    'm',       ',',        '.',    '-',         K_SHIFT,  K_KP_STAR, 
		K_ALT,      ' ',    K_CAPSLOCK,K_F1,       K_F2,   K_F3,        K_F4,     K_F5,  // 3
		K_F6,       K_F7,   K_F8,      K_F9,       K_F10,  K_PAUSE,     K_SCROLL, K_HOME, 
		K_UPARROW,  K_PGUP, K_KP_MINUS,K_LEFTARROW,K_KP_5, K_RIGHTARROW,K_KP_PLUS,K_END, // 4
		K_DOWNARROW,K_PGDN, K_INS,     K_DEL,      0,      0,           '<',      K_F11, 
		K_F12,      0,      0,         K_LWIN,     K_RWIN, K_MENU,      0,        0,     // 5
		0,          0,      0,         0,          0,      0,           0,        0, 
		0,          0,      0,         0,          0,      0,           0,        0,     // 6
		0,          0,      0,         0,          0,      0,           0,        0, 
		0,          0,      0,         0,          0,      0,           0,        0,      // 7
// shifted
		0,           27,    '!',       '\"',        '£',    '$',         '%',      '&', 
		'/',        '(',    ')',       '=',        '?',   '^',         K_BACKSPACE, 9,  // 0 
		'q',        'w',    'e',       'r',        't',    'y',         'u',      'i', 
		'o',        'p',    'é',       '*',        K_ENTER,K_CTRL,      'a',      's',   // 1 
		'd',        'f',    'g',       'h',        'j',    'k',         'l',      'ç', 
		'°',        '|',    K_SHIFT,   '§',        'z',    'x',         'c',      'v',   // 2 
		'b',        'n',    'm',       ',',        '.',    '-',         K_SHIFT,  K_KP_STAR, 
		K_ALT,      ' ',    K_CAPSLOCK,K_F1,       K_F2,   K_F3,        K_F4,     K_F5,  // 3
		K_F6,       K_F7,   K_F8,      K_F9,       K_F10,  K_PAUSE,     K_SCROLL, K_HOME, 
		K_UPARROW,  K_PGUP, K_KP_MINUS,K_LEFTARROW,K_KP_5, K_RIGHTARROW,K_KP_PLUS,K_END, // 4
		K_DOWNARROW,K_PGDN, K_INS,     K_DEL,      0,      0,           '<',      K_F11, 
		K_F12,      0,      0,         K_LWIN,     K_RWIN, K_MENU,      0,        0,     // 5
		0,          0,      0,         0,          0,      0,           0,        0, 
		0,          0,      0,         0,          0,      0,           0,        0,     // 6
		0,          0,      0,         0,          0,      0,           0,        0, 
		0,          0,      0,         0,          0,      0,           0,        0		 // 7

	
}; 

static const unsigned char *keyScanTable = s_scantokey;	

// this should be part of the scantables and the scan tables should be 512 bytes
// (256 scan codes, shifted and unshifted).  Changing everything to use 512 byte
// scan tables now might introduce bugs in tested code.  Since we only need to fix
// the right-alt case for non-US keyboards, we're just using a special-case table
// for it.  Eventually, the tables above should be fixed to handle all possible
// scan codes instead of just the first 128.
static unsigned char	rightAltKey = K_ALT;

#define NUM_OBJECTS (sizeof(rgodf) / sizeof(rgodf[0]))

static DIDATAFORMAT	df = {
	sizeof(DIDATAFORMAT),       // this structure
	sizeof(DIOBJECTDATAFORMAT), // size of object data format
	DIDF_RELAXIS,               // absolute axis coordinates
	sizeof(MYDATA),             // device data size
	NUM_OBJECTS,                // number of objects
	rgodf,                      // and here they are
};

/*
==========================
IN_ActivateMouse
==========================
*/
void IN_ActivateMouse( void ) {
	int i;

	if ( !win32.in_mouse.GetBool() || win32.mouseGrabbed ) {
		return;
	}

	win32.mouseGrabbed = true;
	for ( i = 0; i < 10; i++ ) {
		if ( ::ShowCursor( false ) < 0 ) {
			break;
		}
	}
}

/*
==========================
IN_DeactivateMouse
==========================
*/
void IN_DeactivateMouse( void ) {
	int i;

	if (!win32.mouseGrabbed ) {
		return;
	}

	for ( i = 0; i < 10; i++ ) {
		if ( ::ShowCursor( true ) >= 0 ) {
			break;
		}
	}
	win32.mouseGrabbed = false;
}

/*
==========================
IN_DeactivateMouseIfWindowed
==========================
*/
void IN_DeactivateMouseIfWindowed( void ) {
	if ( !win32.cdsFullscreen ) {
		IN_DeactivateMouse();
	}
}

/*
============================================================

  MOUSE CONTROL

============================================================
*/


/*
===========
Sys_ShutdownInput
===========
*/
void Sys_ShutdownInput( void ) {
	IN_DeactivateMouse();
}

/*
===========
Sys_InitInput
===========
*/
void Sys_InitInput( void ) {
	common->Printf ("\n------- Input Initialization -------\n");
	if ( win32.in_mouse.GetBool() ) {
		// don't grab the mouse on initialization
		Sys_GrabMouseCursor( false );
	} else {
		common->Printf ("Mouse control not active.\n");
	}
	common->Printf ("------------------------------------\n");
	win32.in_mouse.ClearModified();
}

/*
===========
Sys_InitScanTable
===========
*/
void Sys_InitScanTable( void ) {
	idStr lang = cvarSystem->GetCVarString( "sys_lang" );
	if ( lang.Length() == 0 ) {
		lang = "english";
	}
	if ( lang.Icmp( "english" ) == 0 ) {
		keyScanTable = s_scantokey;
		// the only reason that english right alt binds as K_ALT is so that 
		// users who were using right-alt before the patch don't suddenly find
		// that only left-alt is working.
		rightAltKey = K_ALT;
	} else if ( lang.Icmp( "spanish" ) == 0 ) {
		keyScanTable = s_scantokey_spanish;
		rightAltKey = K_RIGHT_ALT;
	} else if ( lang.Icmp( "french" ) == 0 ) {
		keyScanTable = s_scantokey_french;
		rightAltKey = K_RIGHT_ALT;
	} else if ( lang.Icmp( "german" ) == 0 ) {
		keyScanTable = s_scantokey_german;
		rightAltKey = K_RIGHT_ALT;
	} else if ( lang.Icmp( "italian" ) == 0 ) {
		keyScanTable = s_scantokey_italian;
		rightAltKey = K_RIGHT_ALT;
	}
}

/*
==================
Sys_GetScanTable
==================
*/
const unsigned char *Sys_GetScanTable( void ) {
	return keyScanTable;
}

/*
===============
Sys_GetConsoleKey
===============
*/
unsigned char Sys_GetConsoleKey( bool shifted ) {
	return keyScanTable[41 + ( shifted ? 128 : 0 )];
}

/*
==================
IN_Frame

Called every frame, even if not generating commands
==================
*/
void IN_Frame( void ) {
	bool	shouldGrab = true;

	if ( !win32.in_mouse.GetBool() ) {
		shouldGrab = false;
	}
	// if fullscreen, we always want the mouse
	if ( !win32.cdsFullscreen ) {
		if ( win32.mouseReleased ) {
			shouldGrab = false;
		}
		if ( win32.movingWindow ) {
			shouldGrab = false;
		}
		if ( !win32.activeApp ) {
			shouldGrab = false;
		}
	}

	if ( shouldGrab != win32.mouseGrabbed ) {
		if ( win32.mouseGrabbed ) {
			IN_DeactivateMouse();
		} else {
			IN_ActivateMouse();
		}
	}
}


void	Sys_GrabMouseCursor( bool grabIt ) {
	win32.mouseReleased = !grabIt;
	if ( !grabIt ) {
		// release it right now
		IN_Frame();
	}
}

void Sys_AdjustMouseMovement(float &dx, float &dy) {
	static int cp_mouseLastQuery = -1000000000;
	if ((clock() - cp_mouseLastQuery) > CLOCKS_PER_SEC) {
		//ask for mouse settings regularly, but at most once per second
		bool init = (cp_mouseLastQuery == -1000000000);
		cp_mouseLastQuery = clock();

		//control panel parameters
		SystemParametersInfo(SPI_GETMOUSESPEED, 0, &win32.cp_mouseSpeed, 0);
		SystemParametersInfo(SPI_GETMOUSE, 0, win32.cp_mouseAccel, 0);

		//DPI scaling settings (Windows 10)
		if (init) {
			win32.hShcoreDll = LoadLibrary("Shcore.dll");
			win32.pfGetDpiForMonitor = win32.hShcoreDll ? (Win32Vars_t::GetDpiForMonitor_t)GetProcAddress(win32.hShcoreDll, "GetDpiForMonitor") : NULL;
		}
		if (win32.pfGetDpiForMonitor) {
			HMONITOR hMon = MonitorFromWindow(win32.hWnd, MONITOR_DEFAULTTONEAREST);
			if (hMon && hMon != INVALID_HANDLE_VALUE) {
				UINT dpiX, dpiY;
				HRESULT hr = win32.pfGetDpiForMonitor(hMon, 0/*MDT_Effective_DPI*/, &dpiX, &dpiY);
				if (SUCCEEDED(hr)) {
					win32.effectiveScreenDpi[0] = dpiX;
					win32.effectiveScreenDpi[1] = dpiY;
				}
			}
		}
	}

	// based on https://msdn.microsoft.com/en-us/library/windows/desktop/ms646260(v=vs.85).aspx
	float fullDelta = idMath::Fmax(idMath::Fabs(dx), idMath::Fabs(dy));
	if (win32.cp_mouseAccel[0] && fullDelta > win32.cp_mouseAccel[0]) {
		dx *= 2.0f, dy *= 2.0f;
		if (win32.cp_mouseAccel[2] == 2 && fullDelta > win32.cp_mouseAccel[1])
			dx *= 2.0f, dy *= 2.0f;
	}
	dx *= (win32.cp_mouseSpeed * 0.1f) * (win32.effectiveScreenDpi[0] / 96.0f);
	dy *= (win32.cp_mouseSpeed * 0.1f) * (win32.effectiveScreenDpi[1] / 96.0f);
}


//=====================================================================================

struct InputEvent {
	int action, value;
};
idList<InputEvent> keyboardInputEvents;
idList<InputEvent> mouseInputEvents;

/*
====================
Sys_PollKeyboardInputEvents
====================
*/
int Sys_PollKeyboardInputEvents( void ) {
	return keyboardInputEvents.Num();
}

/*
====================
Sys_PollKeyboardInputEvents
====================
*/
int Sys_ReturnKeyboardInputEvent( const int n, int &ch, bool &state ) {
	ch = keyboardInputEvents[n].action;
	state = keyboardInputEvents[n].value;
	return ch;
}

void Sys_StdKeyboardInput( UINT uMsg, WPARAM wParam, LPARAM lParam ) {
	int key;
	switch ( uMsg ) {
	case WM_KEYDOWN:
		key = MapKey( lParam );
		if ( key == K_CTRL || key == K_ALT || key == K_RIGHT_ALT ) {
			// let direct-input handle this because windows sends Alt-Gr
			// as two events (ctrl then alt)
			break;
		}
		Sys_QueEvent( win32.sysMsgTime, SE_KEY, key, true, 0, NULL );
		keyboardInputEvents.Append( { key, 1 } );
		break;
	case WM_KEYUP:
		key = MapKey( lParam );
		if ( key == K_PRINT_SCR ) {
			// don't queue printscreen keys.  Since windows doesn't send us key
			// down events for this, we handle queueing them with DirectInput
			break;
		} else if ( key == K_CTRL || key == K_ALT || key == K_RIGHT_ALT ) {
			// let direct-input handle this because windows sends Alt-Gr
			// as two events (ctrl then alt)
			break;
		}
		Sys_QueEvent( win32.sysMsgTime, SE_KEY, key, false, 0, NULL );
		keyboardInputEvents.Append( { key, 0 } );
		break;
	}
}

void Sys_EndKeyboardInputEvents( void ) {
	keyboardInputEvents.Clear();
}

//=====================================================================================

int Sys_PollMouseInputEvents( void ) {
	return mouseInputEvents.Num();
}

int Sys_ReturnMouseInputEvent( const int n, int &action, int &value ) {
	action = mouseInputEvents[n].action;
	value = mouseInputEvents[n].value;
	return 1;
}

void Sys_StdMouseInput( UINT uMsg, WPARAM wParam, LPARAM lParam ) {
	if ( !win32.mouseGrabbed )
		return;
	switch ( uMsg ) {
	case WM_MOUSEWHEEL: {
		int delta = GET_WHEEL_DELTA_WPARAM( wParam ) / WHEEL_DELTA;
		mouseInputEvents.Append( { M_DELTAZ, delta } );
		int key = delta < 0 ? K_MWHEELDOWN : K_MWHEELUP;
		delta = abs( delta );
		while ( delta-- > 0 ) {
			Sys_QueEvent( win32.sysMsgTime, SE_KEY, key, true, 0, NULL );
			Sys_QueEvent( win32.sysMsgTime, SE_KEY, key, false, 0, NULL );
		}
		break;
	}
	case WM_MOUSEMOVE: {
		int centerX = glConfig.vidWidth / 2;
		int centerY = glConfig.vidHeight / 2;
		int xPos = GET_X_LPARAM( lParam ) - centerX;
		int yPos = GET_Y_LPARAM( lParam ) - centerY;
		Sys_QueEvent( win32.sysMsgTime, SE_MOUSE, xPos, yPos );
		if ( xPos || yPos ) {
			POINT p = { centerX, centerY };
			if ( ClientToScreen( win32.hWnd, &p ) )
				SetCursorPos( p.x, p.y );
		}
		if ( xPos )
			mouseInputEvents.Append( { M_DELTAX, xPos } );
		if ( yPos )
			mouseInputEvents.Append( { M_DELTAY , yPos } );
		break;
	}
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
		Sys_QueEvent( win32.sysMsgTime, SE_KEY, K_MOUSE1, WM_LBUTTONUP - uMsg );
		mouseInputEvents.Append( { M_ACTION1, WM_LBUTTONUP - (int)uMsg } );
		break;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
		Sys_QueEvent( win32.sysMsgTime, SE_KEY, K_MOUSE2, WM_RBUTTONUP - uMsg );
		mouseInputEvents.Append( { M_ACTION2, WM_RBUTTONUP - (int) uMsg } );
		break;
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
		Sys_QueEvent( win32.sysMsgTime, SE_KEY, K_MOUSE3, WM_MBUTTONUP - uMsg );
		mouseInputEvents.Append( { M_ACTION3, WM_MBUTTONUP - (int) uMsg } );
		break;
	}
}

void Sys_EndMouseInputEvents( void ) {
	mouseInputEvents.Clear();
}

unsigned char Sys_MapCharForKey( int key ) {
	return (unsigned char)key;
}
