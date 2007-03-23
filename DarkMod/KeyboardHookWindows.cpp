/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#pragma hdrstop
#include "../darkmod/KeyboardHook.h"
#include "../darkmod/KeyboardHookWindows.h"

static bool init_version = FileVersionList("$Id$", init_version);


// global but NOT. anonymous namespace is only available to this file by design
// not meant to be used by anyone else
namespace
{
	CKeyboardWindows* g_WindowsHook = NULL;
	
	/*
	===========
	TDM_KeyboardProc - Windows version of Mouse Hook, call class version to do the work.
	============
	*/
	LRESULT CALLBACK TDM_KeyboardProc( int nCode, WPARAM wParam, LPARAM lParam )
	{
		LRESULT rc = 0;
		if( NULL != g_WindowsHook )
		{
			rc = g_WindowsHook->KeyboardProc( nCode, wParam, lParam );
		}
		return rc;
	}
} // anonymous namespace - Only visible to this file

/*
===========
CKeyboardWindows::KeyboardProc
============
*/
/*
*	nCode possible values
*		HC_ACTION           0
*		HC_GETNEXT          1
*		HC_SKIP             2
*		HC_NOREMOVE         3
*		HC_SYSMODALON       4
*		HC_SYSMODALOFF      5
*	I have never recieved anything except HC_ACTION and HC_NOREMOVE(At least when logging)
*	If you also accept the NOREMOVE one you will end up with duplicate results AFAIK
*
*	wParam Specifies contains the key virtual code
*	lParam contains the key mask
wParam
[in] Specifies the virtual-key code of the key that generated the keystroke message.
lParam
[in] Specifies the repeat count, scan code, extended-key flag, context code, previous key-state flag, and transition-state flag. For more information about the lParam parameter, see Keystroke Message Flags. This parameter can be one or more of the following values. 
0-15
Specifies the repeat count. The value is the number of times the keystroke is repeated as a result of the user's holding down the key.
16-23
Specifies the scan code. The value depends on the OEM.
24
Specifies whether the key is an extended key, such as a function key or a key on the numeric keypad. The value is 1 if the key is an extended key; otherwise, it is 0.
25-28
Reserved.
29
Specifies the context code. The value is 1 if the ALT key is down; otherwise, it is 0.
30
Specifies the previous key state. The value is 1 if the key is down before the message is sent; it is 0 if the key is up.
31
Specifies the transition state. The value is 0 if the key is being pressed and 1 if it is being released.

*/


