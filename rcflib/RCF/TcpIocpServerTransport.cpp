
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/TcpIocpServerTransport.hpp>

#include <RCF/CurrentSession.hpp>
#include <RCF/MethodInvocation.hpp>
#include <RCF/RcfServer.hpp>
#include <RCF/TcpClientTransport.hpp>
#include <RCF/TcpEndpoint.hpp>
#include <RCF/Tools.hpp>

namespace RCF {

    namespace TcpIocp {

    Iocp::Iocp(int nMaxConcurrency)
    {
        m_hIOCP = NULL;
        if (nMaxConcurrency != -1)
        {
            Create(nMaxConcurrency);
        }
    }

    Iocp::~Iocp()
    {
        RCF_DTOR_BEGIN
            if (m_hIOCP != NULL)
            {
                int ret = CloseHandle(m_hIOCP);
                int err = Platform::OS::BsdSockets::GetLastError();
                RCF_VERIFY(
                    ret,
                    Exception(
                        RcfError_Socket,
                        err,
                        RcfSubsystem_Os,
                        "CloseHande() failed"))
                    (m_hIOCP);
            }
        RCF_DTOR_END
    }

    BOOL Iocp::Create(int nMaxConcurrency)
    {
        m_hIOCP = CreateIoCompletionPort(
            INVALID_HANDLE_VALUE,
            NULL,
            0,
            nMaxConcurrency);

        int err = Platform::OS::BsdSockets::GetLastError();
        RCF_VERIFY(
            m_hIOCP != NULL,
            Exception(
                RcfError_Socket,
                err,
                RcfSubsystem_Os,
                "CreateIoCompletionPort() failed"));

        return(m_hIOCP != NULL);
    }

    BOOL Iocp::AssociateDevice(HANDLE hDevice, ULONG_PTR CompKey)
    {
        BOOL fOk =
            (CreateIoCompletionPort(hDevice, m_hIOCP, CompKey, 0) == m_hIOCP);

        int err = Platform::OS::BsdSockets::GetLastError();
        RCF_VERIFY(
            fOk,
            Exception(
                RcfError_Socket,
                err,
                RcfSubsystem_Os,
                "CreateIoCompletionPort() failed"))
            (hDevice)(static_cast<__int64>(CompKey));

        return fOk;
    }

    BOOL Iocp::AssociateSocket(SOCKET hSocket, ULONG_PTR CompKey)
    {
        return AssociateDevice((HANDLE) hSocket, CompKey);
    }

    BOOL Iocp::PostStatus(ULONG_PTR CompKey, DWORD dwNumBytes, OVERLAPPED* po)
    {
        BOOL fOk = PostQueuedCompletionStatus(m_hIOCP, dwNumBytes, CompKey, po);
        RCF_ASSERT(fOk);
        return(fOk);
    }

    BOOL Iocp::GetStatus(
        ULONG_PTR* pCompKey,
        PDWORD pdwNumBytes,
        OVERLAPPED** ppo,
        DWORD dwMilliseconds)
    {
        return GetQueuedCompletionStatus(
            m_hIOCP,
            pdwNumBytes,
            pCompKey,
            ppo,
            dwMilliseconds);
    }

    void resetSessionStatePtr(SessionStatePtr &sessionStatePtr)
    {
        sessionStatePtr.reset();
    }

    void SessionState::wsaRecv(
        const ByteBuffer &byteBuffer,
        std::size_t bufferLen)
    {

        WSAOVERLAPPED *pOverlapped = static_cast<WSAOVERLAPPED *>(this);
        RCF_ASSERT(pOverlapped);

        if (byteBuffer.getLength() == 0)
        {
            std::vector<char> &vec = getUniqueReadBufferSecondary();
            vec.resize(bufferLen);
            mTempByteBuffer = getReadByteBufferSecondary();
        }
        else
        {
            mTempByteBuffer = ByteBuffer(byteBuffer, 0, bufferLen);
        }

        RCF_ASSERT(
            bufferLen <= mTempByteBuffer.getLength())
            (bufferLen)(mTempByteBuffer.getLength());

        bufferLen = RCF_MIN(mTransport.getMaxSendRecvSize(), bufferLen);
        WSABUF wsabuf = {0};
        wsabuf.buf = mTempByteBuffer.getPtr();
        wsabuf.len = static_cast<u_long>(bufferLen);
        DWORD dwReceived = 0;
        DWORD dwFlags = 0;
        int ret = -1;
        mError = 0;
        mPostState = Reading;

        // set self-reference
        RCF_ASSERT(!mThisPtr.get());
        mThisPtr = mWeakThisPtr.lock();
        RCF_ASSERT(mThisPtr.get());

        using namespace boost::multi_index::detail;
        scope_guard clearSelfReferenceGuard =
            make_guard(resetSessionStatePtr, boost::ref(mThisPtr));

        RCF2_TRACE("calling WSARecv()")(wsabuf.len);

        if (mSynchronized)
        {
            Lock lock(*mMutexPtr);

            if (mHasBeenClosed)
            {
                return;
            }

            ret = WSARecv(
                mFd,
                &wsabuf,
                1,
                &dwReceived,
                &dwFlags,
                pOverlapped,
                NULL);

            mError = WSAGetLastError();
        }
        else
        {
            ret = WSARecv(
                mFd,
                &wsabuf,
                1,
                &dwReceived,
                &dwFlags,
                pOverlapped,
                NULL);

            mError = WSAGetLastError();
        }

        RCF_ASSERT(ret == -1 || ret == 0);
        if (mError == S_OK || mError == WSA_IO_PENDING)
        {
            mError = 0;
            clearSelfReferenceGuard.dismiss();
        }
    }

    void SessionState::wsaSend(
        const std::vector<ByteBuffer> &byteBuffers)
    {
        WSAOVERLAPPED *pOverlapped = static_cast<WSAOVERLAPPED *>(this);
        RCF_ASSERT(pOverlapped);

        std::size_t bytesAdded = 0;

        mWsabufs.resize(0);
        for (std::size_t i=0; i<byteBuffers.size(); ++i)
        {
            std::size_t bytesToAdd = RCF_MIN(
                byteBuffers[i].getLength(),
                mTransport.getMaxSendRecvSize() - bytesAdded);

            if (bytesToAdd > 0)
            {
                WSABUF wsabuf = {0};
                wsabuf.buf = byteBuffers[i].getPtr();
                wsabuf.len = static_cast<u_long>(bytesToAdd);
                mWsabufs.push_back(wsabuf);
                bytesAdded += bytesToAdd;
            }
        }

        DWORD dwSent = 0;
        DWORD dwFlags = 0;
        int ret = -1;
        mError = 0;
        mPostState = Writing;

        // set self-reference
        RCF_ASSERT(!mThisPtr.get());
        mThisPtr = mWeakThisPtr.lock();
        RCF_ASSERT(mThisPtr.get());

        using namespace boost::multi_index::detail;
        scope_guard clearSelfReferenceGuard =
            make_guard(resetSessionStatePtr, boost::ref(mThisPtr));

        RCF2_TRACE("calling WSASend()")(RCF::lengthByteBuffers(byteBuffers))(bytesAdded);

        if (mSynchronized)
        {
            Lock lock(*mMutexPtr);

            if (mHasBeenClosed)
            {
                return;
            }

            ret = WSASend(
                mFd,
                &mWsabufs[0],
                static_cast<DWORD>(mWsabufs.size()),
                &dwSent,
                dwFlags,
                pOverlapped,
                NULL);

            mError = WSAGetLastError();
        }
        else
        {
            ret = WSASend(
                mFd,
                &mWsabufs[0],
                static_cast<DWORD>(mWsabufs.size()),
                &dwSent,
                dwFlags,
                pOverlapped,
                NULL);

            mError = WSAGetLastError();
        }

        RCF_ASSERT(ret == -1 || ret == 0);
        if (mError == S_OK || mError == WSA_IO_PENDING)
        {
            clearSelfReferenceGuard.dismiss();
            mError = 0;
        }
    }

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4355 )  // warning C4355: 'this' : used in base member initializer list
#endif

