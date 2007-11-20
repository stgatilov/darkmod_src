
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_THREADLOCALDATA_HPP
#define INCLUDE_RCF_THREADLOCALDATA_HPP

#include <boost/shared_ptr.hpp>

namespace RCF {

    class ObjectCache;
    class ClientStub;
    class I_Session;
    class RcfSession;
    class ThreadInfo;

    typedef boost::shared_ptr<ClientStub> ClientStubPtr;
    typedef boost::shared_ptr<I_Session> SessionPtr;
    typedef boost::shared_ptr<RcfSession> RcfSessionPtr;
    typedef boost::shared_ptr<ThreadInfo> ThreadInfoPtr;

    ObjectCache &   getThreadLocalObjectCache();
    ClientStubPtr   getCurrentClientStubPtr();
    void            setCurrentClientStubPtr(ClientStubPtr clientStubPtr);
    RcfSessionPtr   getCurrentRcfSessionPtr();
    void            setCurrentRcfSessionPtr(const RcfSessionPtr &rcfSessionPtr = RcfSessionPtr());
    ThreadInfoPtr   getThreadInfoPtr();
    void            setThreadInfoPtr(const ThreadInfoPtr &threadInfoPtr);
    SessionPtr      getCurrentUdpSessionPtr();
    void            setCurrentUdpSessionPtr(SessionPtr sessionPtr);

} // namespace RCF

#endif // ! INCLUDE_RCF_THREADLOCALDATA_HPP
