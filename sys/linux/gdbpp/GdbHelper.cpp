/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include <stdint.h>

// this is an artificial type which contains a 64-bit integer inside
// it is used by 'embed_printer_for_value' feature of GDB pretty-printers
struct GdbHelper {
    int64_t index;
};

// we must ensure that the type is not stripped from the binary
// global variable seems to be enough for that
GdbHelper g_gdb_helper = {17ll};