    SessionState::SessionState(
        ServerTransport &transport,
        Fd fd) :
            mState(Accepting),
            mPostState(Reading),
            mReadBufferRemaining(RCF_DEFAULT_INIT),
            mWriteBufferRemaining(RCF_DEFAULT_INIT),
            mFd(fd),
            mError(RCF_DEFAULT_INIT),
            mOwnFd(true),
            mCloseAfterWrite(RCF_DEFAULT_INIT),
            mTransport(transport),
            mReflected(RCF_DEFAULT_INIT),
            mSynchronized(RCF_DEFAULT_INIT),
            mHasBeenClosed(RCF_DEFAULT_INIT),
            mMutexPtr()
    {
        // blank the OVERLAPPED structure
        clearOverlapped();
    }

#ifdef _MSC_VER
#pragma warning( pop )
#endif

    // Since we only handle SessionState's through shared_ptr's, we don't need to synchronize access to ownFd and zombie.
    SessionState::~SessionState()
    {
        RCF_DTOR_BEGIN
            RCF2_TRACE("")(mSessionPtr.get())(mOwnFd)(mFd);

            // adjust number of queued accepts, if appropriate
            if (mState == SessionState::Accepting)
            {
                // TODO: this code should be in the transport
                InterlockedDecrement( (LONG *) &mTransport.mQueuedAccepts);
                if (mTransport.mQueuedAccepts < mTransport.mQueuedAcceptsThreshold)
                {
                    mTransport.mQueuedAcceptsCondition.notify_one();
                }
            }

            // close the reflected session, if appropriate
            if (mReflected)
            {
                mTransport.closeSession(mReflectionSessionStateWeakPtr);
            }

            // close the socket, if appropriate
            RCF_ASSERT(mFd != -1);
            if (mOwnFd && !mHasBeenClosed)
            {
                int ret = Platform::OS::BsdSockets::closesocket(mFd);
                int err = Platform::OS::BsdSockets::GetLastError();

                RCF_VERIFY(
                    ret == 0,
                    Exception(
                        RcfError_SocketClose,
                        err,
                        RcfSubsystem_Os,
                        "closesocket() failed"))
                    (mFd);

                mHasBeenClosed = true;
            }
        RCF_DTOR_END
    }

    void SessionState::setTransportFilters(
        const std::vector<FilterPtr> &filters)
    {
        mTransportFilters.clear();
        if (!filters.empty())
        {
            mTransportFilters.push_back(
                FilterPtr(new FilterProxy(*this, *filters.front(), true)));

            std::copy(
                filters.begin(),
                filters.end(),
                std::back_inserter(mTransportFilters));

            mTransportFilters.push_back(
                FilterPtr(new FilterProxy(*this, *filters.back(), false)));

            RCF::connectFilters(mTransportFilters);
        }
    }

    const std::vector<FilterPtr> &SessionState::getTransportFilters()
    {
        return mTransportFilters;
    }

    int SessionState::read(
        ByteBuffer &byteBuffer,
        std::size_t bufferLen)
    {
        mTransportFilters.empty() ?
            wsaRecv(byteBuffer, bufferLen) :
            mTransportFilters.front()->read(byteBuffer, bufferLen);

        //return mError ? -1 : 0;

        // for symmetry with write()
        return (mError == 0 || mError == WSA_IO_PENDING) ? 0 : -1;
    }

    int SessionState::write(
        const std::vector<ByteBuffer> &byteBuffers)
    {
        mTransportFilters.empty() ?
            wsaSend(byteBuffers) :
            mTransportFilters.front()->write(byteBuffers);

        //return mError ? -1 : 0;

        // TODO: how in the world can mError ever be WSA_IO_PENDING???
        // Occurred when running trim620test/wgs.
        return (mError == 0 || mError == WSA_IO_PENDING) ? 0 : -1;
    }

    void SessionState::onReadWriteCompleted(
        std::size_t bytesTransferred,
        int error)
    {
        if (mReflected)
        {
            reflect(bytesTransferred);
        }
        else
        {
            RCF_ASSERT(mPostState == Reading || mPostState == Writing)(mPostState);

            if (mPostState == Reading)
            {
                RCF2_TRACE("read completed")(bytesTransferred);

                ByteBuffer byteBuffer(mTempByteBuffer.release(), 0, bytesTransferred);
                mTransportFilters.empty() ?
                    mTransport.onReadWriteCompleted(mWeakThisPtr, bytesTransferred, error):
                    mTransportFilters.back()->onReadCompleted(byteBuffer, error);
            }
            else if (mPostState == Writing)
            {
                RCF2_TRACE("write completed")(bytesTransferred);

                mTransportFilters.empty() ?
                    mTransport.onReadWriteCompleted(mWeakThisPtr, bytesTransferred, error):
                    mTransportFilters.back()->onWriteCompleted(bytesTransferred, error);
            }
            else
            {
                RCF_ASSERT(0);
            }
        }
    }

    void SessionState::clearOverlapped()
    {
        memset(static_cast<OVERLAPPED *>(this), 0, sizeof(OVERLAPPED));
    }

    std::vector<char> &SessionState::getReadBuffer()
    {
        if (!mReadBufferPtr)
        {
            mReadBufferPtr.reset( new std::vector<char>() );
        }
        return *mReadBufferPtr;
    }

    std::vector<char> &SessionState::getUniqueReadBuffer()
    {
        if (!mReadBufferPtr || !mReadBufferPtr.unique())
        {
            mReadBufferPtr.reset( new std::vector<char>() );
        }
        return *mReadBufferPtr;
    }

    ByteBuffer SessionState::getReadByteBuffer() const
    {
        return ByteBuffer(
            &(*mReadBufferPtr)[0],
            (*mReadBufferPtr).size(),
            mReadBufferPtr);
    }

    std::vector<char> &SessionState::getReadBufferSecondary()
    {
        if (!mReadBufferSecondaryPtr)
        {
            mReadBufferSecondaryPtr.reset( new std::vector<char>() );
        }
        return *mReadBufferSecondaryPtr;
    }

    std::vector<char> &SessionState::getUniqueReadBufferSecondary()
    {
        if (!mReadBufferSecondaryPtr || !mReadBufferSecondaryPtr.unique())
        {
            mReadBufferSecondaryPtr.reset( new std::vector<char>() );
        }
        return *mReadBufferSecondaryPtr;
    }

    ByteBuffer SessionState::getReadByteBufferSecondary() const
    {
        return ByteBuffer(
            &(*mReadBufferSecondaryPtr)[0],
            (*mReadBufferSecondaryPtr).size(),
            mReadBufferSecondaryPtr);
    }

    void SessionState::onFilteredReadCompleted(
        const ByteBuffer &byteBuffer,
        int error)
    {
        mTransport.onFilteredReadCompleted(mWeakThisPtr, byteBuffer, error);
    }

