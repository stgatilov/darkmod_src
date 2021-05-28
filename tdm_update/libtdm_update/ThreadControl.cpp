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
#include "ThreadControl.h"

namespace tdm
{

ThreadControl::ThreadIdSet ThreadControl::_interruptedThreadIds;
std::mutex ThreadControl::_interruptionSetMutex;

bool ThreadControl::ThreadHasBeenInterrupted(const std::thread::id& id)
{
    std::lock_guard<std::mutex> lockGuard(_interruptionSetMutex);

    return _interruptedThreadIds.find(id) != _interruptedThreadIds.end();
}

void ThreadControl::InterruptThread(const std::thread::id& id)
{
    std::lock_guard<std::mutex> lockGuard(_interruptionSetMutex);

    // Attempt to insert irrespective of whether it's already in the set
    _interruptedThreadIds.insert(id);
}

void ThreadControl::InterruptionPoint()
{
    std::lock_guard<std::mutex> lockGuard(_interruptionSetMutex);

    if (_interruptedThreadIds.find(std::this_thread::get_id()) != _interruptedThreadIds.end())
    {
        throw ThreadInterruptedException();
    }
}

}
