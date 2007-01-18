#ifndef CMOUSEHOOK_HPP
#define CMOUSEHOOK_HPP

#pragma once
#include "../idlib/precompiled.h"
#include "../darkmod/MouseData.h"

#pragma Message ("Mouse Defines. Linux and mac ports need to be added here.")

#ifdef MOUSE_TESTING
const char* MouseGetActionString( const int ActionCode );
#endif // #ifdef MOUSE_TESTING

class CMouseHook;
class CHookBase // Base class for all hook classes
{
public:
	CHookBase( CMouseHook* pParent = NULL ){}
	virtual ~CHookBase() {}
};

class CMouseHook
{
public:
	virtual ~CMouseHook(void);
	static CMouseHook* getInstance();
//	const MouseData_t* GetPreviousMessage() const;
//	const MouseData_t* GetCurrentMessage() const;
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
	CHookBase* m_hook;
	
	// singleton stuff goes here
	CMouseHook(void);
    static bool m_instanceFlag;
    static CMouseHook* m_single;
};

#endif // #ifndef CMOUSEHOOK_HPP
