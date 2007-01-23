#pragma hdrstop
#include "../darkmod/MouseHook.h"

#ifdef _WINDOWS_
#include "mousehookwindows.h"
#endif
// We will add additional ones for other OS here later

static bool init_version = FileVersionList("$Source$  $Revision$   $Date$", init_version);

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