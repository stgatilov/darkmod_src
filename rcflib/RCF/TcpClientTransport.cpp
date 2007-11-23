
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/TcpClientTransport.hpp>

#include <boost/bind.hpp>

#include <RCF/TcpEndpoint.hpp>
#include <RCF/TimedBsdSockets.hpp>

namespace RCF {

    class TcpClientFilterProxy : public Filter, boost::noncopyable
    {
    public:
        TcpClientFilterProxy(TcpClientTransport &transport, Filter &filter, bool top) :
            mTransport(transport),
            mFilter(filter),
            mTop(top)
        {}

    private:

        void read(const ByteBuffer &byteBuffer, std::size_t bytesRequested)
        {
            mTop ?
                mFilter.read(byteBuffer, bytesRequested) :
                mTransport.bsdRecv(byteBuffer, bytesRequested);
        }

        void write(const std::vector<ByteBuffer> &byteBuffers)
        {
            mTop ?
                mFilter.write(byteBuffers) :
                mTransport.bsdSend(byteBuffers);
        }

        void onReadCompleted(const ByteBuffer &byteBuffer, int error)
        {
            mTop ?
                mTransport.onReadCompleted(byteBuffer, error) :
                mFilter.onReadCompleted(byteBuffer, error);
        }

        void onWriteCompleted(std::size_t bytesTransferred, int error)
        {
            mTop ?
                mTransport.onWriteCompleted(bytesTransferred, error) :
                mFilter.onWriteCompleted(bytesTransferred, error);
        }

        const FilterDescription &getFilterDescription() const
        {
            RCF_ASSERT(0);
            return * (const FilterDescription *) NULL;
        }

        TcpClientTransport &mTransport;
        Filter &mFilter;
        bool mTop;
    };

#ifdef BOOST_WINDOWS

    // return -2 for timeout, -1 for error, 0 for ready
    int pollSocketWithProgressMwfmo(
        const ClientProgressPtr &clientProgressPtr,
        ClientProgress::Activity activity,
        unsigned int endTimeMs,
        int fd,
        int &err,
        bool bRead)
    {
        bool bTimer = clientProgressPtr->mTriggerMask & ClientProgress::Timer ?
            true :
            false;

        bool bUiMessage = clientProgressPtr->mTriggerMask & ClientProgress::UiMessage ?
            true :
            false;

        unsigned int pollingIntervalMs = bTimer ?
            clientProgressPtr->mTimerIntervalMs :
            -1;

        int uiMessageFilter = clientProgressPtr->mUiMessageFilter;
        HANDLE readEvent = WSACreateEvent();
        using namespace boost::multi_index::detail;
        scope_guard WSACloseEventGuard = make_guard(WSACloseEvent, readEvent);
        RCF_UNUSED_VARIABLE(WSACloseEventGuard);
        int nRet = WSAEventSelect(fd, readEvent, bRead ? FD_READ : FD_WRITE);
        RCF_ASSERT(nRet == 0)(nRet);
        HANDLE handles[] = { readEvent };
        while (true)
        {
            unsigned int timeoutMs = generateTimeoutMs(endTimeMs);
            timeoutMs = RCF_MIN(timeoutMs, pollingIntervalMs);

            DWORD dwRet = bUiMessage ?
                MsgWaitForMultipleObjects(1, handles, 0, timeoutMs, uiMessageFilter) :
                WaitForMultipleObjects(1, handles, 0, timeoutMs);

            if (dwRet == WAIT_TIMEOUT)
            {
                if (generateTimeoutMs(endTimeMs) == 0)
                {
                    err = 0;
                    return -2;
                }
                else if (bTimer)
                {
                    ClientProgress::Action action = ClientProgress::Continue;

                    clientProgressPtr->mProgressCallback(
                        0,
                        0,
                        ClientProgress::Timer,
                        activity,
                        action);

                    RCF_VERIFY(
                        action != ClientProgress::Cancel,
                        Exception(RcfError_ClientCancel))
                        (endTimeMs)(getCurrentTimeMs())(pollingIntervalMs);
                }
            }
            else if (dwRet == WAIT_OBJECT_0)
            {
                // read event signalled
                return 0;
            }
            else if (bUiMessage && dwRet == WAIT_OBJECT_0 + 1)
            {
                ClientProgress::Action action = ClientProgress::Continue;

                clientProgressPtr->mProgressCallback(
                    0,
                    0,
                    ClientProgress::UiMessage,
                    activity,
                    action);

                RCF_VERIFY(
                    action != ClientProgress::Cancel,
                    Exception(RcfError_ClientCancel))
                    (endTimeMs)(getCurrentTimeMs())(pollingIntervalMs);

                // a sample message filter

                //MSG msg = {0};
                //while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                //{
                //    if (msg.message == WM_QUIT)
                //    {
                //
                //    }
                //    else if (msg.message == WM_PAINT)
                //    {
                //        TranslateMessage(&msg);
                //        DispatchMessage(&msg);
                //    }
                //}

            }

        }
    }

#endif