LRESULT CKeyboardWindows::KeyboardProc( int nCode, WPARAM wParam, LPARAM lParam )
{
	assert( NULL != m_parent );
	
	if( nCode == HC_ACTION )
	{
		m_parent->m_KeyPress.VirtualKeyCode = (int) wParam; // Self Explanatory
		m_parent->m_KeyPress.RepeatCount    = (lParam & 0x0000FFFF); // Only useful if machine is seriously lagged
		m_parent->m_KeyPress.ScanCode       = (lParam & 0x00FF0000) >> 16;// better to use the VK's
		m_parent->m_KeyPress.KeyPressCount  = m_parent->m_KeyPressCount++;

		if( BITCHK(lParam, 24 ) == 1 )
		{
			BITSET(	m_parent->m_KeyPress.KeyMask, KEYSTATE_EXTENDED );
		}
		else
		{
			BITCLR(	m_parent->m_KeyPress.KeyMask, KEYSTATE_EXTENDED );
		}
		if( BITCHK( lParam, 30 ) == 1 )
		{
			BITSET(	m_parent->m_KeyPress.KeyMask, KEYSTATE_PRESSED_LAST );
		}
		else
		{
			BITCLR( m_parent->m_KeyPress.KeyMask, KEYSTATE_PRESSED_LAST );
		}
		if( BITCHK( lParam, 31 ) !=1 )
		{
			BITSET(	m_parent->m_KeyPress.KeyMask, KEYSTATE_PRESSED );
		}
		else
		{
			BITCLR( m_parent->m_KeyPress.KeyMask, KEYSTATE_PRESSED );
		}
		if( HIWORD( GetKeyState(VK_LSHIFT) ) )
		{
			BITSET(	m_parent->m_KeyPress.KeyMask, KEYSTATE_SHIFT_LEFT );
		}
		else
		{
			BITCLR( m_parent->m_KeyPress.KeyMask, KEYSTATE_SHIFT_LEFT );
		}
		if( HIWORD( GetKeyState(VK_RSHIFT) ) )
		{
			BITSET(	m_parent->m_KeyPress.KeyMask, KEYSTATE_SHIFT_RIGHT );
		}
		else
		{
			BITCLR( m_parent->m_KeyPress.KeyMask, KEYSTATE_SHIFT_RIGHT );
		}
		if( HIWORD( GetKeyState(VK_LCONTROL) ) )
		{
			BITSET(	m_parent->m_KeyPress.KeyMask, KEYSTATE_CTRL_LFFT );
		}
		else
		{
			BITCLR( m_parent->m_KeyPress.KeyMask, KEYSTATE_CTRL_LFFT );
		}
		if( HIWORD( GetKeyState(VK_RCONTROL) ) )
		{
			BITSET(	m_parent->m_KeyPress.KeyMask, KEYSTATE_CTRL_RIGHT );
		}
		else
		{
			BITCLR( m_parent->m_KeyPress.KeyMask, KEYSTATE_CTRL_RIGHT );
		}
		if( HIWORD( GetKeyState(VK_LMENU) )  )
		{
			BITSET(	m_parent->m_KeyPress.KeyMask, KEYSTATE_ALT_LEFT );
		}
		else
		{
			BITCLR( m_parent->m_KeyPress.KeyMask, KEYSTATE_ALT_LEFT );
		}
		if( HIWORD( GetKeyState(VK_RMENU) ) )
		{
			BITSET(	m_parent->m_KeyPress.KeyMask, KEYSTATE_ALT_RIGHT );
		}
		else
		{
			BITCLR( m_parent->m_KeyPress.KeyMask, KEYSTATE_ALT_RIGHT );
		}

#ifdef _DEBUG
		DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING(
			"VirtualKeyCode: %d    ScanCode: %02X\r",
			m_parent->m_KeyPress.VirtualKeyCode,
			m_parent->m_KeyPress.ScanCode
			);
		DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING(
			"RepeatCount: %u    KeyPressCount: %d\r",
			m_parent->m_KeyPress.RepeatCount,			
			m_parent->m_KeyPress.KeyPressCount
			);		
		DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING(
			//"Context(Alt): %s    Extended: %s\r",
			"Right Alt : %s    Left Alt : %s\r",
			m_parent->m_KeyPress.GetAltRight() ? "True" : "False",
			m_parent->m_KeyPress.GetAltLeft() ? "True" : "False"
			);
		DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING(
			//"Context(Alt): %s    Extended: %s\r",
			"Right Shift : %s    Left Shift : %s\r",
			m_parent->m_KeyPress.GetShiftRight() ? "True" : "False",
			m_parent->m_KeyPress.GetShiftLeft() ? "True" : "False"
			);
		DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING(
			//"Context(Alt): %s    Extended: %s\r",
			"Right Ctrl : %s    Left Ctrl : %s\r",
			m_parent->m_KeyPress.GetCtrlRight() ? "True" : "False",
			m_parent->m_KeyPress.GetCtrlLeft() ? "True" : "False"
			);
		DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING(
			"Was Pressed before ? %s    Pressed now ? %s \r",
			m_parent->m_KeyPress.GetWasPressed() ? "True" : "False",
			m_parent->m_KeyPress.GetPressed() ? "True" : "False"
			);
		TCHAR buffer[128];
		if( GetKeyNameText( lParam, buffer, 32 ) ) 
		{
			DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING( "Key Name Text: \"%s\"\r", buffer );
		}
		else
		{
			DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING( "Key Name Text: Function Failed!\r" );
		}
#endif
		m_parent->ImpuleUpdate();
	}
	// Be polite! Always call next hook proc in chain
	return CallNextHookEx( m_KeyboardHook, nCode, wParam, lParam);
}

bool CKeyboardWindows::m_instanceFlag = false;
CKeyboardWindows* CKeyboardWindows::m_single = NULL;

CKeyboardWindows* CKeyboardWindows::getInstance( CKeyboard* pParent )
{
    if(! m_instanceFlag)
    {
        m_single = new CKeyboardWindows( pParent );
        m_instanceFlag = true;
    }
	return m_single;
}

CKeyboardWindows::CKeyboardWindows( CKeyboard* pParent ):
m_parent( pParent ),
m_KeyboardHook( NULL )
{
	g_WindowsHook = this;
	m_KeyboardHook = SetWindowsHookEx( WH_KEYBOARD, TDM_KeyboardProc, GetModuleHandle(NULL), 0);
	assert( NULL != m_KeyboardHook );
}

CKeyboardWindows::~CKeyboardWindows(void)
{
	UnhookWindowsHookEx( m_KeyboardHook );
	g_WindowsHook = NULL;
	m_KeyboardHook = NULL;
	m_single = NULL;
	m_instanceFlag = false;
}

