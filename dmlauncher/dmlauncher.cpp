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

/*
Darkmod Launcher. Launches doom3. Builds command-line args
from currentfm.txt and dmargs.txt.
*/
#include "Launcher.h"

int main(int argc, char* argv[])
{
	// Instantiate a new Launcher class
	Launcher launcher(argc, argv);

	return launcher.Launch() ? EXIT_SUCCESS : EXIT_FAILURE;
}
