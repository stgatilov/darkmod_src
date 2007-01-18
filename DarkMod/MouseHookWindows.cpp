#pragma hdrstop
#include "MouseHook.h"
#include "MouseHookWindows.h"
#include "../darkmod/MouseData.h"

#ifndef WH_MOUSE_LL
#define WH_MOUSE_LL        14
#endif

// global but NOT. anonymous namespace is only available to this file by design
// not meant to be used by anyone else
namespace {
	CMouseHookWindows* g_WindowsHook = NULL;
	typedef struct sMouseDefTranslation
	{
		tMouseDef TDM_Enum; // These will not change regardless of the OS
		// This is what we send to the game code instead of OS Specific
		int WindowsVersion;// The equivalent windows DEF
		// We get this here but DO NOT send to the program rather we translate it
	} MouseDefTranslation_t;
	const MouseDefTranslation_t MouseDefTranslations[] =
	{
		{ TDM_LBUTTONDOWN, WM_LBUTTONDOWN },
		{ TDM_LBUTTONUP,   WM_LBUTTONUP   },
		{ TDM_RBUTTONDOWN, WM_RBUTTONDOWN },
		{ TDM_RBUTTONUP,   WM_RBUTTONUP   },
		{ TDM_MBUTTONDOWN, WM_MBUTTONDOWN },
		{ TDM_MBUTTONUP,   WM_MBUTTONUP   },
		{ TDM_NONE,        -1 }
	};
	
	tMouseDef TranslateMouseAction( int ActionCode ) {
		tMouseDef rc = TDM_NONE;
		for( size_t t = 0; MouseDefTranslations[t].TDM_Enum != TDM_NONE; t++ )
		{
			if( MouseDefTranslations[t].WindowsVersion == ActionCode )
			{
				rc = MouseDefTranslations[t].TDM_Enum;
				break;
			}
		}
		return rc;
	}
}
/*
===========
MouseProc - Windows version of Mouse Hook, call class version to do the work.
============
*/

LRESULT CALLBACK TDM_MouseProc( int nCode, WPARAM wParam, LPARAM lParam )
{
	exit(0);
	return g_WindowsHook->MouseProc( nCode, wParam, lParam );
}

/*
===========
idGameLocal::MouseProc
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
*	wParam Specifies the identifier of the mouse message.
*	lParam contains MOUSEHOOKSTRUCT pointer
*/

LRESULT CMouseHookWindows::MouseProc( int nCode, WPARAM wParam, LPARAM lParam )
{
	exit(0);
	if( nCode == HC_ACTION )
	{
/*
		// Always save message or only ones we care about ?
		// only ones we care about for the moment
		switch( wParam )
		{
		case TDM_LBUTTONDOWN:
		case TDM_LBUTTONUP:
		case TDM_RBUTTONDOWN:
		case TDM_RBUTTONUP:
		case TDM_MBUTTONDOWN:
		case TDM_MBUTTONUP:
			m_MouseDataPrevious = m_MouseDataCurrent; // previous processed message
			m_MouseDataCurrent = (MOUSEHOOKSTRUCT*)lParam;// current
			m_MouseDataCurrent.Action = wParam;// left button right button etc
			break;
		default:
			break;
		}
*/
		switch( wParam )
		{
		case TDM_LBUTTONDOWN:
			m_Mouse_LBPressed = true;
			break;
		case TDM_LBUTTONUP:
			m_Mouse_LBPressed = false;
			break;
		case TDM_RBUTTONDOWN:
			m_Mouse_RBPressed = true;
			break;
		case TDM_RBUTTONUP:
			m_Mouse_RBPressed = false;
			break;			
		case TDM_MBUTTONDOWN:
			m_Mouse_MBPressed = true;
			break;
		case TDM_MBUTTONUP:
			m_Mouse_MBPressed = false;
			break;
		default:
			break;
		}
	}
	// Be polite! Always call next hook proc in chain
	return CallNextHookEx( m_MouseHook, nCode, wParam, lParam);
}

CMouseHookWindows::CMouseHookWindows( CMouseHook* pParent )
:CHookBase(NULL),m_parent(pParent), m_MouseHook(NULL)
{
	//m_KeyboardHook = SetWindowsHookEx(WH_KEYBOARD, TDMKeyboardHook, GetModuleHandle(NULL), 0);
	//m_KeyboardHook = SetWindowsHookEx(WH_KEYBOARD, TDMKeyboardHook, (HINSTANCE) NULL, GetCurrentThreadId());
	//m_MouseHook = SetWindowsHookEx( WH_MOUSE, TDM_MouseProc, (HINSTANCE) NULL, GetCurrentThreadId() );
	m_MouseHook = SetWindowsHookEx( WH_MOUSE_LL, TDM_MouseProc, GetModuleHandle(NULL), 0 );
	if( NULL == m_MouseHook )
	{
		exit(9);
	}
}

CMouseHookWindows::~CMouseHookWindows(void)
{
	if( NULL !=	m_MouseHook )
	{
		UnhookWindowsHookEx( m_MouseHook );
	}
}