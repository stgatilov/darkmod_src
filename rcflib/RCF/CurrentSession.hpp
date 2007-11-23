
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_CURRENTSESSION_HPP
#define INCLUDE_RCF_CURRENTSESSION_HPP

#include <boost/shared_ptr.hpp>

#include <RCF/RcfSession.hpp>
#include <RCF/ThreadLibrary.hpp>

namespace RCF {

    // TODO: these should not be public.
    //SessionPtr getCurrentSessionPtr();
    //void setCurrentRcfSessionPtr(const RcfSessionPtr &sessionPtr = RcfSessionPtr());

    /// Obtains the session object of the currently executing remote call (server side).
    /// \return Pointer to current session. Null if no remote call is executing.
    //RcfSessionPtr getCurrentRcfSessionPtr();

    class SetCurrentSessionGuard
    {
    public:
        SetCurrentSessionGuard(const RcfSessionPtr &sessionPtr)
        {
            setCurrentRcfSessionPtr(sessionPtr);
        }

        SetCurrentSessionGuard(const SessionPtr &sessionPtr)
        {
            setCurrentRcfSessionPtr( boost::dynamic_pointer_cast<RcfSession>(sessionPtr) );
        }

        ~SetCurrentSessionGuard()
        {
            setCurrentRcfSessionPtr();
        }
    };

    inline SessionPtr getCurrentSessionPtr()
    {
        return SessionPtr(getCurrentRcfSessionPtr());
    }

} // namespace RCF

#endif // ! INCLUDE_RCF_CURRENTSESSION_HPP
