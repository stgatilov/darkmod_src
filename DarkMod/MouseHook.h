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
class CHookBase // Base class for all hook classes - Pure
{
public:
	friend class CMouseHook;
	CHookBase( CMouseHook* pParent ):m_Mouse_LBPressed(false),m_Mouse_RBPressed(false),m_Mouse_MBPressed(false){}
	virtual ~CHookBase() {}
protected:
//	MouseData_t m_MouseDataPrevious; // everytime we get a new mouse message save old one here - Extraneous ?
//	MouseData_t m_MouseDataCurrent;  // most recent mouse message
	bool m_Mouse_LBPressed; // true if currently pressed false otherwise
	bool m_Mouse_RBPressed; // default value is false on init
	bool m_Mouse_MBPressed;
};

class CMouseHook
{
public:
	virtual ~CMouseHook(void);
	static CMouseHook* getInstance();
//	const MouseData_t* GetPreviousMessage() const;
//	const MouseData_t* GetCurrentMessage() const;
	bool LeftButtonPressed() const;
	bool RightButtonPressed() const;
	bool MiddleButtonPressed() const;
private:
	CHookBase* m_hook;
	CMouseHook(void);
    static bool m_instanceFlag;
    static CMouseHook* m_single;
};

#endif // #ifndef CMOUSEHOOK_HPP
