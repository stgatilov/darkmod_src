#pragma once

/*
*	Mouse input data
*/
#ifndef TDM_MOUSEDATAS
#define TDM_MOUSEDATAS

/*
*  All the ifdefs make the code harder to read and downright ugly
*  So I am doing what I should have done the first time around and moving the
*  Functionality to separate class's
*  We dont even include the header here so changes to the mouse class dont require
*  us to recompile EVERYTHING. We declare it as a pointer thus we allocate
*  and delete in the constructor/destructor of course.
*  Not only are we decoupling the mouse header from this header but we are also
*  Isolating the OS specifics even further to another header and cpp file
*  Then we just add new classes for each required OS we support.
*  This file is completely OS agnostic
*/

typedef enum {
	TDM_NONE = 0, // invalid
	TDM_LBUTTONDOWN,
	TDM_LBUTTONUP,
	TDM_RBUTTONDOWN,
	TDM_RBUTTONUP,
	TDM_MBUTTONDOWN,
	TDM_MBUTTONUP
} tMouseDef;
/*
typedef struct MouseData_s
{
	int Action;
	long X;
	long Y;

	MouseData_s( MouseData_s& Clone )
	{
		*this = Clone;
	}
	MouseData_s():Action(TDM_NONE),X(0),Y(0)
	{
	}
	MouseData_s& operator=( MouseData_s& Clone )
	{
		X = Clone.X;
		Y = Clone.Y;
		Action = Clone.Action;
		return *this;
	}
} MouseData_t;
*/
#endif // #ifndef TDM_MOUSEDATAS