    void SessionState::onFilteredWriteCompleted(
        std::size_t bytesTransferred,
        int error)
    {
        mTransport.onFilteredWriteCompleted(mWeakThisPtr, bytesTransferred, error);
    }

    FilterProxy::FilterProxy(
        SessionState &sessionState,
        Filter &filter,
        bool top) :
            mSessionState(sessionState),
            mFilter(filter),
            mTop(top)
    {}

    void FilterProxy::read(
        const ByteBuffer &byteBuffer,
        std::size_t bytesRequested)
    {
        mTop ?
            mFilter.read(byteBuffer, bytesRequested) :
            mSessionState.wsaRecv(byteBuffer, bytesRequested);
    }

    void FilterProxy::write(
        const std::vector<ByteBuffer> &byteBuffers)
    {
        mTop ?
            mFilter.write(byteBuffers) :
            mSessionState.wsaSend(byteBuffers);
    }

    void FilterProxy::onReadCompleted(
        const ByteBuffer &byteBuffer,
        int error)
    {
        mTop ?
            mSessionState.onFilteredReadCompleted(byteBuffer, error) :
            mFilter.onReadCompleted(byteBuffer, error);
    }

    void FilterProxy::onWriteCompleted(
        std::size_t bytesTransferred,
        int error)
    {
        mTop ?
            mSessionState.onFilteredWriteCompleted(bytesTransferred, error) :
            mFilter.onWriteCompleted(bytesTransferred, error);
    }

    Proactor::Proactor(
        ServerTransport &transport,
        const SessionStatePtr &sessionStatePtr) :
            transport(transport),
            sessionStatePtr(sessionStatePtr)
    {}

    void Proactor::postRead()
    {
        transport.postRead(sessionStatePtr.lock());
    }

    void Proactor::postWrite(
        const std::vector<ByteBuffer> &byteBuffers)
    {
        transport.postWrite(sessionStatePtr.lock(), byteBuffers);
    }

    void Proactor::postClose()
    {
        transport.closeSession(sessionStatePtr);
    }

    ByteBuffer Proactor::getReadByteBuffer()
    {
        return sessionStatePtr.lock()->getReadByteBuffer();
    }

    I_ServerTransport &Proactor::getServerTransport()
    {
        return transport;
    }

    SessionState &Proactor::getSessionState()
    {
        return *getSessionStatePtr();
    }

    SessionStatePtr Proactor::getSessionStatePtr() const
    {
        return sessionStatePtr.lock();
    }

    const I_RemoteAddress &Proactor::getRemoteAddress()
    {
        return sessionStatePtr.lock()->mRemoteAddress;
    }

    void Proactor::setTransportFilters(
        const std::vector<FilterPtr> &filters)
    {
        sessionStatePtr.lock()->setTransportFilters(filters);
    }

    const std::vector<FilterPtr> &Proactor::getTransportFilters()
    {
        return sessionStatePtr.lock()->getTransportFilters();
    }

    ServerTransport::ServerTransport(int port) :
        mpSessionManager(RCF_DEFAULT_INIT),
        mMaxPendingConnectionCount(100),
        mMaxSendRecvSize(1024*1024*10),
        mAcceptorPort(RCF_DEFAULT_INIT),
        mPort(port),
        mStopFlag(RCF_DEFAULT_INIT),
        mOpen(RCF_DEFAULT_INIT),
        mAcceptorFd(-1),
        mIocpAutoPtr(RCF_DEFAULT_INIT),
        mQueuedAccepts(0),
        mQueuedAcceptsThreshold(10),
        mQueuedAcceptsAugment(10),
        mlpfnAcceptEx(RCF_DEFAULT_INIT),
        mlpfnGetAcceptExSockAddrs(RCF_DEFAULT_INIT)
    {
        setNetworkInterface("127.0.0.1");
    }

    ServerTransport::ServerTransport(const std::string &networkInterface, int port) :
        mpSessionManager(RCF_DEFAULT_INIT),
        mMaxPendingConnectionCount(100),
        mMaxSendRecvSize(1024*1024*10),
        mAcceptorPort(RCF_DEFAULT_INIT),
        mPort(port),
        mStopFlag(RCF_DEFAULT_INIT),
        mOpen(RCF_DEFAULT_INIT),
        mAcceptorFd(-1),
        mIocpAutoPtr(),
        mQueuedAccepts(0),
        mQueuedAcceptsThreshold(10),
        mQueuedAcceptsAugment(10),
        mlpfnAcceptEx(RCF_DEFAULT_INIT),
        mlpfnGetAcceptExSockAddrs(RCF_DEFAULT_INIT)
    {
        setNetworkInterface(networkInterface);
    }

    ServerTransportPtr ServerTransport::clone()
    {
        return ServerTransportPtr( new ServerTransport(getNetworkInterface(), mPort) );
    }

    void ServerTransport::setPort(int port)
    {
        mPort = port;
    }

    int ServerTransport::getPort() const
    {
        return mPort;
    }

    void ServerTransport::setMaxPendingConnectionCount(
        std::size_t maxPendingConnectionCount)
    {
        mMaxPendingConnectionCount = maxPendingConnectionCount;
    }

    std::size_t ServerTransport::getMaxPendingConnectionCount() const
    {
        return mMaxPendingConnectionCount;
    }

    void ServerTransport::setMaxSendRecvSize(std::size_t maxSendRecvSize)
    {
        mMaxSendRecvSize = maxSendRecvSize;
    }

    std::size_t ServerTransport::getMaxSendRecvSize() const
    {
        return mMaxSendRecvSize;
    }

