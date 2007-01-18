#ifndef CMOUSEHOOK_WINDOWS_HPP
#define CMOUSEHOOK_WINDOWS_HPP

#pragma once
#include "../idlib/precompiled.h"
#include "MouseHook.h"

class CMouseHookWindows : public CHookBase
{
public:
	virtual ~CMouseHookWindows(void);
	LRESULT MouseProc( int nActionCode, WPARAM wParam, LPARAM lParam );
	static CMouseHookWindows* getInstance( CMouseHook* pParent );
protected:
	CMouseHook* m_parent;
	CMouseHookWindows(void);
	HHOOK m_MouseHook;
private:
	CMouseHookWindows( CMouseHook* pParent );
	static bool m_instanceFlag;
    static CMouseHookWindows* m_single;
};

#endif // #ifndef CMOUSEHOOK_WINDOWS_HPP

