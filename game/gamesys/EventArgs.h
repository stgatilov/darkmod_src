/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
 $Revision$ (Revision of last commit) 
 $Date$ (Date of last commit)
 $Author$ (Author of last commit)
 
******************************************************************************/
#ifndef EVENT_ARGS_H_
#define EVENT_ARGS_H_

struct EventArg
{
	char type;
	const char* name;
	const char* desc;
};

/**
 * greebo: An object encapsulating the format specifier for idEventDef, 
 * including the event argument order, their names and description.
 *
 * idEventDef allow for up to 8 arguments, and so are the constructors of this class.
 */
class EventArgs :
	public std::vector<EventArg>
{
public:
	EventArgs();															// 0 args
	
	EventArgs(char argType1, const char* argName1, const char* argDesc1);	// 1 arg

	EventArgs(char argType1, const char* argName1, const char* argDesc1,
			  char argType2, const char* argName2, const char* argDesc2);	// 2 args

	EventArgs(char argType1, const char* argName1, const char* argDesc1,
			  char argType2, const char* argName2, const char* argDesc2,
			  char argType3, const char* argName3, const char* argDesc3);	// 3 args

	EventArgs(char argType1, const char* argName1, const char* argDesc1,	// 4 args
			  char argType2, const char* argName2, const char* argDesc2,
			  char argType3, const char* argName3, const char* argDesc3,
			  char argType4, const char* argName4, const char* argDesc4);

	EventArgs(char argType1, const char* argName1, const char* argDesc1,	// 5 args
			  char argType2, const char* argName2, const char* argDesc2,
			  char argType3, const char* argName3, const char* argDesc3,
			  char argType4, const char* argName4, const char* argDesc4,
			  char argType5, const char* argName5, const char* argDesc5);

	EventArgs(char argType1, const char* argName1, const char* argDesc1,	// 6 args
			  char argType2, const char* argName2, const char* argDesc2,
			  char argType3, const char* argName3, const char* argDesc3,
			  char argType4, const char* argName4, const char* argDesc4,
			  char argType5, const char* argName5, const char* argDesc5,
			  char argType6, const char* argName6, const char* argDesc6);	

	EventArgs(char argType1, const char* argName1, const char* argDesc1,	// 7 args
			  char argType2, const char* argName2, const char* argDesc2,
			  char argType3, const char* argName3, const char* argDesc3,
			  char argType4, const char* argName4, const char* argDesc4,
			  char argType5, const char* argName5, const char* argDesc5,
			  char argType6, const char* argName6, const char* argDesc6,
			  char argType7, const char* argName7, const char* argDesc7);

	EventArgs(char argType1, const char* argName1, const char* argDesc1,	// 8 args
			  char argType2, const char* argName2, const char* argDesc2,
			  char argType3, const char* argName3, const char* argDesc3,
			  char argType4, const char* argName4, const char* argDesc4,
			  char argType5, const char* argName5, const char* argDesc5,
			  char argType6, const char* argName6, const char* argDesc6,
			  char argType7, const char* argName7, const char* argDesc7,
			  char argType8, const char* argName8, const char* argDesc8);
};

inline EventArgs::EventArgs()
{}
	
inline EventArgs::EventArgs(char argType1, const char* argName1, const char* argDesc1) :
	std::vector<EventArg>(1)
{
	(*this)[0].type = argType1; (*this)[0].name = argName1; (*this)[0].desc = argDesc1;
}

inline EventArgs::EventArgs(char argType1, const char* argName1, const char* argDesc1,
			  char argType2, const char* argName2, const char* argDesc2) :
	std::vector<EventArg>(2)
{
	(*this)[0].type = argType1; (*this)[0].name = argName1; (*this)[0].desc = argDesc1;
	(*this)[1].type = argType2; (*this)[1].name = argName2; (*this)[1].desc = argDesc2;
}

inline EventArgs::EventArgs(char argType1, const char* argName1, const char* argDesc1,
			  char argType2, const char* argName2, const char* argDesc2,
			  char argType3, const char* argName3, const char* argDesc3) :
	std::vector<EventArg>(3)
{
	(*this)[0].type = argType1; (*this)[0].name = argName1; (*this)[0].desc = argDesc1;
	(*this)[1].type = argType2; (*this)[1].name = argName2; (*this)[1].desc = argDesc2;
	(*this)[2].type = argType3; (*this)[2].name = argName3; (*this)[2].desc = argDesc3;
}

inline EventArgs::EventArgs(char argType1, const char* argName1, const char* argDesc1,
			  char argType2, const char* argName2, const char* argDesc2,
			  char argType3, const char* argName3, const char* argDesc3,
			  char argType4, const char* argName4, const char* argDesc4) :
	std::vector<EventArg>(4)
{
	(*this)[0].type = argType1; (*this)[0].name = argName1; (*this)[0].desc = argDesc1;
	(*this)[1].type = argType2; (*this)[1].name = argName2; (*this)[1].desc = argDesc2;
	(*this)[2].type = argType3; (*this)[2].name = argName3; (*this)[2].desc = argDesc3;
	(*this)[3].type = argType4; (*this)[3].name = argName4; (*this)[3].desc = argDesc4;
}