    void ServerTransport::open()
    {
        RCF_ASSERT(mIocpAutoPtr.get() == NULL);
        RCF_ASSERT(mAcceptorFd == -1)(mAcceptorFd);
        RCF_ASSERT(mPort >= -1);
        RCF_ASSERT(mQueuedAccepts == 0)(mQueuedAccepts);

        // create io completion port and associate the listener socket
        // TODO: need to configure this?
        int nMaxConcurrency = 0;
        mIocpAutoPtr.reset(new Iocp());
        mIocpAutoPtr->Create(nMaxConcurrency);

        // set up a listening socket, if we have a non-negative port number (>0)
        if (mPort >= 0)
        {
            // create listener socket
            int ret = 0;
            int err = 0;
            mAcceptorFd = static_cast<int>(socket(PF_INET, SOCK_STREAM, IPPROTO_TCP));
            if (mAcceptorFd == -1)
            {
                err = Platform::OS::BsdSockets::GetLastError();
                RCF_THROW(Exception
                    (RcfError_Socket, err, RcfSubsystem_Os, "socket() failed"))
                    (mAcceptorFd);
            }

            // bind listener socket
            std::string networkInterface = getNetworkInterface();
            unsigned long ul_addr = inet_addr( networkInterface.c_str() );
            if (ul_addr == INADDR_NONE)
            {
                hostent *hostDesc = gethostbyname(networkInterface.c_str());
                if (hostDesc)
                {
                    char *szIp = ::inet_ntoa( * (in_addr*) hostDesc->h_addr_list[0]);
                    ul_addr = ::inet_addr(szIp);
                }
            }
            sockaddr_in serverAddr;
            memset(&serverAddr, 0, sizeof(serverAddr));
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_addr.s_addr = ul_addr;
            serverAddr.sin_port = htons( static_cast<u_short>(mPort) );
            ret = bind(mAcceptorFd, (struct sockaddr*) &serverAddr, sizeof(serverAddr));
            if (ret < 0)
            {
                err = Platform::OS::BsdSockets::GetLastError();
                if (err == WSAEADDRINUSE)
                {
                    RCF_THROW(Exception(
                        RcfError_PortInUse, err, RcfSubsystem_Os, "bind() failed"))
                        (mAcceptorFd)(mPort)(networkInterface)(ret);
                }
                else
                {
                    RCF_THROW(Exception(
                        RcfError_SocketBind, err, RcfSubsystem_Os, "bind() failed"))
                        (mAcceptorFd)(mPort)(networkInterface)(ret);
                }
            }

            // listen on listener socket
            ret = listen(mAcceptorFd, static_cast<int>(mMaxPendingConnectionCount));
            if (ret < 0)
            {
                err = Platform::OS::BsdSockets::GetLastError();
                RCF_THROW(Exception(
                    RcfError_Socket, err, RcfSubsystem_Os, "listen() failed"))
                    (mAcceptorFd)(ret);
            }
            RCF_ASSERT( mAcceptorFd != -1 )(mAcceptorFd);

            // retrieve the port number, if it's generated by the system
            if (mPort == 0)
            {
                sockaddr_in addr = {0};
                int nameLen = sizeof(addr);
                int ret = getsockname(mAcceptorFd, (sockaddr *) &addr, &nameLen);
                if (ret < 0)
                {
                    err = Platform::OS::BsdSockets::GetLastError();
                    RCF_THROW(Exception(
                        RcfError_Socket, err, RcfSubsystem_Os, "getsockname() failed"))
                        (mAcceptorFd)(mPort)(networkInterface)(ret);
                }
                mPort = ntohs(addr.sin_port);
            }


            // load AcceptEx() function
            GUID GuidAcceptEx = WSAID_ACCEPTEX;
            DWORD dwBytes;
            ret = WSAIoctl(
                mAcceptorFd,
                SIO_GET_EXTENSION_FUNCTION_POINTER,
                &GuidAcceptEx,
                sizeof(GuidAcceptEx),
                &mlpfnAcceptEx,
                sizeof(mlpfnAcceptEx),
                &dwBytes,
                NULL,
                NULL);
            err = Platform::OS::BsdSockets::GetLastError();
            RCF_VERIFY(
                ret == 0,
                Exception(RcfError_Socket, err, RcfSubsystem_Os,
                "WSAIoctl() failed"));

            // load GetAcceptExSockAddrs() function
            GUID GuidGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;
            ret = WSAIoctl(
                mAcceptorFd,
                SIO_GET_EXTENSION_FUNCTION_POINTER,
                &GuidGetAcceptExSockAddrs,
                sizeof(GuidGetAcceptExSockAddrs),
                &mlpfnGetAcceptExSockAddrs,
                sizeof(mlpfnGetAcceptExSockAddrs),
                &dwBytes,
                NULL,
                NULL);
            err = Platform::OS::BsdSockets::GetLastError();
            RCF_VERIFY(
                ret == 0,
                Exception(RcfError_Socket, err, RcfSubsystem_Os,
                "WsaIoctl() failed"));

            // associate listener socket to iocp
            mIocpAutoPtr->AssociateSocket( (SOCKET) mAcceptorFd, (ULONG_PTR) mAcceptorFd);
        }

    }

    void ServerTransport::close()
    {
        // delete all remaining io requests
        flushIocp();

        // delete iocp
        mIocpAutoPtr.reset();

        // close listener socket
        if (mAcceptorFd != -1)
        {
            int ret = closesocket(mAcceptorFd);
            int err = Platform::OS::BsdSockets::GetLastError();

            RCF_VERIFY(
                ret == 0,
                Exception(
                    RcfError_SocketClose,
                    err,
                    RcfSubsystem_Os,
                    "closesocket() failed"))
                (mAcceptorFd);

            mAcceptorFd = -1;
        }

        // reset queued accepts count
        mQueuedAccepts = 0;
    }

    // synchronized - no shared resources
    SessionStatePtr ServerTransport::createSession(
        int fd)
    {
        SessionStatePtr sessionStatePtr( new SessionState(*this, fd) );
        ProactorPtr proactorPtr( new Proactor(*this, sessionStatePtr) );
        SessionPtr sessionPtr = getSessionManager().createSession();
        sessionPtr->setProactorPtr(proactorPtr);
        sessionStatePtr->mSessionPtr = sessionPtr;
        sessionStatePtr->mWeakThisPtr = SessionStateWeakPtr(sessionStatePtr);
        return sessionStatePtr;
    }

