#ifndef CMOUSEHOOK_WINDOWS_HPP
#define CMOUSEHOOK_WINDOWS_HPP

#pragma once
#include "../idlib/precompiled.h"
#include "MouseHook.h"

#ifndef WH_MOUSE_LL
#define WH_MOUSE_LL        14
#endif

class CMouseHookWindows : public CHookBase
{
public:
	friend class CMouseHook;
	CMouseHookWindows( CMouseHook* pParent );
	virtual ~CMouseHookWindows(void);
	LRESULT MouseProc( int nActionCode, WPARAM wParam, LPARAM lParam );
private:
	CMouseHook* m_parent;
	CMouseHookWindows(void);
	HHOOK m_MouseHook;
};

#endif // #ifndef CMOUSEHOOK_WINDOWS_HPP

