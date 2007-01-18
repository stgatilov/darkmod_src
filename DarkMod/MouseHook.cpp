#pragma hdrstop
#include "MouseHook.h"

#ifdef _WINDOWS_
#include "mousehookwindows.h"
#endif
// We will add additional ones for other OS here later

#ifdef MOUSE_TESTING

const char* MouseDefs_Strings[] = {
	"TDM_NONE", // invalid
	"TDM_LBUTTONDOWN",
	"TDM_LBUTTONUP",
	"TDM_RBUTTONDOWN",
	"TDM_RBUTTONUP",
	"TDM_MBUTTONDOWN",
	"TDM_MBUTTONUP",
	NULL
};

const int MouseActions[] = {
	TDM_NONE,
	TDM_LBUTTONDOWN,
	TDM_LBUTTONUP,
	TDM_RBUTTONDOWN,
	TDM_RBUTTONUP,
	TDM_MBUTTONDOWN,
	TDM_MBUTTONUP
};

const char* MouseGetActionString( const int ActionCode )
{
	for( int t = 0; NULL != MouseDefs_Strings[t];t++ )
	{
		if( ActionCode == MouseActions[t] )
		{
			return MouseDefs_Strings[t];
		}
	}
	return MouseDefs_Strings[0];
}

#endif // #ifdef MOUSE_TESTING

bool CMouseHook::m_instanceFlag = false;
CMouseHook* CMouseHook::m_single = NULL;

CMouseHook* CMouseHook::getInstance()
{
    if(! m_instanceFlag)
    {
        m_single = new CMouseHook();
        m_instanceFlag = true;
    }
	return m_single;
}

CMouseHook::CMouseHook(void)
: m_hook(NULL),
  m_Mouse_LBPressed(false),
  m_Mouse_RBPressed(false),
  m_Mouse_MBPressed(false)
{
#ifdef _WINDOWS_
	m_hook = CMouseHookWindows::getInstance(this);
#endif
}

CMouseHook::~CMouseHook(void)
{
	delete m_hook;
}

bool CMouseHook::GetLeftStatus() const
{
	return m_Mouse_LBPressed;
}

bool CMouseHook::GetRightStatus() const
{
	return m_Mouse_RBPressed;
}

bool CMouseHook::GetMiddleStatus() const
{
	return m_Mouse_MBPressed;
}

void CMouseHook::SetLeftStatus( bool Pressed )
{
	m_Mouse_LBPressed = Pressed;
}

void CMouseHook::SetRightStatus( bool Pressed )
{
	m_Mouse_RBPressed = Pressed;
}

void CMouseHook::SetMiddleStatus( bool Pressed )
{
	m_Mouse_MBPressed = Pressed;
}