    void ServerTransport::transition(const SessionStatePtr &sessionStatePtr)
    {
        switch(sessionStatePtr->mState)
        {
        case SessionState::Accepting:

            // parse the local and remote address info
            {
                SOCKADDR *pLocalAddr = NULL;
                SOCKADDR *pRemoteAddr = NULL;

                int localAddrLen = 0;
                int remoteAddrLen = 0;

                std::vector<char> &readBuffer = sessionStatePtr->getReadBuffer();
                RCF_ASSERT(
                    readBuffer.size() >= 2*(sizeof(sockaddr_in) + 16))
                    (readBuffer.size())(2*(sizeof(sockaddr_in) + 16));

                RCF_ASSERT(mlpfnGetAcceptExSockAddrs);
                mlpfnGetAcceptExSockAddrs(
                    &readBuffer[0],
                    0,
                    sizeof(sockaddr_in) + 16,
                    sizeof(sockaddr_in) + 16,
                    &pLocalAddr,
                    &localAddrLen,
                    &pRemoteAddr,
                    &remoteAddrLen);

                sockaddr_in *pLocalSockAddr =
                    reinterpret_cast<sockaddr_in *>(pLocalAddr);

                sessionStatePtr->mLocalAddress = IpAddress(*pLocalSockAddr);

                sockaddr_in *pRemoteSockAddr =
                    reinterpret_cast<sockaddr_in *>(pRemoteAddr);

                sessionStatePtr->mRemoteAddress = IpAddress(*pRemoteSockAddr);
            }

            InterlockedDecrement( (LONG *) &mQueuedAccepts);

            if (mQueuedAccepts < mQueuedAcceptsThreshold)
            {
                mQueuedAcceptsCondition.notify_one();
            }

            // is this ip allowed?
            if (isClientAddrAllowed(sessionStatePtr->mRemoteAddress.getSockAddr()))
            {
                // associate fd with iocp
                int fd = sessionStatePtr->mFd;
                BOOL bRet = mIocpAutoPtr->AssociateSocket(fd, fd);
                int err = Platform::OS::BsdSockets::GetLastError();
                RCF_VERIFY(
                    bRet,
                    Exception(
                        RcfError_Socket, err, RcfSubsystem_Os,
                        "AssociateSocket() failed"))(fd);

                // fake a write completion to get things moving
                sessionStatePtr->mState = SessionState::WritingData;
                sessionStatePtr->mWriteBufferRemaining = 0;
                transition(sessionStatePtr);
            }

            break;

        case SessionState::ReadingDataCount:
            {
                std::size_t readBufferRemaining = sessionStatePtr->mReadBufferRemaining;

                RCF_ASSERT(
                    0 <= readBufferRemaining && readBufferRemaining <= 4)
                    (readBufferRemaining);

                if (readBufferRemaining == 0)
                {
                    std::vector<char> &readBuffer = sessionStatePtr->getReadBuffer();
                    unsigned int packetLength = * (unsigned int *) (&readBuffer[0]);
                    networkToMachineOrder(&packetLength, 4, 1);
                    if (packetLength <= getMaxMessageLength())
                    {
                        // TODO: configurable limit on packetLength
                        sessionStatePtr->getReadBuffer().resize(packetLength);
                        sessionStatePtr->mReadBufferRemaining = packetLength;
                        sessionStatePtr->mState = SessionState::ReadingData;
                        transition(sessionStatePtr);
                    }
                    else
                    {

                        boost::shared_ptr<std::vector<char> > vecPtr(
                            new std::vector<char>(4+1+1+4));

                        // without RCF:: qualifiers, borland chooses not to generate any code at all...
                        std::size_t pos = 4;
                        RCF::encodeInt(Descriptor_Error, *vecPtr, pos);
                        RCF::encodeInt(0, *vecPtr, pos);
                        RCF::encodeInt(RcfError_ServerMessageLength, *vecPtr, pos);

                        std::vector<ByteBuffer> byteBuffers;

                        byteBuffers.push_back( ByteBuffer(
                            &(*vecPtr)[4],
                            pos-4,
                            4,
                            vecPtr));

                        sessionStatePtr->mState = SessionState::Ready;
                        sessionStatePtr->mCloseAfterWrite = true;
                        Fd fd = sessionStatePtr->mFd;
                        postWrite(sessionStatePtr, byteBuffers);

                        // TODO: synchronize this?
                        int ret = shutdown(fd, SD_SEND);
                        int err = GetLastError();

                        RCF_ASSERT(
                            ret == 0 ||
                            (ret == -1 && err == WSAENOTCONN))
                            (ret)(err)(WSAENOTCONN);
                    }
                }
                else if (0 < readBufferRemaining && readBufferRemaining <= 4)
                {
                    std::vector<char> &readBuffer =
                        sessionStatePtr->getReadBuffer();

                    RCF_ASSERT(
                        readBufferRemaining <= readBuffer.size())
                        (readBufferRemaining)(readBuffer.size());

                    std::size_t readIdx = readBuffer.size() - readBufferRemaining;
                    char *readPos = & readBuffer[readIdx];

                    boost::shared_ptr<std::vector<char> > readBufferPtr =
                        sessionStatePtr->mReadBufferPtr;

                    ByteBuffer byteBuffer(
                        readPos,
                        readBufferRemaining,
                        readBufferPtr);

                    sessionStatePtr->read(byteBuffer, readBufferRemaining);
                }
            }
            break;

        case SessionState::ReadingData:
            {
                std::size_t readBufferRemaining = sessionStatePtr->mReadBufferRemaining;
                if (readBufferRemaining == 0)
                {
                    sessionStatePtr->mState = SessionState::Ready;
                    getSessionManager().onReadCompleted(
                        sessionStatePtr->mSessionPtr );
                }
                else
                {
                    std::vector<char> &readBuffer =
                        sessionStatePtr->getReadBuffer();

                    RCF_ASSERT(
                        readBufferRemaining <= readBuffer.size())
                        (readBufferRemaining)(readBuffer.size());

                    std::size_t readIdx = readBuffer.size() - readBufferRemaining;
                    char *readPos = & readBuffer[readIdx];

                    boost::shared_ptr<std::vector<char> > readBufferPtr =
                        sessionStatePtr->mReadBufferPtr;

                    ByteBuffer byteBuffer(
                        readPos,
                        readBufferRemaining,
                        readBufferPtr);

                    sessionStatePtr->read(
                        byteBuffer,
                        readBufferRemaining);

                }
            }
            break;

        case SessionState::WritingData:
            {
                std::size_t writeBufferRemaining = sessionStatePtr->mWriteBufferRemaining;
                if (writeBufferRemaining == 0)
                {
                    if (sessionStatePtr->mCloseAfterWrite)
                    {
                        // NB: the following code keeps the connection open long enough
                        // for the client to read the data that has just been written.
                        // Without it, the client will sometimes not receive the data
                        // and instead just get a hard close. Which is serious problem
                        // for the client (it gets no information at all).

                        Fd fd = sessionStatePtr->mFd;
                        const int BufferSize = 8*1024;
                        char buffer[BufferSize];
                        // TODO: upper limit on iterations?
                        // better to do it async, actually
                        while (recv(fd, buffer, BufferSize, 0) > 0);
                    }
                    else
                    {
                        sessionStatePtr->mWriteByteBuffers.resize(0);
                        sessionStatePtr->mState = SessionState::Ready;

                        getSessionManager().onWriteCompleted(
                            sessionStatePtr->mSessionPtr );
                    }
                }
                else
                {
                    std::vector<ByteBuffer> &writeByteBuffers =
                        sessionStatePtr->mWriteByteBuffers;

                    std::size_t writeBufferLen = RCF::lengthByteBuffers(writeByteBuffers);

                    RCF_ASSERT( writeBufferRemaining <= writeBufferLen )
                        (writeBufferRemaining)(writeBufferLen);

                    std::size_t offset = writeBufferLen - writeBufferRemaining;

                    ThreadLocalCached< std::vector<ByteBuffer> > tlcSlicedbyteBuffers;
                    std::vector<ByteBuffer> &slicedbyteBuffers = tlcSlicedbyteBuffers.get();

                    RCF::sliceByteBuffers(
                        slicedbyteBuffers,
                        writeByteBuffers,
                        offset);

                    sessionStatePtr->write(slicedbyteBuffers);
                    slicedbyteBuffers.resize(0);
                }
            }
            break;

        default:

            RCF_ASSERT(0)(sessionStatePtr->mState)(sessionStatePtr->mFd);
        }

    }

