
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_STUBENTRY_HPP
#define INCLUDE_RCF_STUBENTRY_HPP

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include <RCF/ThreadLibrary.hpp>

namespace RCF {

    class I_RcfClient;
    class StubEntry;
    typedef boost::shared_ptr<I_RcfClient> RcfClientPtr;
    typedef boost::shared_ptr<StubEntry> StubEntryPtr;

    class StubEntry : boost::noncopyable
    {
    public:
        StubEntry(const RcfClientPtr &rcfClientPtr);
        RcfClientPtr getRcfClientPtr() const;
        void touch();
        unsigned int getElapsedTimeS() const;

    private:
        RcfClientPtr    mRcfClientPtr;
        mutable Mutex   mMutex;
        unsigned int    mTimeStamp;
    };

} // namespace RCF

#endif // ! INCLUDE_RCF_STUBENTRY_HPP
