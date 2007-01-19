#pragma hdrstop
#include "../darkmod/KeyboardHook.h"
#include "../darkmod/KeyboardHookWindows.h"

// global but NOT. anonymous namespace is only available to this file by design
// not meant to be used by anyone else
namespace
{
	CKeyboardHookWindows* g_WindowsHook = NULL;
	
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
CKeyboardHookWindows::KeyboardProc
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
*	wParam Specifies the identifier of the BLANK! message.
*	lParam contains BLANK! pointer
*/

LRESULT CKeyboardHookWindows::KeyboardProc( int nCode, WPARAM wParam, LPARAM lParam )
{
	assert( NULL != m_parent );
	
//	DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING("Keyboard Hook - nCode: %u   wParam: %04X   lParam: %08lX TIME: %d\r", nCode, wParam, lParam, gameLocal.time);
	if( nCode == HC_ACTION )
	{
		//KeyCode_t kc;
		//KeyCode_t tc;

		m_parent->m_KeyPress.VirtualKeyCode = (int) wParam;
		m_parent->m_KeyPress.RepeatCount =   (lParam & 0x0000FFFF);
//		m_parent->m_KeyPress.RepeatCount =		( HIWORD(lParam) & KF_REPEAT ); // doesnt work right ?
		m_parent->m_KeyPress.ScanCode =			(lParam & 0x00FF0000) >> 16;
		//kc.Extended =			(lParam & 0x01000000) >> 24;// fix
		m_parent->m_KeyPress.Extended =			(( HIWORD(lParam) & KF_EXTENDED ) == 1 );
		m_parent->m_KeyPress.Reserved =			(lParam & 0x1E000000) >> 25;
		//kc.Context =			(lParam & 0x20000000) >> 29; // fix
		m_parent->m_KeyPress.Context =			(( HIWORD(lParam) & KF_ALTDOWN )==1);
		m_parent->m_KeyPress.PreviousKeyState =	(((lParam & 0x40000000) >> 30)==1); // fix
		//m_parent->m_KeyPress.PreviousKeyState =	(( HIWORD(lParam) & KF_REPEAT )==1);
		m_parent->m_KeyPress.TransitionState = (((lParam & 0x80000000) >> 31)==1); // fix
		//m_parent->m_KeyPress.TransitionState =	(( HIWORD(lParam) & KF_UP )==1);
		//m_parent->m_KeyPressCount++;
		m_parent->m_KeyPress.KeyPressCount =		m_parent->m_KeyPressCount++;
		//memcpy(&m_parent->m_KeyPress, &kc, sizeof(KeyCode_t));

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
			"Context(Alt): %s    Extended: %s\r",
			m_parent->m_KeyPress.Context? "True" : "False",
			m_parent->m_KeyPress.Extended ? "True":"False"
			);
		DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING(
			"PreviousKeyState: %s    TransitionState: %s \r",
			m_parent->m_KeyPress.PreviousKeyState ? "Down" : "Up",
			m_parent->m_KeyPress.TransitionState ? "Up":"Down"
			);

		TCHAR buffer[32];
		if( GetKeyNameText( lParam, buffer, 32 ) ) 
		{
			DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING( "Key Name Text: \"%s\"\r", buffer );
		}
		else
		{
			DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING( "Key Name Text: Function Failed!\r" );
		}


		// Only update impulses when the player has already spawned
		if( gameLocal.GetLocalPlayer() )
		{
			for( int i = 0; i < IR_COUNT; i++)
			{
				// If the keypress is associated with an impulse then we update it.
				if( m_parent->m_KeyData[i].KeyState != KS_FREE &&
					m_parent->m_KeyData[i].VirtualKeyCode == m_parent->m_KeyPress.VirtualKeyCode )
				{
					memcpy(&m_parent->m_KeyData[i], &m_parent->m_KeyPress, sizeof(KeyCode_t));
					m_parent->m_KeyData[i].KeyState = KS_UPDATED;
					DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING("IR %d updated\r", i);
				}
			}
		}

		// Run key capture for keybinding if it is active
		if( m_parent->m_bKeyCapActive )
			m_parent->KeyCapture();
	}
	// Be polite! Always call next hook proc in chain
	return CallNextHookEx( m_KeyboardHook, nCode, wParam, lParam);
}

bool CKeyboardHookWindows::m_instanceFlag = false;
CKeyboardHookWindows* CKeyboardHookWindows::m_single = NULL;

CKeyboardHookWindows* CKeyboardHookWindows::getInstance( CKeyboardHook* pParent )
{
    if(! m_instanceFlag)
    {
        m_single = new CKeyboardHookWindows( pParent );
        m_instanceFlag = true;
    }
	return m_single;
}

CKeyboardHookWindows::CKeyboardHookWindows( CKeyboardHook* pParent ):
m_parent( pParent ),
m_KeyboardHook( NULL )
{
	g_WindowsHook = this;
	//m_KeyboardHook = SetWindowsHookEx( WH_KEYBOARD, TDM_MouseProc, (HINSTANCE) NULL, GetCurrentThreadId());
	//m_KeyboardHook = SetWindowsHookEx( WH_KEYBOARD_LL, TDM_MouseProc, (HINSTANCE) NULL, GetCurrentThreadId() );
	m_KeyboardHook = SetWindowsHookEx( WH_KEYBOARD, TDM_KeyboardProc, GetModuleHandle(NULL), 0);
	//m_KeyboardHook = SetWindowsHookEx( WH_KEYBOARD_LL, TDM_MouseProc, GetModuleHandle(NULL), 0 );
	assert( NULL != m_KeyboardHook );
}

CKeyboardHookWindows::~CKeyboardHookWindows(void)
{
	UnhookWindowsHookEx( m_KeyboardHook );
	g_WindowsHook = NULL;
	m_KeyboardHook = NULL;
	m_single = NULL;
	m_instanceFlag = false;
}
