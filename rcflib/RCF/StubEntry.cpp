
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/StubEntry.hpp>
#include <RCF/Tools.hpp>

#include <time.h>

namespace RCF {

    // time in s since ca 1970, may fail after year 2038
    inline unsigned int getCurrentTimeS()
    {
        return static_cast<unsigned int>(time(NULL));
    }

    StubEntry::StubEntry(const RcfClientPtr &rcfClientPtr) :
        mRcfClientPtr(rcfClientPtr),
        mTimeStamp(getCurrentTimeS())
    {
        RCF_ASSERT(rcfClientPtr);
    }

    RcfClientPtr StubEntry::getRcfClientPtr() const
    {
        return mRcfClientPtr;
    }

    void StubEntry::touch()
    {
        // TODO: if we need sync at all for this, then InterlockedExchange etc
        // would be better

        Lock lock(mMutex);
        mTimeStamp = getCurrentTimeS();
    }

    unsigned int StubEntry::getElapsedTimeS() const
    {
        Lock lock(mMutex);
        if (mTimeStamp == 0)
        {
            return 0;
        }
        else
        {
            unsigned int currentTimeS = getCurrentTimeS();
            return currentTimeS > mTimeStamp ?
                currentTimeS - mTimeStamp :
                mTimeStamp - currentTimeS;
        }
    }

} // namespace RCF