inline EventArgs::EventArgs(char argType1, const char* argName1, const char* argDesc1,	// 5 args
			  char argType2, const char* argName2, const char* argDesc2,
			  char argType3, const char* argName3, const char* argDesc3,
			  char argType4, const char* argName4, const char* argDesc4,
			  char argType5, const char* argName5, const char* argDesc5) :
	std::vector<EventArg>(5)
{
	(*this)[0].type = argType1; (*this)[0].name = argName1; (*this)[0].desc = argDesc1;
	(*this)[1].type = argType2; (*this)[1].name = argName2; (*this)[1].desc = argDesc2;
	(*this)[2].type = argType3; (*this)[2].name = argName3; (*this)[2].desc = argDesc3;
	(*this)[3].type = argType4; (*this)[3].name = argName4; (*this)[3].desc = argDesc4;
	(*this)[4].type = argType5; (*this)[4].name = argName5; (*this)[4].desc = argDesc5;
}	

inline EventArgs::EventArgs(char argType1, const char* argName1, const char* argDesc1,	// 6 args
			  char argType2, const char* argName2, const char* argDesc2,
			  char argType3, const char* argName3, const char* argDesc3,
			  char argType4, const char* argName4, const char* argDesc4,
			  char argType5, const char* argName5, const char* argDesc5,
			  char argType6, const char* argName6, const char* argDesc6) :
	std::vector<EventArg>(6)
{
	(*this)[0].type = argType1; (*this)[0].name = argName1; (*this)[0].desc = argDesc1;
	(*this)[1].type = argType2; (*this)[1].name = argName2; (*this)[1].desc = argDesc2;
	(*this)[2].type = argType3; (*this)[2].name = argName3; (*this)[2].desc = argDesc3;
	(*this)[3].type = argType4; (*this)[3].name = argName4; (*this)[3].desc = argDesc4;
	(*this)[4].type = argType5; (*this)[4].name = argName5; (*this)[4].desc = argDesc5;
	(*this)[5].type = argType6; (*this)[5].name = argName6; (*this)[5].desc = argDesc6;
}	

inline EventArgs::EventArgs(char argType1, const char* argName1, const char* argDesc1,	// 7 args
			  char argType2, const char* argName2, const char* argDesc2,
			  char argType3, const char* argName3, const char* argDesc3,
			  char argType4, const char* argName4, const char* argDesc4,
			  char argType5, const char* argName5, const char* argDesc5,
			  char argType6, const char* argName6, const char* argDesc6,
			  char argType7, const char* argName7, const char* argDesc7) :
	std::vector<EventArg>(7)
{
	(*this)[0].type = argType1; (*this)[0].name = argName1; (*this)[0].desc = argDesc1;
	(*this)[1].type = argType2; (*this)[1].name = argName2; (*this)[1].desc = argDesc2;
	(*this)[2].type = argType3; (*this)[2].name = argName3; (*this)[2].desc = argDesc3;
	(*this)[3].type = argType4; (*this)[3].name = argName4; (*this)[3].desc = argDesc4;
	(*this)[4].type = argType5; (*this)[4].name = argName5; (*this)[4].desc = argDesc5;
	(*this)[5].type = argType6; (*this)[5].name = argName6; (*this)[5].desc = argDesc6;
	(*this)[6].type = argType7; (*this)[6].name = argName7; (*this)[6].desc = argDesc7;
}

inline EventArgs::EventArgs(char argType1, const char* argName1, const char* argDesc1,	// 8 args
			  char argType2, const char* argName2, const char* argDesc2,
			  char argType3, const char* argName3, const char* argDesc3,
			  char argType4, const char* argName4, const char* argDesc4,
			  char argType5, const char* argName5, const char* argDesc5,
			  char argType6, const char* argName6, const char* argDesc6,
			  char argType7, const char* argName7, const char* argDesc7,
			  char argType8, const char* argName8, const char* argDesc8) :
	std::vector<EventArg>(8)
{
	(*this)[0].type = argType1; (*this)[0].name = argName1; (*this)[0].desc = argDesc1;
	(*this)[1].type = argType2; (*this)[1].name = argName2; (*this)[1].desc = argDesc2;
	(*this)[2].type = argType3; (*this)[2].name = argName3; (*this)[2].desc = argDesc3;
	(*this)[3].type = argType4; (*this)[3].name = argName4; (*this)[3].desc = argDesc4;
	(*this)[4].type = argType5; (*this)[4].name = argName5; (*this)[4].desc = argDesc5;
	(*this)[5].type = argType6; (*this)[5].name = argName6; (*this)[5].desc = argDesc6;
	(*this)[6].type = argType7; (*this)[6].name = argName7; (*this)[6].desc = argDesc7;
	(*this)[7].type = argType8; (*this)[7].name = argName8; (*this)[7].desc = argDesc8;
}

#endif