    class PollingFunctor : public I_PollingFunctor
    {
    public:
        PollingFunctor(
            const ClientProgressPtr &clientProgressPtr,
            ClientProgress::Activity activity,
            unsigned int endTimeMs) :
                mClientProgressPtr(clientProgressPtr),
                mActivity(activity),
                mEndTimeMs(endTimeMs)
        {}

        int operator()(int fd, int &err, bool bRead)
        {

#ifdef BOOST_WINDOWS

            if (
                mClientProgressPtr.get() &&
                mClientProgressPtr->mTriggerMask & ClientProgress::UiMessage)
            {
                return pollSocketWithProgressMwfmo(
                    mClientProgressPtr,
                    mActivity,
                    mEndTimeMs,
                    fd,
                    err,
                    bRead);
            }
            else

#endif

            if (mClientProgressPtr)
            {
                return pollSocketWithProgress(
                    mClientProgressPtr,
                    mActivity,
                    mEndTimeMs,
                    fd,
                    err,
                    bRead);
            }
            else
            {
                return pollSocket(
                    mEndTimeMs,
                    fd,
                    err,
                    bRead);
            }
        }

    private:
        ClientProgressPtr mClientProgressPtr;
        ClientProgress::Activity mActivity;
        unsigned int mEndTimeMs;
    };



    void TcpClientTransport::bsdRecv(
        const ByteBuffer &byteBuffer_,
        std::size_t bytesRequested)
    {
        ByteBuffer byteBuffer = byteBuffer_;
        if (byteBuffer.getLength() == 0)
        {
            if (mReadBuffer2Ptr.get() == NULL || !mReadBuffer2Ptr.unique())
            {
                mReadBuffer2Ptr.reset( new std::vector<char>() );
            }
            mReadBuffer2Ptr->resize(bytesRequested);
            byteBuffer = ByteBuffer(mReadBuffer2Ptr);
        }

        std::size_t bytesToRead = RCF_MIN(bytesRequested, byteBuffer.getLength());

        PollingFunctor pollingFunctor(
            mClientProgressPtr,
            ClientProgress::Receive,
            mEndTimeMs);

        RCF3_TRACE("calling recv()")(byteBuffer.getLength())(bytesToRead);

        int err = 0;
        int ret = RCF::timedRecv(
            pollingFunctor,
            err,
            mFd,
            byteBuffer,
            bytesToRead,
            0);

        switch (ret)
        {
        case -2:
            RCF_THROW(
                Exception(RcfError_ClientReadTimeout))
                (bytesToRead);
            break;

        case -1:
            RCF_THROW(
                Exception(
                    RcfError_ClientReadFail,
                    err,
                    RcfSubsystem_Os))
                (bytesToRead)(err);
            break;

        case  0:
            RCF_THROW(
                Exception(RcfError_PeerDisconnect))
                (bytesToRead);
            break;

        default:
            RCF_ASSERT(
                0 < ret && ret <= static_cast<int>(bytesRequested))
                (ret)(bytesRequested);

            ByteBuffer b(byteBuffer.release(), 0, ret);
            mTransportFilters.empty() ?
                onReadCompleted(b, 0) :
                mTransportFilters.back()->onReadCompleted(b, 0);
        }
    }

