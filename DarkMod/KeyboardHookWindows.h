#ifndef CKEYBOARDHOOK_WINDOWS_HPP
#define CKEYBOARDHOOK_WINDOWS_HPP

#pragma once
#include "../idlib/precompiled.h"
#include "../darkmod/keyboardhook.h"

class CKeyboardHookWindows :
	public CKeyboardHookBase
{
public:
	virtual ~CKeyboardHookWindows(void);
	LRESULT KeyboardProc( int nCode, WPARAM wParam, LPARAM lParam );
	static CKeyboardHookWindows* getInstance( CKeyboardHook* pParent );
protected:
	CKeyboardHook* m_parent;
	CKeyboardHookWindows(void);// not defined
	HHOOK m_KeyboardHook;
private:
	CKeyboardHookWindows( CKeyboardHook* pParent );
	static bool m_instanceFlag;
    static CKeyboardHookWindows* m_single;
};

#endif // #ifndef CKEYBOARDHOOK_WINDOWS_HPP