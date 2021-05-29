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

#pragma once

#include <set>
#include <thread>
#include <mutex>

namespace tdm
{

// Thrown to the calling thread if the thread was interrupted
class ThreadInterruptedException
{};

// Controller class which keeps track of the thread interruption state
class ThreadControl
{
private:
    // The set of thread ID hashes which have been interrupted
    typedef std::set<std::thread::id> ThreadIdSet;
    static ThreadIdSet _interruptedThreadIds;

    // The mutex which needs to be locked before the set can be accessed
    static std::mutex _interruptionSetMutex;

public:
    // Returns true if the given thread has been asked to interrupt
    static bool ThreadHasBeenInterrupted(const std::thread::id& id);

    // Marks a thread for interruption
    static void InterruptThread(const std::thread::id& id);

    // An executing thread should call this in regular intervals
    // to check whether it was asked to interrupt
    static void InterruptionPoint();
};

} // namespace