    void TcpClientTransport::bsdSend(const std::vector<ByteBuffer> &byteBuffers)
    {
        PollingFunctor pollingFunctor(
            mClientProgressPtr,
            ClientProgress::Send,
            mEndTimeMs);

        int err = 0;

        RCF3_TRACE("calling send()")(lengthByteBuffers(byteBuffers));

        int ret = RCF::timedSend(
            pollingFunctor,
            err,
            mFd,
            byteBuffers,
            getMaxSendSize(),
            0);

        switch (ret)
        {
        case -2:
            RCF_THROW(
                Exception(RcfError_ClientWriteTimeout));
            break;

        case -1:
            RCF_THROW(
                Exception(
                    RcfError_ClientWriteFail,
                    err,
                    RcfSubsystem_Os))
                (err);
            break;

        case 0:
            RCF_THROW(
                Exception(RcfError_PeerDisconnect));
            break;

        default:
            RCF_ASSERT(
                0 < ret && ret <= static_cast<int>(lengthByteBuffers(byteBuffers)))
                (ret)(lengthByteBuffers(byteBuffers));

            mTransportFilters.empty() ?
                onWriteCompleted(ret, 0) :
                mTransportFilters.back()->onWriteCompleted(ret, 0);
        }

    }

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4355 )  // warning C4355: 'this' : used in base member initializer list
#endif

    TcpClientTransport::TcpClientTransport(const std::string &ip, int port) :
        mRemoteAddr(),
        mIp(ip),
        mPort(port),
        mFd(-1),
        mOwn(true),
        mMaxSendSize(1024*1024*10),
        mBytesTransferred(RCF_DEFAULT_INIT),
        mBytesSent(RCF_DEFAULT_INIT),
        mBytesRead(RCF_DEFAULT_INIT),
        mBytesTotal(RCF_DEFAULT_INIT),
        mError(RCF_DEFAULT_INIT),
        mEndTimeMs(RCF_DEFAULT_INIT)
    {
        memset(&mRemoteAddr, 0, sizeof(mRemoteAddr));
        setTransportFilters( std::vector<FilterPtr>() );
    }

    TcpClientTransport::TcpClientTransport(const sockaddr_in &remoteAddr) :
        mRemoteAddr(remoteAddr),
        mIp(),
        mPort(RCF_DEFAULT_INIT),
        mFd(-1),
        mOwn(true),
        mMaxSendSize(1024*1024*10),
        mBytesTransferred(RCF_DEFAULT_INIT),
        mBytesSent(RCF_DEFAULT_INIT),
        mBytesRead(RCF_DEFAULT_INIT),
        mBytesTotal(RCF_DEFAULT_INIT),
        mError(RCF_DEFAULT_INIT),
        mEndTimeMs(RCF_DEFAULT_INIT)
    {
        setTransportFilters( std::vector<FilterPtr>() );
    }

    TcpClientTransport::TcpClientTransport(const TcpClientTransport &rhs) :
        I_ClientTransport(rhs),
        WithProgressCallback(rhs),
        mRemoteAddr(rhs.mRemoteAddr),
        mIp(rhs.mIp),
        mPort(rhs.mPort),
        mFd(-1),
        mOwn(true),
        mMaxSendSize(1024*1024*10),
        mBytesTransferred(RCF_DEFAULT_INIT),
        mBytesSent(RCF_DEFAULT_INIT),
        mBytesRead(RCF_DEFAULT_INIT),
        mBytesTotal(RCF_DEFAULT_INIT),
        mError(RCF_DEFAULT_INIT),
        mEndTimeMs(RCF_DEFAULT_INIT)
    {
        setTransportFilters( std::vector<FilterPtr>() );
    }

    TcpClientTransport::TcpClientTransport(int fd) :
        mRemoteAddr(),
        mIp(),
        mPort(RCF_DEFAULT_INIT),
        mFd(fd),
        mOwn(true),
        mMaxSendSize(1024*1024*10),
        mBytesTransferred(RCF_DEFAULT_INIT),
        mBytesSent(RCF_DEFAULT_INIT),
        mBytesRead(RCF_DEFAULT_INIT),
        mBytesTotal(RCF_DEFAULT_INIT),
        mError(RCF_DEFAULT_INIT),
        mEndTimeMs(RCF_DEFAULT_INIT)
    {
        memset(&mRemoteAddr, 0, sizeof(mRemoteAddr));
        setTransportFilters( std::vector<FilterPtr>() );
    }

