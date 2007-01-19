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
	KeyCode_t kc;
	int i;

	DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING("Keyboard Hook - nCode: %u   wParam: %04X   lParam: %08lX TIME: %d\r", nCode, wParam, lParam, gameLocal.time);
#ifdef _DEBUG
	switch( nCode ) {
		case HC_ACTION:
			gameLocal.Printf( "\nHC_ACTION in TDMKeyboardHook" );
			break;
		case HC_GETNEXT:
			gameLocal.Printf( "\nHC_GETNEXT in TDMKeyboardHook" );
			break;
		case HC_SKIP:
			gameLocal.Printf( "\nHC_SKIP in TDMKeyboardHook" );
			break;
		case HC_NOREMOVE:
			gameLocal.Printf( "\nHC_NOREMOVE in TDMKeyboardHook" );
			break;
		case HC_SYSMODALON:
			gameLocal.Printf( "\nHC_SYSMODALON in TDMKeyboardHook" );
			break;
		case HC_SYSMODALOFF:
			gameLocal.Printf( "\nHC_SYSMODALOFF in TDMKeyboardHook" );
			break;
		default:
			gameLocal.Printf( "\nUnknown action in TDMKeyboardHook!" );
			break;
	};
#endif // #ifdef _DEBUG
	if( nCode == HC_ACTION )
	{
		
		m_parent->m_KeyPressCount++;

		kc.VirtualKeyCode = (int) wParam;
		kc.RepeatCount =		(lParam & 0x0000FFFF);
		kc.ScanCode =			(lParam & 0x00FF0000) >> 16;

		kc.Extended =			(lParam & 0x01000000) >> 24;// fix

		kc.Reserved =			(lParam & 0x1E000000) >> 25;

		kc.Context =			(lParam & 0x20000000) >> 29; // fix
		kc.PreviousKeyState =	(lParam & 0x40000000) >> 30; // fix

		kc.TransitionState =	(lParam & 0x80000000) >> 31; // fix


		kc.KeyPressCount =		m_parent->m_KeyPressCount;

		memcpy(&m_parent->m_KeyPress, &kc, sizeof(KeyCode_t));

		DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING(
			"VirtualKeyCode: %d   RepeatCount: %u   ScanCode: %02X   Extended: %u   Reserved; %02X   Context: %u   PreviousKeyState: %u   TransitionState: %u KeyPressCount: %d\r",
			kc.VirtualKeyCode,
			kc.RepeatCount,
			kc.ScanCode,
			kc.Extended,
			kc.Reserved,
			kc.Context,
			kc.PreviousKeyState,
			kc.TransitionState,
			kc.KeyPressCount
			);

		// Only update impulses when the player has already spawned
		if( gameLocal.GetLocalPlayer() )
		{
			for(i = 0; i < IR_COUNT; i++)
			{
				// If the keypress is associated with an impulse then we update it.
				if( m_parent->m_KeyData[i].KeyState != KS_FREE &&
					m_parent->m_KeyData[i].VirtualKeyCode == kc.VirtualKeyCode )
				{
					memcpy(&m_parent->m_KeyData[i], &kc, sizeof(KeyCode_t));
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