    // TODO: partial completions?
    void SessionState::reflect(
        std::size_t bytesTransferred)
    {
        RCF_ASSERT(
            mState == SessionState::ReadingData ||
            mState == SessionState::ReadingDataCount ||
            mState == SessionState::WritingData)
            (mState);

        RCF_ASSERT(mSynchronized);
        RCF_ASSERT(mReflected);
        RCF_ASSERT(mMutexPtr.get());

        if (mState == SessionState::WritingData)
        {
            mState = SessionState::ReadingData;
            std::vector<char> &readBuffer = getReadBuffer();
            readBuffer.resize(8*1024);
            OVERLAPPED *pOverlapped = this;
            WSAOVERLAPPED *pWsaOverlapped =
                reinterpret_cast<WSAOVERLAPPED *>(pOverlapped);

            u_long len = static_cast<u_long>(readBuffer.size());
            char *buf = &readBuffer[0];

            WSABUF wsabuf = {0};
            wsabuf.buf = buf;
            wsabuf.len = len;

            DWORD dwReceived = 0;
            DWORD dwFlags = 0;

            // set self-reference
            RCF_ASSERT(!mThisPtr.get());
            mThisPtr = mWeakThisPtr.lock();
            RCF_ASSERT(mThisPtr.get());

            using namespace boost::multi_index::detail;
            scope_guard clearSelfReferenceGuard =
                make_guard(resetSessionStatePtr, boost::ref(mThisPtr));

            Lock lock(*mMutexPtr);

            if (!mHasBeenClosed)
            {

                RCF2_TRACE("calling WSARecv()")(wsabuf.len);

                int ret = WSARecv(
                    mFd,
                    &wsabuf,
                    1,
                    &dwReceived,
                    &dwFlags,
                    pWsaOverlapped,
                    NULL);

                int err = WSAGetLastError();

                RCF_ASSERT(ret == -1 || ret == 0);
                if (err == S_OK || err == WSA_IO_PENDING)
                {
                    clearSelfReferenceGuard.dismiss();
                }
            }
        }
        else if (
            mState == SessionState::ReadingData ||
            mState == SessionState::ReadingDataCount)
        {
            mState = SessionState::WritingData;
            std::vector<char> &readBuffer = getReadBuffer();
            OVERLAPPED *pOverlapped = this;
            WSAOVERLAPPED *pWsaOverlapped =
                reinterpret_cast<WSAOVERLAPPED *>(pOverlapped);

            WSABUF wsabuf = {0};
            wsabuf.buf =  (char *) &readBuffer[0];
            wsabuf.len = static_cast<u_long>(bytesTransferred);

            DWORD dwSent = 0;
            DWORD dwFlags = 0;

            // set self-reference
            RCF_ASSERT(!mThisPtr.get());
            mThisPtr = mWeakThisPtr.lock();
            RCF_ASSERT(mThisPtr.get());

            using namespace boost::multi_index::detail;
            scope_guard clearSelfReferenceGuard =
                make_guard(resetSessionStatePtr, boost::ref(mThisPtr));

            SessionStatePtr sessionStatePtr = mReflectionSessionStateWeakPtr.lock();
            if (sessionStatePtr)
            {
                RCF_ASSERT(sessionStatePtr->mSynchronized);
                RCF_ASSERT(sessionStatePtr->mReflected);
                RCF_ASSERT(sessionStatePtr->mMutexPtr.get());

                Lock lock(*sessionStatePtr->mMutexPtr);

                if (!sessionStatePtr->mHasBeenClosed)
                {

                    RCF2_TRACE("calling WSASend()")(wsabuf.len);

                    int ret = WSASend(
                        sessionStatePtr->mFd,
                        &wsabuf,
                        1,
                        &dwSent,
                        dwFlags,
                        pWsaOverlapped,
                        NULL);

                    int err = WSAGetLastError();

                    RCF_ASSERT(ret == -1 || ret == 0);
                    if (err == S_OK || err == WSA_IO_PENDING)
                    {
                        clearSelfReferenceGuard.dismiss();
                    }
                }
            }
        }
    }

    bool ServerTransport::cycleAccepts(
        int timeoutMs,
        const volatile bool &stopFlag)
    {
        if (timeoutMs == 0)
        {
            generateAccepts();
        }
        else
        {
            Lock lock(mQueuedAcceptsMutex);
            if (!stopFlag && !mStopFlag)
            {
                mQueuedAcceptsCondition.wait(lock);
                if (!stopFlag && !mStopFlag)
                {
                    generateAccepts();
                }
                else
                {
                    return true;
                }
            }
        }
        return stopFlag || mStopFlag;
    }

    void ServerTransport::stopAccepts()
    {
        mStopFlag = true;
        Lock lock(mQueuedAcceptsMutex);
        mQueuedAcceptsCondition.notify_one();
    }

    void ServerTransport::generateAccepts()
    {
        if (mAcceptorFd == -1)
        {
            mQueuedAccepts = mQueuedAcceptsThreshold;
            return;
        }
        else if (mQueuedAccepts < mQueuedAcceptsThreshold)
        {
            for (unsigned int i=0; i<mQueuedAcceptsAugment; i++)
            {
                Fd fd = static_cast<Fd>( socket(
                    AF_INET,
                    SOCK_STREAM,
                    IPPROTO_TCP));

                int error = Platform::OS::BsdSockets::GetLastError();

                RCF_VERIFY(
                    fd != -1,
                    Exception(
                        RcfError_Socket,
                        error,
                        RcfSubsystem_Os,
                        "socket() failed"));

                Platform::OS::BsdSockets::setblocking(fd, false);
                SessionStatePtr sessionStatePtr = createSession(fd);
                std::vector<char> &readBuffer =
                    sessionStatePtr->getUniqueReadBuffer();

                readBuffer.resize(2*(sizeof(sockaddr_in) + 16));

                sessionStatePtr->mReadBufferRemaining =
                    2*(sizeof(sockaddr_in) + 16);

                DWORD dwBytes = 0;

                for (unsigned int i=0; i<2*(sizeof(sockaddr_in) + 16); ++i)
                {
                    readBuffer[i] = 0;
                }

                sessionStatePtr->clearOverlapped();

                sessionStatePtr->mThisPtr = sessionStatePtr;

                BOOL ret = mlpfnAcceptEx(
                    mAcceptorFd,
                    fd,
                    &readBuffer[0],
                    0,
                    sizeof(sockaddr_in) + 16,
                    sizeof(sockaddr_in) + 16,
                    &dwBytes,
                    static_cast<OVERLAPPED *>(sessionStatePtr.get()));

                int err = WSAGetLastError();

                if (ret == FALSE && err == ERROR_IO_PENDING)
                {
                    // async accept initiated successfully
                }
                else if (dwBytes > 0)
                {
                    RCF_ASSERT(0);
                    sessionStatePtr->mThisPtr.reset();
                    transition(sessionStatePtr);
                }
                else
                {
                    sessionStatePtr->mThisPtr.reset();
                    int err = Platform::OS::BsdSockets::GetLastError();
                    RCF_THROW(Exception(
                        RcfError_Socket,
                        err,
                        RcfSubsystem_Os,
                        "AcceptEx() failed"))
                    (err);
                }

#if defined(_MSC_VER) && _MSC_VER <= 1200

#else
                BOOST_STATIC_ASSERT( sizeof(LONG) == sizeof(mQueuedAccepts) );
#endif

                InterlockedIncrement( (LONG *) &mQueuedAccepts);
            }
        }
    }

    void ServerTransport::flushIocp() const
    {
        DWORD dwMilliseconds = 0;
        DWORD dwNumBytes = 0;
        ULONG_PTR completionKey = 0;
        OVERLAPPED *pOverlapped = 0;

        while (true)
        {
            BOOL ret = mIocpAutoPtr->GetStatus(
                &completionKey,
                &dwNumBytes,
                &pOverlapped,
                dwMilliseconds);

            DWORD dwErr = GetLastError();

            RCF_UNUSED_VARIABLE(ret);
            RCF_UNUSED_VARIABLE(dwErr);

            if (pOverlapped)
            {
                SessionState *pSessionState =
                    static_cast<SessionState *>(pOverlapped);
                pSessionState->mThisPtr.reset();
            }
            else
            {
                break;
            }
        }
    }