#ifdef _MSC_VER
#pragma warning( pop )
#endif

    TcpClientTransport::~TcpClientTransport()
    {
        RCF_DTOR_BEGIN
            if (mOwn)
            {
                close();
            }
        RCF_DTOR_END
    }

    std::auto_ptr<I_ClientTransport> TcpClientTransport::clone() const
    {
        return ClientTransportAutoPtr( new TcpClientTransport(*this) );
    }

    EndpointPtr TcpClientTransport::getEndpointPtr() const
    {
        return EndpointPtr( new TcpEndpoint(mIp, mPort) );
    }

    void TcpClientTransport::connect(unsigned int timeoutMs)
    {
        // TODO: replace throw with return, where possible

        unsigned int startTimeMs = getCurrentTimeMs();
        mEndTimeMs = startTimeMs + timeoutMs;

        if (!isConnected())
        {
            // close the current connection
            close();

            mFd = static_cast<int>( ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP) );
            int err = Platform::OS::BsdSockets::GetLastError();
            RCF_VERIFY(
                mFd != -1,
                Exception(
                    RcfError_Socket, err, RcfSubsystem_Os, "socket() failed"));
            Platform::OS::BsdSockets::setblocking(mFd, false);

            if (mRemoteAddr.sin_addr.s_addr == 0)
            {
                unsigned long ul_addr = ::inet_addr(mIp.c_str());
                if (ul_addr == INADDR_NONE)
                {
                    hostent *hostDesc = ::gethostbyname( mIp.c_str() );
                    if (hostDesc)
                    {
                        char *szIp = ::inet_ntoa( * (in_addr*) hostDesc->h_addr_list[0]);
                        ul_addr = ::inet_addr(szIp);
                    }
                }

                memset(&mRemoteAddr, 0, sizeof(mRemoteAddr));
                mRemoteAddr.sin_family = AF_INET;
                mRemoteAddr.sin_addr.s_addr = ul_addr;
                // the :: seems to screw up gcc (!?!)
                //remoteAddr.sin_port = ::htons(mPort);
                mRemoteAddr.sin_port = htons( static_cast<u_short>(mPort) );
            }

            PollingFunctor pollingFunctor(
                mClientProgressPtr,
                ClientProgress::Connect,
                mEndTimeMs);

            err = 0;

            int ret = timedConnect(
                pollingFunctor,
                err,
                mFd,
                (sockaddr*) &mRemoteAddr,
                sizeof(mRemoteAddr));

            if (ret != 0)
            {
                close();

                int rcfErr = (err == 0) ?
                    RcfError_ClientConnectTimeout :
                    RcfError_ClientConnectFail;

                RCF_THROW(
                    Exception(rcfErr, err, RcfSubsystem_Os))
                    (ret)(err)(timeoutMs)(mIp)(mPort);
            }

            if (
                mClientProgressPtr.get() &&
                (mClientProgressPtr->mTriggerMask & ClientProgress::Event))
            {
                ClientProgress::Action action = ClientProgress::Continue;

                mClientProgressPtr->mProgressCallback(
                    0,
                    0,
                    ClientProgress::Event,
                    ClientProgress::Connect,
                    action);

                RCF_VERIFY(
                    action != ClientProgress::Cancel,
                    Exception(RcfError_ClientCancel));
            }
        }

    }

    void TcpClientTransport::disconnect(unsigned int timeoutMs)
    {
        RCF_UNUSED_VARIABLE(timeoutMs);

        // close the connection
        close();
    }

    // helper to disambiguate std::vector<ByteBuffer>::resize()
    typedef void (std::vector<ByteBuffer>::*PfnResize)(std::vector<ByteBuffer>::size_type);

    void clearByteBuffers( std::vector<ByteBuffer> &byteBuffers)
    {
        byteBuffers.resize(0);
    }

    int TcpClientTransport::send(
        const std::vector<ByteBuffer> &data,
        unsigned int totalTimeoutMs)
    {

        RCF3_TRACE("")(lengthByteBuffers(data))(totalTimeoutMs);

        unsigned int startTimeMs = getCurrentTimeMs();
        mEndTimeMs = startTimeMs + totalTimeoutMs;

        RCF_VERIFY(
            isConnected(),
            RCF::Exception(RcfError_NotConnected));

        mByteBuffers.resize(0);

        using namespace boost::multi_index::detail;
        scope_guard resizeGuard =
            make_guard(clearByteBuffers, boost::ref(mByteBuffers));
        RCF_UNUSED_VARIABLE(resizeGuard);

        std::copy(data.begin(), data.end(), std::back_inserter(mByteBuffers));

        ByteBuffer &byteBuffer = const_cast<ByteBuffer &>(mByteBuffers.front());
        unsigned int messageLength = static_cast<unsigned int>(lengthByteBuffers(data));
        RCF::machineToNetworkOrder(&messageLength, 4, 1);
        BOOST_STATIC_ASSERT(sizeof(unsigned int) == 4);
        RCF_ASSERT(byteBuffer.getLeftMargin() >= 4)(byteBuffer.getLeftMargin());
        byteBuffer.expandIntoLeftMargin(4);
        * (unsigned int*) byteBuffer.getPtr() = messageLength;

        std::size_t ret = timedSend(mByteBuffers);
        RCF_ASSERT(ret == lengthByteBuffers(mByteBuffers))(ret)(lengthByteBuffers(mByteBuffers));

        return 1;
    }

    std::size_t TcpClientTransport::timedSend(const std::vector<ByteBuffer> &data)
    {
        std::size_t bytesRequested = lengthByteBuffers(data);
        std::size_t bytesToWrite = bytesRequested;
        std::size_t bytesWritten = 0;

        using namespace boost::multi_index::detail;
        scope_guard resizeGuard =
            make_guard(clearByteBuffers, boost::ref(mSlicedByteBuffers));
        RCF_UNUSED_VARIABLE(resizeGuard);

        while (true)
        {
            sliceByteBuffers(mSlicedByteBuffers, data, bytesWritten);

            mTransportFilters.empty() ?
                bsdSend(mSlicedByteBuffers):
                mTransportFilters.front()->write(mSlicedByteBuffers);

            RCF_ASSERT(
                0 < mBytesTransferred &&
                mBytesTransferred <= lengthByteBuffers(mSlicedByteBuffers))
                (mBytesTransferred)(lengthByteBuffers(mSlicedByteBuffers));

            bytesToWrite -= mBytesTransferred;
            bytesWritten += mBytesTransferred;

            if (
                mClientProgressPtr.get() &&
                (mClientProgressPtr->mTriggerMask & ClientProgress::Event))
            {
                ClientProgress::Action action = ClientProgress::Continue;

                mClientProgressPtr->mProgressCallback(
                    bytesWritten, bytesRequested,
                    ClientProgress::Event,
                    ClientProgress::Send,
                    action);

                RCF_VERIFY(
                    action != ClientProgress::Cancel,
                    Exception(RcfError_ClientCancel))
                    (mBytesSent)(mBytesTotal);
            }

            if (bytesToWrite == 0)
            {
                return bytesWritten;
            }
        }
    }

    // return bufferLen
    std::size_t TcpClientTransport::timedReceive(
        ByteBuffer &byteBuffer,
        std::size_t bytesRequested)
    {
        std::size_t bytesToRead = bytesRequested;
        std::size_t bytesRead = 0;
        while (true)
        {
            ByteBuffer buffer(byteBuffer, bytesRead, bytesToRead);

            mTransportFilters.empty() ?
                bsdRecv(buffer, bytesToRead):
                mTransportFilters.front()->read(buffer, bytesToRead);

            RCF_ASSERT(
                0 < mBytesTransferred && mBytesTransferred <= bytesToRead)
                (mBytesTransferred)(bytesRead);

            bytesToRead -= mBytesTransferred;
            bytesRead += mBytesTransferred;

            if (
                mClientProgressPtr.get() &&
                (mClientProgressPtr->mTriggerMask & ClientProgress::Event))
            {
                ClientProgress::Action action = ClientProgress::Continue;

                mClientProgressPtr->mProgressCallback(
                    bytesRead,
                    bytesRequested,
                    ClientProgress::Event,
                    ClientProgress::Receive,
                    action);

                RCF_VERIFY(
                    action != ClientProgress::Cancel,
                    Exception(RcfError_ClientCancel))
                    (mBytesRead)(mBytesTotal);
            }

            if (bytesToRead == 0)
            {
                return bytesRead;
            }

        }
    }

    int TcpClientTransport::receive(
        ByteBuffer &byteBuffer,
        unsigned int timeoutMs)
    {
        mEndTimeMs = getCurrentTimeMs() + timeoutMs;

        unsigned int length = 0;
        BOOST_STATIC_ASSERT(sizeof(length) == 4);
        ByteBuffer buffer( (char*) &length, 4);
        std::size_t ret = timedReceive(buffer, 4);

        RCF_ASSERT(ret == 4)(ret);
        networkToMachineOrder(&length, sizeof(length), 1);
        RCF_VERIFY(
            0 <= length && length <= getMaxMessageLength(),
            Exception(RcfError_ClientMessageLength))
            (length)(getMaxMessageLength());

        // read the rest of the message
        if (mReadBufferPtr.get() == NULL || !mReadBufferPtr.unique())
        {
            mReadBufferPtr.reset( new std::vector<char>() );
        }
        mReadBufferPtr->resize(length);

        buffer = ByteBuffer(
            &(*mReadBufferPtr)[0],
            mReadBufferPtr->size(),
            mReadBufferPtr);

        ret = timedReceive(buffer, length);

        RCF_ASSERT(ret == length);
        byteBuffer = buffer;
        RCF3_TRACE("")(length);
        return 1;
    }

    void TcpClientTransport::setCloseFunctor(const CloseFunctor &closeFunctor)
    {
        mCloseFunctor = closeFunctor;
    }

    void TcpClientTransport::setRemoteAddr(const sockaddr_in &remoteAddr)
    {
        mRemoteAddr = remoteAddr;
    }

    const sockaddr_in &TcpClientTransport::getRemoteAddr() const
    {
        return mRemoteAddr;
    }

    void TcpClientTransport::close()
    {
        if (mCloseFunctor)
        {
            mCloseFunctor();
            mCloseFunctor = CloseFunctor();
            mFd = -1;
        }
        else
        {
            if (mFd != -1)
            {
                int ret = Platform::OS::BsdSockets::closesocket(mFd);
                int err = Platform::OS::BsdSockets::GetLastError();

                RCF_VERIFY(
                    ret == 0,
                    Exception(
                        RcfError_Socket,
                        err,
                        RcfSubsystem_Os,
                        "closesocket() failed"))
                    (mFd);

                mFd = -1;
            }
        }
    }

    bool TcpClientTransport::isConnected()
    {
        return isFdConnected(mFd);
    }

    int TcpClientTransport::releaseFd()
    {
        int myFd = mFd;
        mFd = -1;
        return myFd;
    }

    int TcpClientTransport::getFd() const
    {
        return mFd;
    }

    void TcpClientTransport::setTransportFilters(
        const std::vector<FilterPtr> &filters)
    {
        mTransportFilters.clear();
        if (!filters.empty())
        {
            mTransportFilters.push_back(
                FilterPtr(new TcpClientFilterProxy(*this, *filters.front(), true)));

            std::copy(
                filters.begin(),
                filters.end(),
                std::back_inserter(mTransportFilters));

            mTransportFilters.push_back(
                FilterPtr(new TcpClientFilterProxy(*this, *filters.back(), false)));

            RCF::connectFilters(mTransportFilters);

        }
    }

    void TcpClientTransport::onReadCompleted(
        const ByteBuffer &byteBuffer,
        int error)
    {
        if (error)
        {
            RCF_THROW(Exception(RcfError_Filter));
        }
        else
        {
            mBytesTransferred = byteBuffer.getLength() ;
            mError = error;
        }
    }
    void TcpClientTransport::getTransportFilters(
        std::vector<FilterPtr> &filters)
    {
        // TODO: keep the adapter filters out of mTransportFilters?
        if (mTransportFilters.empty())
        {
            filters.resize(0);
        }
        else
        {
            RCF_ASSERT(mTransportFilters.size() >= 3)(mTransportFilters.size());

            //filters.assign(
            //    ++mTransportFilters.begin(),
            //    --mTransportFilters.end());

            // for borland
            std::vector<FilterPtr>::const_iterator iter0(mTransportFilters.begin());
            std::vector<FilterPtr>::const_iterator iter1(mTransportFilters.end());
            ++iter0;
            --iter1;
            filters.assign(iter0, iter1);

        }
    }

    void TcpClientTransport::onWriteCompleted(
        std::size_t bytesTransferred,
        int error)
    {
        if (error)
        {
            // TODO: this should never happen, use an assertion
            RCF_THROW(Exception(RcfError_Filter));
        }
        else
        {
            mBytesTransferred = bytesTransferred;
            mError = error;
        }
    }

    void TcpClientTransport::setMaxSendSize(std::size_t maxSendSize)
    {
        mMaxSendSize = maxSendSize;
    }

    std::size_t TcpClientTransport::getMaxSendSize()
    {
        return mMaxSendSize;
    }

} // namespace RCF
