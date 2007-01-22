#pragma hdrstop
#include "MouseHook.h"
#include "MouseHookWindows.h"
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501

#include <winuser.h>

// global but NOT. anonymous namespace is only available to this file by design
// not meant to be used by anyone else
namespace
{
	CMouseHookWindows* g_WindowsHook = NULL;
	
	/*
	===========
	MouseProc - Windows version of Mouse Hook, call class version to do the work.
	============
	*/
	LRESULT CALLBACK TDM_MouseProc( int nCode, WPARAM wParam, LPARAM lParam )
	{
		LRESULT rc = 0;
		if( NULL != g_WindowsHook )
		{
			rc = g_WindowsHook->MouseProc( nCode, wParam, lParam );
		}
		return rc;
	}
}// anonymous namespace - Only visible to this file

/*
===========
CMouseHookWindows::MouseProc
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
	assert( NULL != m_parent );
	if( nCode == HC_ACTION )
	{
		switch( wParam )
		{
		case WM_LBUTTONDOWN:
			m_parent->m_Mouse_LBPressed = true;
			break;
		case WM_LBUTTONUP:
			m_parent->m_Mouse_LBPressed = false;
			break;
		case WM_RBUTTONDOWN:
			m_parent->m_Mouse_RBPressed = true;
			break;
		case WM_RBUTTONUP:
			m_parent->m_Mouse_RBPressed = false;
			break;			
		case WM_MBUTTONDOWN:
			m_parent->m_Mouse_MBPressed = true;
			break;
		case WM_MBUTTONUP:
			m_parent->m_Mouse_MBPressed = false;
			break;
		default:
			break;
		}
	}
	// Be polite! Always call next hook proc in chain
	return CallNextHookEx( NULL, nCode, wParam, lParam);
}

bool CMouseHookWindows::m_instanceFlag = false;
CMouseHookWindows* CMouseHookWindows::m_single = NULL;

CMouseHookWindows* CMouseHookWindows::getInstance( CMouseHook* pParent )
{
    if(! m_instanceFlag)
    {
        m_single = new CMouseHookWindows( pParent );
        m_instanceFlag = true;
    }
	return m_single;
}


CMouseHookWindows::CMouseHookWindows( CMouseHook* pParent )
:CMouseHookBase(NULL),m_parent(pParent), m_MouseHook(NULL)
{
	g_WindowsHook = this;
	//m_MouseHook = SetWindowsHookEx( WH_MOUSE_LL, TDM_MouseProc, GetModuleHandle(NULL), 0 );
	m_MouseHook = SetWindowsHookEx( WH_MOUSE, TDM_MouseProc, GetModuleHandle(NULL), 0 );
	assert( NULL != m_MouseHook );
}

CMouseHookWindows::~CMouseHookWindows(void)
{
	m_single = NULL;
	m_instanceFlag = false;
	g_WindowsHook = NULL;
	UnhookWindowsHookEx( m_MouseHook );
}