    void ServerTransport::cycle(
        int timeoutMs,
        const volatile bool &stopFlag)
    {

        RCF_UNUSED_VARIABLE(stopFlag);

        if (mQueuedAccepts < mQueuedAcceptsThreshold)
        {
            mQueuedAcceptsCondition.notify_one();
        }

        // extract a completed io operation from the iocp
        DWORD           dwMilliseconds = timeoutMs < 0 ? INFINITE : timeoutMs;
        DWORD           dwNumBytes = 0;
        ULONG_PTR       completionKey = 0;
        OVERLAPPED *    pOverlapped = 0;

        BOOL ret = mIocpAutoPtr->GetStatus(
            &completionKey,
            &dwNumBytes,
            &pOverlapped,
            dwMilliseconds);

        DWORD dwErr = GetLastError();

        RCF_ASSERT(
            pOverlapped || (!pOverlapped && dwErr == WAIT_TIMEOUT))
            (pOverlapped)(dwErr);

        if (pOverlapped)
        {
            SessionState *pSessionState =
                static_cast<SessionState *>(pOverlapped);
            SessionStatePtr sessionStatePtr(pSessionState->mThisPtr);
            if (sessionStatePtr)
            {
                sessionStatePtr->mThisPtr.reset();
                if (ret)
                {
                    if (completionKey == static_cast<SOCKET>(mAcceptorFd))
                    {
                        // accept completed
                        SetCurrentSessionGuard guard(sessionStatePtr->mSessionPtr);
                        sessionStatePtr->onAcceptCompleted();
                    }
                    else if (dwNumBytes > 0)
                    {
                        // read or write completed
                        SetCurrentSessionGuard guard(sessionStatePtr->mSessionPtr);
                        int bytesRead = dwNumBytes;
                        sessionStatePtr->onReadWriteCompleted(bytesRead, 0);
                    }
                }
            }
        }
    }

    void ServerTransport::onFilteredReadCompleted(
        const SessionStateWeakPtr &sessionStateWeakPtr,
        const ByteBuffer &byteBuffer,
        int error)
    {
        std::size_t bytesTransferred = byteBuffer.getLength() ;
        onReadWriteCompleted(sessionStateWeakPtr, bytesTransferred, error);
    }

    void ServerTransport::onFilteredWriteCompleted(
        const SessionStateWeakPtr &sessionStateWeakPtr,
        std::size_t bytesTransferred,
        int error)
    {
        onReadWriteCompleted(sessionStateWeakPtr, bytesTransferred, error);
    }

    void ServerTransport::onReadWriteCompleted(
        const SessionStateWeakPtr &sessionStateWeakPtr,
        std::size_t bytesTransferred,
        int error)
    {
        SessionStatePtr sessionStatePtr( sessionStateWeakPtr.lock());
        if (sessionStatePtr)
        {
            if (error == 0)
            {
                if (sessionStatePtr->mState == SessionState::ReadingData ||
                    sessionStatePtr->mState == SessionState::ReadingDataCount)
                {
                    std::size_t &readBufferRemaining =
                        sessionStatePtr->mReadBufferRemaining;

                    RCF_ASSERT(
                        bytesTransferred <= readBufferRemaining )
                        (bytesTransferred)(readBufferRemaining);

                    readBufferRemaining -= bytesTransferred;
                    transition(sessionStatePtr);
                }
                else if (sessionStatePtr->mState == SessionState::WritingData)
                {
                    std::size_t &writeBufferRemaining =
                        sessionStatePtr->mWriteBufferRemaining;

                    RCF_ASSERT(
                        bytesTransferred <= writeBufferRemaining)
                        (bytesTransferred)(writeBufferRemaining);

                    writeBufferRemaining -= bytesTransferred;
                    transition(sessionStatePtr);
                }
            }
        }
    }

    void SessionState::onAcceptCompleted()
    {
        mTransport.transition(shared_from_this());
    }

    void ServerTransport::postWrite(const SessionStatePtr &sessionStatePtr)
    {
        BOOST_STATIC_ASSERT(sizeof(unsigned int) == 4);

        RCF_ASSERT(
            sessionStatePtr->mState == SessionState::Ready)
            (sessionStatePtr->mState);

        RCF_ASSERT(
            sessionStatePtr->mWriteBuffer.size() > 4)
            (sessionStatePtr->mWriteBuffer.size());

        sessionStatePtr->mState = SessionState::WritingData;

        sessionStatePtr->mWriteBufferRemaining = static_cast<unsigned int>(
            sessionStatePtr->mWriteBuffer.size());

        RCF_ASSERT(
            sessionStatePtr->mWriteBuffer.size() >= 4)
            (sessionStatePtr->mWriteBuffer.size());

        *(unsigned int*) &sessionStatePtr->mWriteBuffer[0] =
            static_cast<unsigned int>(sessionStatePtr->mWriteBuffer.size()-4);

        RCF::machineToNetworkOrder(&sessionStatePtr->mWriteBuffer[0], 4, 1);

        transition(sessionStatePtr);
    }

    void ServerTransport::postWrite(
        const SessionStatePtr &sessionStatePtr,
        const std::vector<ByteBuffer> &byteBuffers)
    {
        RCF_ASSERT(
            sessionStatePtr->mState == SessionState::Ready)
            (sessionStatePtr->mState);

        RCF_ASSERT(sizeof(unsigned int) == 4);

        std::vector<ByteBuffer> &writeByteBuffers =
            sessionStatePtr->mWriteByteBuffers;

        writeByteBuffers.resize(0);

        std::copy(
            byteBuffers.begin(),
            byteBuffers.end(),
            std::back_inserter(writeByteBuffers));

        int messageSize = static_cast<int>(RCF::lengthByteBuffers(byteBuffers));
        RCF::machineToNetworkOrder(&messageSize, 4, 1);
        ByteBuffer &byteBuffer = writeByteBuffers.front();

        RCF_ASSERT(
            byteBuffer.getLeftMargin() >= 4)
            (byteBuffer.getLeftMargin());

        byteBuffer.expandIntoLeftMargin(4);
        * (int*) byteBuffer.getPtr() = messageSize;

        sessionStatePtr->mState = SessionState::WritingData;
        sessionStatePtr->mWriteBufferRemaining = RCF::lengthByteBuffers(writeByteBuffers);

        transition(sessionStatePtr);

    }

    void ServerTransport::postRead(const SessionStatePtr &sessionStatePtr)
    {
        RCF_ASSERT(
            sessionStatePtr->mState == SessionState::Ready)
            (sessionStatePtr->mState);

        sessionStatePtr->mState = SessionState::ReadingDataCount;
        sessionStatePtr->getUniqueReadBuffer().resize(4);
        sessionStatePtr->mReadBufferRemaining = 4;

        transition(sessionStatePtr);
    }

    // Thread-safe, forces closure of the session, regardless of mOwnFd
    void ServerTransport::closeSession(const SessionStateWeakPtr &sessionStateWeakPtr, int fd)
    {
        SessionStatePtr sessionStatePtr(sessionStateWeakPtr.lock());
        if (sessionStatePtr)
        {
            RCF_ASSERT(sessionStatePtr->mMutexPtr);

            Lock lock(*sessionStatePtr->mMutexPtr);
            if (!sessionStatePtr->mHasBeenClosed)
            {
                //RCF_VERIFY(0 == closesocket(sessionStatePtr->mFd));

                int ret = Platform::OS::BsdSockets::closesocket(sessionStatePtr->mFd);
                int err = Platform::OS::BsdSockets::GetLastError();

                RCF_VERIFY(
                    ret == 0,
                    Exception(
                        RcfError_SocketClose,
                        err,
                        RcfSubsystem_Os,
                        "closesocket() failed"))
                    (sessionStatePtr->mFd);

                sessionStatePtr->mHasBeenClosed = true;
            }
        }
        else if (fd != -1)
        {
            int ret = Platform::OS::BsdSockets::closesocket(fd);
            int err = Platform::OS::BsdSockets::GetLastError();

            RCF_VERIFY(
                ret == 0,
                Exception(
                    RcfError_SocketClose,
                    err,
                    RcfSubsystem_Os,
                    "closesocket() failed"))
                (sessionStatePtr->mFd);

        }
    }

