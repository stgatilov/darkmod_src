#ifndef CKEYBOARDHOOK_WINDOWS_HPP
#define CKEYBOARDHOOK_WINDOWS_HPP

#pragma once
#include "../idlib/precompiled.h"
#include "../darkmod/keyboardhook.h"

class CKeyboardWindows :
	public CKeyboardHookBase
{
public:
	virtual ~CKeyboardWindows(void);
	LRESULT KeyboardProc( int nCode, WPARAM wParam, LPARAM lParam );
	static CKeyboardWindows* getInstance( CKeyboard* pParent );
protected:
	CKeyboard* m_parent;
	CKeyboardWindows(void);// not defined
	HHOOK m_KeyboardHook;
private:
	CKeyboardWindows( CKeyboard* pParent );
	static bool m_instanceFlag;
    static CKeyboardWindows* m_single;
};

#endif // #ifndef CKEYBOARDHOOK_WINDOWS_HPP