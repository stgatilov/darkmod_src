#ifndef CMOUSEHOOK_HPP
#define CMOUSEHOOK_HPP

#pragma once
#include "../idlib/precompiled.h"
#include "../darkmod/MouseData.h"

#define MOUSETEST  1 // undefine this to remove the status text

class CMouseHook;
class CMouseHookBase // Base class for all Mouse hook classes
{
public:
	CMouseHookBase( CMouseHook* pParent = NULL ){}
	virtual ~CMouseHookBase() {}
};

class CMouseHook
{
public:
	virtual ~CMouseHook(void);
	static CMouseHook* getInstance();
	bool GetLeftStatus() const;
	bool GetRightStatus() const;
	bool GetMiddleStatus() const;
	void SetMiddleStatus(bool Pressed);
	void SetRightStatus(bool Pressed);
	void SetLeftStatus(bool Pressed);
private:

	bool m_Mouse_LBPressed; // true if currently pressed false otherwise
	bool m_Mouse_RBPressed; // default value is false on init
	bool m_Mouse_MBPressed;

	// this is a base class* for the OS dependant hook class
	CMouseHookBase* m_hook;
	
	// singleton stuff goes here
	CMouseHook(void);
    static bool m_instanceFlag;
    static CMouseHook* m_single;
};

#endif // #ifndef CMOUSEHOOK_HPP