    // create a server-aware client transport on the connection associated with
    // this session. fd is owned by the client, not the server session.
    // will only create a client transport the first time it is called,
    // after that an empty auto_ptr is returned.
    ClientTransportAutoPtr ServerTransport::createClientTransport(
        SessionPtr sessionPtr)
    {
        ProactorPtr proactorPtr = sessionPtr->getProactorPtr();

        Proactor &tcpIocpProactor =
            dynamic_cast<Proactor &>(*proactorPtr);
        // TODO: exception safety in this function
        SessionStatePtr sessionStatePtr(tcpIocpProactor.getSessionStatePtr());
        RCF_ASSERT(sessionStatePtr->mOwnFd);
        sessionStatePtr->mMutexPtr.reset(new Mutex());
        sessionStatePtr->mOwnFd = false;
        sessionStatePtr->mSynchronized = true;

        std::auto_ptr<TcpClientTransport> tcpClientTransport(
            new TcpClientTransport(sessionStatePtr->mFd));

        tcpClientTransport->setRemoteAddr(
            sessionStatePtr->mRemoteAddress.getSockAddr());

        typedef void (ServerTransport::*Pfn)(const SessionStateWeakPtr &, int);
        tcpClientTransport->setCloseFunctor( boost::bind(
            (Pfn) &ServerTransport::closeSession,
            this,
            SessionStateWeakPtr(sessionStatePtr),
            sessionStatePtr->mFd));

        return ClientTransportAutoPtr(tcpClientTransport.release());
    }

    // create a server session on the connection associated with the client transport
    SessionPtr ServerTransport::createServerSession(
        ClientTransportAutoPtr clientTransportAutoPtr)
    {
        TcpClientTransport &tcpClientTransport =
            dynamic_cast<TcpClientTransport &>(*clientTransportAutoPtr);

        int fd = tcpClientTransport.releaseFd();
        RCF_ASSERT(fd > 0)(fd);
        SessionStatePtr sessionStatePtr = createSession(fd);

        sessionStatePtr->mRemoteAddress =
            IpAddress(tcpClientTransport.getRemoteAddr());

        sessionStatePtr->mState = SessionState::WritingData;
        sessionStatePtr->mWriteBufferRemaining = 0;
        BOOL bRet = mIocpAutoPtr->AssociateSocket(fd, fd);

        int err = Platform::OS::BsdSockets::GetLastError();
        RCF_VERIFY(
            bRet,
            Exception(
                RcfError_Socket, err, RcfSubsystem_Os,
                "AssociateSocket() failed"))(fd);

        transition(sessionStatePtr);
        return sessionStatePtr->mSessionPtr;
    }

    // start reflecting data between the two given sessions
    bool ServerTransport::reflect(
        const SessionPtr &sessionPtr1,
        const SessionPtr &sessionPtr2)
    {
        ProactorPtr proactorPtr1 = sessionPtr1->getProactorPtr();
        ProactorPtr proactorPtr2 = sessionPtr2->getProactorPtr();

        Proactor &tcpIocpProactor1 = dynamic_cast<Proactor &>(*proactorPtr1);
        Proactor &tcpIocpProactor2 = dynamic_cast<Proactor &>(*proactorPtr2);

        SessionStatePtr sessionStatePtr1 = tcpIocpProactor1.getSessionStatePtr();
        SessionStatePtr sessionStatePtr2 = tcpIocpProactor2.getSessionStatePtr();

        return
            sessionStatePtr1.get() &&
            sessionStatePtr2.get() &&
            reflect(
                sessionStatePtr1,
                sessionStatePtr2);

    }

    bool ServerTransport::reflect(
        const SessionStatePtr &sessionStatePtr1,
        const SessionStatePtr &sessionStatePtr2)
    {
        RCF_ASSERT(sessionStatePtr1.get() && sessionStatePtr2.get())
            (sessionStatePtr1.get())(sessionStatePtr2.get());

        sessionStatePtr1->mReflectionSessionStateWeakPtr =
            SessionStateWeakPtr(sessionStatePtr2);
        sessionStatePtr1->mMutexPtr.reset(new Mutex());
        sessionStatePtr1->mSynchronized = true;

        sessionStatePtr2->mReflectionSessionStateWeakPtr =
            SessionStateWeakPtr(sessionStatePtr1);
        sessionStatePtr2->mMutexPtr.reset(new Mutex());
        sessionStatePtr2->mSynchronized = true;

        sessionStatePtr1->mReflected = true;
        sessionStatePtr2->mReflected = true;

        return true;
    }

    // check if a server session is still connected
    bool ServerTransport::isConnected(
        const SessionPtr &sessionPtr)
    {
        Proactor &tcpIocpProactor =
            dynamic_cast<Proactor &>(*sessionPtr->getProactorPtr());

        SessionStatePtr sessionStatePtr = tcpIocpProactor.getSessionStatePtr();
        return sessionStatePtr;

        // TODO: also check mHasBeenClosed on synchronized sessions?
        // ...
    }

    // create a server-aware client transport to given endpoint
    ClientTransportAutoPtr ServerTransport::createClientTransport(
        const I_Endpoint &endpoint)
    {
        const TcpEndpoint &tcpEndpoint =
            dynamic_cast<const TcpEndpoint &>(endpoint);

        return ClientTransportAutoPtr( new TcpClientTransport(
            tcpEndpoint.getIp(),
            tcpEndpoint.getPort()));
    }

    void ServerTransport::setSessionManager(
        I_SessionManager &sessionManager)
    {
        mpSessionManager = &sessionManager;
    }

    I_SessionManager &ServerTransport::getSessionManager()
    {
        RCF_ASSERT(mpSessionManager);
        return *mpSessionManager;
    }

    bool ServerTransport::cycleTransportAndServer(
        RcfServer &server,
        int timeoutMs,
        const volatile bool &stopFlag)
    {
        if (!stopFlag && !mStopFlag)
        {
            cycle(timeoutMs/2, stopFlag);
            server.cycleSessions(timeoutMs/2, stopFlag);
        }
        return stopFlag || mStopFlag;
    }

    void ServerTransport::onServiceAdded(RcfServer &server)
    {
        setSessionManager(server);

        WriteLock writeLock( getTaskEntriesMutex() );
        getTaskEntries().clear();

        getTaskEntries().push_back(
            TaskEntry(
                boost::bind(
                    &ServerTransport::cycleTransportAndServer,
                    this,
                    boost::ref(server),
                    _1,
                    _2),
                StopFunctor(),
                "RCF iocp server"));

        getTaskEntries().push_back(
            TaskEntry(
                boost::bind(&ServerTransport::cycleAccepts, this, _1, _2),
                boost::bind(&ServerTransport::stopAccepts, this),
                "RCF iocp accept"));

        mStopFlag = false;
    }

    void ServerTransport::onServiceRemoved(RcfServer &)
    {}

    void ServerTransport::onServerStart(RcfServer &)
    {
        if (!mOpen)
        {
            open();
            mOpen = true;
        }
    }

    void ServerTransport::onServerStop(RcfServer &)
    {
        if (mOpen)
        {
            close();
            mOpen = false;
            mStopFlag = false;
        }
    }

    void ServerTransport::onServerOpen(RcfServer &)
    {
        if (!mOpen)
        {
            open();
            mOpen = true;
        }
    }

    void ServerTransport::onServerClose(RcfServer &)
    {
        if (mOpen)
        {
            close();
            mOpen = false;
        }
    }

    } // namespace TcpIocp

} // namespace RCF
