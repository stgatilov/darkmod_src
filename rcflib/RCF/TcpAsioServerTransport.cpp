
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/TcpAsioServerTransport.hpp>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include <RCF/AsyncFilter.hpp>
#include <RCF/CurrentSession.hpp>
#include <RCF/MethodInvocation.hpp>
#include <RCF/RcfServer.hpp>
#include <RCF/TcpClientTransport.hpp>
#include <RCF/TcpEndpoint.hpp>

namespace RCF {

    // FilterAdapter

    class FilterAdapter : public RCF::IdentityFilter
    {
    public:
        FilterAdapter(TcpAsioServerTransport::SessionState &sessionState) :
            mSessionState(sessionState)
        {}

    private:
        void read(
            const ByteBuffer &byteBuffer,
            std::size_t bytesRequested)
        {
            mSessionState.read(byteBuffer, bytesRequested);
        }

        void write(
            const std::vector<ByteBuffer> &byteBuffers)
        {
            mSessionState.write(byteBuffers);
        }

        void onReadCompleted(
            const ByteBuffer &byteBuffer,
            int error)
        {
            mSessionState.onReadWrite(byteBuffer.getLength(), error);
        }

        void onWriteCompleted(
            std::size_t bytesTransferred,
            int error)
        {
            mSessionState.onReadWrite(bytesTransferred, error);
        }

        const FilterDescription &getFilterDescription() const
        {
            RCF_ASSERT(0);
            return * (const FilterDescription *) NULL;
        }

        TcpAsioServerTransport::SessionState &mSessionState;
    };

    // TcpAsioProactor

    TcpAsioServerTransport::TcpAsioProactor::TcpAsioProactor(TcpAsioServerTransport &tcpAsioServerTransport) :
        mTcpAsioServerTransport(tcpAsioServerTransport)
    {}

    void TcpAsioServerTransport::TcpAsioProactor::postRead()
    {
        SessionStatePtr sessionStatePtr( mSessionStateWeakPtr.lock() );
        RCF_ASSERT(sessionStatePtr.get());

        sessionStatePtr->mState = SessionState::ReadingDataCount;
        sessionStatePtr->getUniqueReadBuffer().resize(4);
        sessionStatePtr->mReadBufferRemaining = 4;
        sessionStatePtr->invokeAsyncRead();
    }

    ByteBuffer TcpAsioServerTransport::TcpAsioProactor::getReadByteBuffer()
    {
        SessionStatePtr sessionStatePtr( mSessionStateWeakPtr.lock() );
        RCF_ASSERT(sessionStatePtr.get());

        return ByteBuffer(
            &(*sessionStatePtr->mReadBufferPtr)[0],
            (*sessionStatePtr->mReadBufferPtr).size(),
            sessionStatePtr->mReadBufferPtr);
    }

    void TcpAsioServerTransport::TcpAsioProactor::postWrite(const std::vector<ByteBuffer> &byteBuffers)
    {
        SessionStatePtr sessionStatePtr( mSessionStateWeakPtr.lock() );
        RCF_ASSERT(sessionStatePtr.get());

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
        sessionStatePtr->invokeAsyncWrite();

    }

    void TcpAsioServerTransport::TcpAsioProactor::postClose()
    {
        SessionStatePtr sessionStatePtr( mSessionStateWeakPtr.lock() );
        RCF_ASSERT(sessionStatePtr.get());
        sessionStatePtr->forceClose();
    }


    I_ServerTransport &TcpAsioServerTransport::TcpAsioProactor::getServerTransport()
    {
        return mTcpAsioServerTransport;
    }

    const I_RemoteAddress &TcpAsioServerTransport::TcpAsioProactor::getRemoteAddress()
    {
        SessionStatePtr sessionStatePtr( mSessionStateWeakPtr.lock() );
        RCF_ASSERT(sessionStatePtr.get());
        return sessionStatePtr->mIpAddress;
    }

    void TcpAsioServerTransport::TcpAsioProactor::setTransportFilters(
        const std::vector<FilterPtr> &filters)
    {
        SessionStatePtr sessionStatePtr( mSessionStateWeakPtr.lock() );
        RCF_ASSERT(sessionStatePtr.get());
        sessionStatePtr->setTransportFilters(filters);
    }

    const std::vector<FilterPtr> &TcpAsioServerTransport::TcpAsioProactor::getTransportFilters()
    {
        return mSessionStateWeakPtr.lock()->getTransportFilters();
    }

    // SessionState

    TcpAsioServerTransport::SessionState::SessionState(
        TcpAsioServerTransport &transport,
        DemuxerPtr demuxerPtr) :
        	mState(Ready),
            mReadBufferRemaining(RCF_DEFAULT_INIT),
            mWriteBufferRemaining(RCF_DEFAULT_INIT),
            mSocketPtr(new Socket(*demuxerPtr)),
            mTransport(transport),
            mFilterAdapterPtr(new FilterAdapter(*this)),
            mClosed(RCF_DEFAULT_INIT),
            mSynchronized(RCF_DEFAULT_INIT),
            mReflecting(RCF_DEFAULT_INIT)
    {
    }

    boost::asio::error closeAsioSocket(boost::asio::ip::tcp::socket &s)
    {
        boost::asio::error error;
        s.close( boost::asio::assign_error(error));
        return error;
    }

    TcpAsioServerTransport::SessionState::~SessionState()
    {
        RCF_DTOR_BEGIN

        // TODO: invoke accept if appropriate
        // TODO: need a proper acceptex strategy in the first place
        //RCF_ASSERT(mState != Accepting);

        // close reflecting session if appropriate
        if (mReflecting)
        {
            SessionStatePtr sessionStatePtr(mReflecteeWeakPtr.lock());
            if (sessionStatePtr)
            {
                sessionStatePtr->forceClose();
            }
        }

        RCF_DTOR_END;
    }

    std::vector<char> &TcpAsioServerTransport::SessionState::getReadBuffer()
    {
        if (!mReadBufferPtr)
        {
            mReadBufferPtr.reset( new std::vector<char>() );
        }
        return *mReadBufferPtr;
    }

    std::vector<char> &TcpAsioServerTransport::SessionState::getUniqueReadBuffer()
    {
        if (!mReadBufferPtr || !mReadBufferPtr.unique())
        {
            mReadBufferPtr.reset( new std::vector<char>() );
        }
        return *mReadBufferPtr;
    }

    ByteBuffer TcpAsioServerTransport::SessionState::getReadByteBuffer() const
    {
        return ByteBuffer(
            &(*mReadBufferPtr)[0],
            (*mReadBufferPtr).size(),
            mReadBufferPtr);
    }

    std::vector<char> &TcpAsioServerTransport::SessionState::getReadBufferSecondary()
    {
        if (!mReadBufferSecondaryPtr)
        {
            mReadBufferSecondaryPtr.reset( new std::vector<char>() );
        }
        return *mReadBufferSecondaryPtr;
    }

    std::vector<char> &TcpAsioServerTransport::SessionState::getUniqueReadBufferSecondary()
    {
        if (!mReadBufferSecondaryPtr || !mReadBufferSecondaryPtr.unique())
        {
            mReadBufferSecondaryPtr.reset( new std::vector<char>() );
        }
        return *mReadBufferSecondaryPtr;
    }

    ByteBuffer TcpAsioServerTransport::SessionState::getReadByteBufferSecondary() const
    {
        return ByteBuffer(
            &(*mReadBufferSecondaryPtr)[0],
            (*mReadBufferSecondaryPtr).size(),
            mReadBufferSecondaryPtr);
    }

    void TcpAsioServerTransport::SessionState::read(
        const ByteBuffer &byteBuffer,
        std::size_t bytesRequested)
    {

        RCF2_TRACE("");

        if (byteBuffer.getLength() == 0)
        {
            std::vector<char> &vec = getUniqueReadBufferSecondary();
            vec.resize(bytesRequested);
            mTempByteBuffer = getReadByteBufferSecondary();
        }
        else
        {
            mTempByteBuffer = ByteBuffer(byteBuffer, 0, bytesRequested);
        }

        RCF_ASSERT(
            bytesRequested <= mTempByteBuffer.getLength())
            (bytesRequested)(mTempByteBuffer.getLength());

        char *buffer = mTempByteBuffer.getPtr();
        std::size_t bufferLen = mTempByteBuffer.getLength();

        if (mSynchronized)
        {
            Lock lock(*mMutexAutoPtr);
            if (!mClosed)
            {
                mSocketPtr->async_receive(
                    boost::asio::buffer(buffer, bufferLen),
                    0,
                    boost::bind(
                        &SessionState::onReadCompletion,
                        shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
            }
        }
        else
        {
            mSocketPtr->async_receive(
                boost::asio::buffer(buffer, bufferLen),
                0,
                boost::bind(
                    &SessionState::onReadCompletion,
                    shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        }
    }

    void TcpAsioServerTransport::SessionState::write(
        const std::vector<ByteBuffer> &byteBuffers)
    {
        RCF2_TRACE("");

        // TODO: send all the buffers at once
        // ...

        RCF_ASSERT(!byteBuffers.empty());
        char *buffer = byteBuffers.front().getPtr();
        std::size_t bufferLen = byteBuffers.front().getLength();

        if (mSynchronized)
        {
            Lock lock(*mMutexAutoPtr);
            if (!mClosed)
            {
                boost::asio::async_write(
                    *mSocketPtr,
                    boost::asio::buffer(buffer, bufferLen),
                    boost::bind(
                    &SessionState::onWriteCompletion,
                    shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
            }
        }
        else
        {
            boost::asio::async_write(
                *mSocketPtr,
                boost::asio::buffer(buffer, bufferLen),
                boost::bind(
                &SessionState::onWriteCompletion,
                shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
        }
    }

    // TODO: merge onReadCompletion/onWriteCompletion into one function

    void TcpAsioServerTransport::SessionState::onReadCompletion(const boost::asio::error &error, size_t bytesTransferred)
    {
        if (!error)
        {
            if (mReflecting)
            {
                onReflectedReadWrite(0, bytesTransferred);
            }
            else
            {
                SetCurrentSessionGuard guard(mSessionPtr);
                ByteBuffer byteBuffer(mTempByteBuffer.release(), 0, bytesTransferred);

                mTransportFilters.empty() ?
                    onReadWrite(bytesTransferred, 0) : // used to be boost::asio::error(error)
                    mTransportFilters.back()->onReadCompleted(byteBuffer, 0);
            }
        }
    }

    void TcpAsioServerTransport::SessionState::onWriteCompletion(const boost::asio::error &error, size_t bytesTransferred)
    {
        if (!error)
        {
            if (mReflecting)
            {
                if (mReflecteePtr)
                {
                    mReflecteePtr.reset();
                }
                onReflectedReadWrite(0, bytesTransferred);
            }
            else
            {
                SetCurrentSessionGuard guard(mSessionPtr);
                mTransportFilters.empty() ?
                    onReadWrite(bytesTransferred, 0) : // used to be boost::asio::error(error)
                    mTransportFilters.back()->onWriteCompleted(bytesTransferred, 0);
            }
        }
    }

    void TcpAsioServerTransport::SessionState::setTransportFilters(
        const std::vector<FilterPtr> &filters)
    {

        mTransportFilters.assign(filters.begin(), filters.end());
        connectFilters(mTransportFilters);
        if (!mTransportFilters.empty())
        {
            mTransportFilters.front()->setPreFilter( *mFilterAdapterPtr );
            mTransportFilters.back()->setPostFilter( *mFilterAdapterPtr );
        }
    }

    const std::vector<FilterPtr> &TcpAsioServerTransport::SessionState::getTransportFilters()
    {
        return mTransportFilters;
    }

    void TcpAsioServerTransport::SessionState::invokeAsyncRead()
    {
        RCF2_TRACE("");

        ByteBuffer byteBuffer(
            getReadByteBuffer(),
            getReadByteBuffer().getLength()-mReadBufferRemaining);

        mTransportFilters.empty() ?
            read(byteBuffer, mReadBufferRemaining) :
            mTransportFilters.front()->read(byteBuffer, mReadBufferRemaining);
    }

    void TcpAsioServerTransport::SessionState::invokeAsyncWrite()
    {
        RCF2_TRACE("");

        // TODO: thread-local
        std::vector<RCF::ByteBuffer> byteBuffers;
        sliceByteBuffers(
            byteBuffers,
            mWriteByteBuffers,
            lengthByteBuffers(mWriteByteBuffers)-mWriteBufferRemaining);

        mTransportFilters.empty() ?
            write(byteBuffers) :
            mTransportFilters.front()->write(byteBuffers);

    }

    void TcpAsioServerTransport::SessionState::invokeAsyncAccept()
    {
        RCF2_TRACE("");

        mState = SessionState::Accepting;

        mTransport.mAcceptorPtr->async_accept(
            *mSocketPtr,
            boost::bind(
                &SessionState::onAccept,
                shared_from_this(),
                boost::asio::placeholders::error));
    }

    void TcpAsioServerTransport::SessionState::onAccept(const boost::asio::error& error)
    {
        RCF2_TRACE("");

        if (!error)
        {
            // save the remote address in the SessionState object
            boost::asio::ip::tcp::endpoint endpoint = mSocketPtr->remote_endpoint();
            sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(endpoint.port());
            addr.sin_addr.s_addr = htonl(endpoint.address().to_v4().to_ulong());
            mIpAddress = IpAddress(addr);
            mState = SessionState::WritingData;

            // create a new SessionState, and do an accept on that
            mTransport.createSessionState()->invokeAsyncAccept();

            // set current RCF session
            SetCurrentSessionGuard guard(mSessionPtr);

            if (mTransport.isClientAddrAllowed(addr))
            {
                // start things rolling by faking a completed write operation
                onReadWrite(0, boost::asio::error(0));
            }
        }
        else if (
            error == boost::asio::error::connection_aborted ||
            error == boost::asio::error::operation_aborted)
        {
            invokeAsyncAccept();
        }
    }

    void onError(boost::asio::error &error1, const boost::asio::error &error2)
    {
        error1 = error2;
    }

    void TcpAsioServerTransport::SessionState::onReadWrite(size_t bytesTransferred, const boost::asio::error& error)
    {
        RCF_ASSERT(!error);
        RCF_ASSERT(!mReflecting);
        {
            switch(mState)
            {
            case SessionState::ReadingDataCount:
            case SessionState::ReadingData:

                RCF_ASSERT(
                    bytesTransferred <= mReadBufferRemaining)
                    (bytesTransferred)(mReadBufferRemaining);

                mReadBufferRemaining -= bytesTransferred;
                if (mReadBufferRemaining > 0)
                {
                    invokeAsyncRead();
                }
                else
                {
                    RCF_ASSERT(mReadBufferRemaining == 0)(mReadBufferRemaining);
                    if (mState == SessionState::ReadingDataCount)
                    {
                        std::vector<char> &readBuffer = getReadBuffer();
                        RCF_ASSERT(readBuffer.size() == 4)(readBuffer.size());
                        unsigned int packetLength = * (unsigned int *) (&readBuffer[0]);
                        networkToMachineOrder(&packetLength, 4, 1);
                        if (packetLength <= mTransport.getMaxMessageLength())
                        {
                            readBuffer.resize(packetLength);
                            mReadBufferRemaining = packetLength;
                            mState = SessionState::ReadingData;
                            invokeAsyncRead();
                        }
                        else
                        {
                            // TODO: improve this a bit

                            boost::shared_ptr<std::vector<char> > vecPtr(
                                new std::vector<char>(4+1+1+4));

                            std::size_t pos = 4;
                            RCF::encodeInt(Descriptor_Error, *vecPtr, pos);
                            RCF::encodeInt(0, *vecPtr, pos);
                            RCF::encodeInt(RcfError_ServerMessageLength, *vecPtr, pos);

                            *(boost::uint32_t *)(&vecPtr->front()) =
                                static_cast<boost::uint32_t>(pos-4);

                            char *buffer = &vecPtr->front();
                            std::size_t bufferLen = pos;

                            boost::asio::error err;
                            boost::asio::write(
                                *mSocketPtr,
                                boost::asio::buffer(buffer, bufferLen),
                                boost::asio::transfer_all(),
                                boost::bind(
                                    &onError,
                                    boost::ref(err),
                                    boost::asio::placeholders::error));

                            mSocketPtr->shutdown(boost::asio::socket_base::shutdown_send);
                        }

                    }
                    else if (mState == SessionState::ReadingData)
                    {
                        mState = SessionState::Ready;
                        mTransport.getSessionManager().onReadCompleted(getSessionPtr());

                        if (mTransport.mInterrupt)
                        {
                            mTransport.mInterrupt = false;
                            mTransport.mDemuxerPtr->interrupt();
                        }
                    }
                }
                break;


            case SessionState::WritingData:

                RCF_ASSERT(
                    bytesTransferred <= mWriteBufferRemaining)
                    (bytesTransferred)(mWriteBufferRemaining);

                mWriteBufferRemaining -= bytesTransferred;
                if (mWriteBufferRemaining > 0)
                {
                    invokeAsyncWrite();
                }
                else
                {
                    mState = SessionState::Ready;
                    mTransport.getSessionManager().onWriteCompleted(getSessionPtr());

                    if (mTransport.mInterrupt)
                    {
                        mTransport.mInterrupt = false;
                        mTransport.mDemuxerPtr->interrupt();
                    }
                }
                break;

            default:
                RCF_ASSERT(0);
            }
        }
    }

    void TcpAsioServerTransport::SessionState::onReflectedReadWrite(
        const boost::asio::error& error,
        size_t bytesTransferred)
    {
        RCF2_TRACE("");

        RCF_ASSERT(
            mState == SessionState::ReadingData ||
            mState == SessionState::ReadingDataCount ||
            mState == SessionState::WritingData)
            (mState);

        // TODO: whether sync is needed, depends on boost::asio implementation
        RCF_ASSERT(mSynchronized);
        RCF_ASSERT(mReflecting);
        RCF_ASSERT(mMutexAutoPtr.get());

        if (mState == SessionState::WritingData)
        {
            mState = SessionState::ReadingData;
            std::vector<char> &readBuffer = getReadBuffer();
            readBuffer.resize(8*1024);

            char *buffer = &readBuffer[0];
            std::size_t bufferLen = readBuffer.size();

            Lock lock(*mMutexAutoPtr);
            if (!mClosed)
            {
                mSocketPtr->async_receive(
                    boost::asio::buffer(buffer, bufferLen),
                    0,
                    boost::bind(
                        &SessionState::onReadCompletion,
                        shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
            }
        }
        else if (
            mState == SessionState::ReadingData ||
            mState == SessionState::ReadingDataCount)
        {
            mState = SessionState::WritingData;
            std::vector<char> &readBuffer = getReadBuffer();

            char *buffer = (char *) &readBuffer[0];
            std::size_t bufferLen = bytesTransferred;

            // mReflecteePtr will be nulled in onWriteCompletion()
            // (otherwise we could easily end up with a cycle)
            RCF_ASSERT(!mReflecteePtr);
            mReflecteePtr = mReflecteeWeakPtr.lock();
            if (mReflecteePtr)
            {
                RCF_ASSERT(mReflecteePtr->mSynchronized);
                RCF_ASSERT(mReflecteePtr->mReflecting);
                RCF_ASSERT(mReflecteePtr->mMutexAutoPtr.get());

                Lock lock(*mReflecteePtr->mMutexAutoPtr);
                if (!mReflecteePtr->mClosed)
                {
                    // TODO: if this can throw, then we need a scope_guard
                    // to reset mReflecteePtr
                    boost::asio::async_write(
                        *(mReflecteePtr->mSocketPtr),
                        boost::asio::buffer(buffer, bufferLen),
                        boost::bind(
                            &SessionState::onWriteCompletion,
                            shared_from_this(),
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
                }
            }
        }
    }

    // TcpAsioServerTransport

    TcpAsioServerTransport::SessionStatePtr TcpAsioServerTransport::createSessionState()
    {
        RCF2_TRACE("");

        SessionStatePtr sessionStatePtr( new SessionState(*this, mDemuxerPtr) );
        SessionPtr sessionPtr( getSessionManager().createSession() );
        TcpAsioProactorPtr tcpAsioProactorPtr( new TcpAsioProactor(*this) );
        sessionPtr->setProactorPtr( ProactorPtr(tcpAsioProactorPtr) );
        tcpAsioProactorPtr->mSessionStateWeakPtr = sessionStatePtr;
        sessionStatePtr->setSessionPtr(sessionPtr);
        return sessionStatePtr;
    }

    int TcpAsioServerTransport::getPort() const
    {
        return mPort;
    }

    // I_ServerTransportEx implementation

    ClientTransportAutoPtr TcpAsioServerTransport::createClientTransport(const I_Endpoint &endpoint)
    {
        RCF2_TRACE("");

        const TcpEndpoint &tcpEndpoint = dynamic_cast<const TcpEndpoint &>(endpoint);
        ClientTransportAutoPtr clientTransportAutoPtr(
            new TcpClientTransport(tcpEndpoint.getIp(), tcpEndpoint.getPort()));
        return clientTransportAutoPtr;
    }

    SessionPtr TcpAsioServerTransport::createServerSession(ClientTransportAutoPtr clientTransportAutoPtr)
    {
        RCF2_TRACE("");

        TcpClientTransport *pTcpClientTransport =
            dynamic_cast<TcpClientTransport *>(clientTransportAutoPtr.get());

        if (pTcpClientTransport == NULL)
        {
            RCF_THROW(
                Exception("incompatible client transport"))
                (pTcpClientTransport)(typeid(*clientTransportAutoPtr));
        }

        TcpClientTransport &tcpClientTransport = *pTcpClientTransport;
        SessionStatePtr sessionStatePtr(createSessionState());
        SessionPtr sessionPtr(sessionStatePtr->getSessionPtr());

        // TODO: exception safety
        sessionStatePtr->mSocketPtr->assign(
            boost::asio::ip::tcp::v4(),
            tcpClientTransport.releaseFd());

        sessionStatePtr->mState = SessionState::WritingData;
        sessionStatePtr->onReadWrite(0, boost::asio::error(0));
        return sessionPtr;
    }

    ClientTransportAutoPtr TcpAsioServerTransport::createClientTransport(SessionPtr sessionPtr)
    {
        RCF2_TRACE("");

        ProactorPtr proactorPtr(sessionPtr->getProactorPtr());
        TcpAsioProactorPtr tcpAsioProactorPtr(boost::dynamic_pointer_cast<TcpAsioProactor>(proactorPtr));
        SessionStatePtr sessionStatePtr(tcpAsioProactorPtr->mSessionStateWeakPtr.lock());
        sessionStatePtr->mMutexAutoPtr.reset(new Mutex());
        sessionStatePtr->mSynchronized = true;
        sessionStatePtr->mDemuxerPtr = mDemuxerPtr;

        int fd = sessionStatePtr->mSocketPtr->native();
        std::auto_ptr<TcpClientTransport> tcpClientTransport(
            new TcpClientTransport(fd));

        boost::asio::ip::tcp::endpoint endpoint = sessionStatePtr->mSocketPtr->remote_endpoint();
        sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(endpoint.port());
        addr.sin_addr.s_addr = htonl(endpoint.address().to_v4().to_ulong());
        tcpClientTransport->setRemoteAddr(addr);

        tcpClientTransport->setCloseFunctor( boost::bind(
            &TcpAsioServerTransport::forceCloseSession,
            this,
            SessionStateWeakPtr(sessionStatePtr),
            sessionStatePtr->getSocketPtr()));

        return ClientTransportAutoPtr(tcpClientTransport.release());
    }

    bool TcpAsioServerTransport::reflect(const SessionPtr &sessionPtr1, const SessionPtr &sessionPtr2)
    {
        RCF2_TRACE("");

        ProactorPtr proactorPtr1(sessionPtr1->getProactorPtr());
        TcpAsioProactorPtr tcpAsioProactorPtr1(boost::dynamic_pointer_cast<TcpAsioProactor>(proactorPtr1));
        SessionStatePtr sessionStatePtr1(tcpAsioProactorPtr1->mSessionStateWeakPtr.lock());

        ProactorPtr proactorPtr2(sessionPtr2->getProactorPtr());
        TcpAsioProactorPtr tcpAsioProactorPtr2(boost::dynamic_pointer_cast<TcpAsioProactor>(proactorPtr2));
        SessionStatePtr sessionStatePtr2(tcpAsioProactorPtr2->mSessionStateWeakPtr.lock());

        sessionStatePtr1->mReflecteeWeakPtr = sessionStatePtr2;
        sessionStatePtr1->mMutexAutoPtr.reset(new Mutex());
        sessionStatePtr1->mSynchronized = true;

        sessionStatePtr2->mReflecteeWeakPtr = sessionStatePtr1;
        sessionStatePtr2->mMutexAutoPtr.reset(new Mutex());
        sessionStatePtr2->mSynchronized = true;

        sessionStatePtr1->mReflecting = true;
        sessionStatePtr2->mReflecting = true;


        return true;
    }

    bool TcpAsioServerTransport::isConnected(const SessionPtr &sessionPtr)
    {
        ProactorPtr proactorPtr(sessionPtr->getProactorPtr());
        TcpAsioProactorPtr tcpAsioProactorPtr( boost::dynamic_pointer_cast<TcpAsioProactor>(proactorPtr));
        SessionStatePtr sessionStatePtr( tcpAsioProactorPtr->mSessionStateWeakPtr.lock() );
        return sessionStatePtr.get() && isFdConnected(sessionStatePtr->mSocketPtr->native());
    }

    // I_Service implementation

    void TcpAsioServerTransport::open()
    {
        RCF2_TRACE("");

        mInterrupt = false;
        mStopFlag = false;
        mDemuxerPtr.reset( new Demuxer );
        mCycleTimerPtr.reset( new DeadlineTimer(*mDemuxerPtr) );

        if (mPort >= 0)
        {

            //mAcceptorPtr = SocketAcceptorPtr( new SocketAcceptor(
            //    *mDemuxerPtr,
            //    boost::asio::ip::tcp::endpoint(
            //        boost::asio::ip::address::from_string(getNetworkInterface()),
            //        static_cast<unsigned short>(mPort))));

            boost::asio::ip::tcp::endpoint endpoint(
                boost::asio::ip::address::from_string(getNetworkInterface()),
                static_cast<unsigned short>(mPort));

            mAcceptorPtr = SocketAcceptorPtr( new SocketAcceptor(*mDemuxerPtr));
            mAcceptorPtr->open(endpoint.protocol());
            mAcceptorPtr->set_option(boost::asio::socket_base::reuse_address(true));
            mAcceptorPtr->bind(endpoint);
            mAcceptorPtr->listen();

            // retrieve the port number, if it's generated by the system
            if (mPort == 0)
            {
                sockaddr_in addr = {0};
                Platform::OS::BsdSockets::socklen_t nameLen = sizeof(addr);
                int fd = mAcceptorPtr->native();
                int ret = getsockname(fd, (sockaddr *) &addr, &nameLen);
                if (ret < 0)
                {
                    int err = Platform::OS::BsdSockets::GetLastError();
                    RCF_THROW(Exception(
                        RcfError_Socket, err, RcfSubsystem_Os, "getsockname() failed"))
                        (fd)(mPort)(getNetworkInterface())(ret);
                }
                mPort = ntohs(addr.sin_port);
            }

        }
    }

    // Thread-safe, forces closure of the session, regardless of mOwnFd
    void TcpAsioServerTransport::SessionState::forceClose()
    {
        RCF2_TRACE("");

        Lock lock(*mMutexAutoPtr);
        if (!mClosed)
        {
            mSocketPtr->close();
            mDemuxerPtr.reset();
            mClosed = true;
        }
    }

    void TcpAsioServerTransport::forceCloseSession(
        SessionStateWeakPtr sessionStateWeakPtr,
        SocketPtr socketPtr)
    {
        SessionStatePtr sessionStatePtr(sessionStateWeakPtr.lock());
        if (sessionStatePtr)
        {
            sessionStatePtr->forceClose();
        }
    }

    void TcpAsioServerTransport::close()
    {
        RCF2_TRACE("");

        mAcceptorPtr.reset();
        mCycleTimerPtr.reset();
        mDemuxerPtr.reset();
    }

    bool TcpAsioServerTransport::cycle(
        int timeoutMs,
        const volatile bool &stopFlag,
        bool returnEarly)
    {
        RCF2_TRACE("");

        //RCF_UNUSED_VARIABLE(stopFlag);
        stopFlag == stopFlag;
        RCF_ASSERT(timeoutMs >= -1)(timeoutMs);

        mInterrupt = returnEarly;
        if (timeoutMs != -1)
        {
            mCycleTimerPtr->cancel();
            mCycleTimerPtr->expires_from_now(boost::posix_time::milliseconds(timeoutMs)); // replacing timeoutMs with 1000 makes Test_Minimal work with cw...
            mCycleTimerPtr->async_wait( boost::bind(&TcpAsioServerTransport::stopCycle, this, boost::asio::placeholders::error));
        }

        mDemuxerPtr->reset();
        mDemuxerPtr->run();

        return false;
    }

    void TcpAsioServerTransport::stopCycle(const boost::asio::error &error)
    {
        RCF2_TRACE("");

        if (!error)
        {
            mDemuxerPtr->interrupt();
        }
    }

    void TcpAsioServerTransport::stop()
    {
        mDemuxerPtr->interrupt();
    }

    void TcpAsioServerTransport::onServiceAdded(RcfServer &server)
    {
        setServer(server);
        WriteLock writeLock( getTaskEntriesMutex() );
        getTaskEntries().clear();
        getTaskEntries().push_back(
            TaskEntry(
            boost::bind(&TcpAsioServerTransport::cycle, this, _1, _2, _3),
            boost::bind(&TcpAsioServerTransport::stop, this),
            "RCF asio server"));
    }

    void TcpAsioServerTransport::onServiceRemoved(RcfServer &)
    {}

    void TcpAsioServerTransport::onServerOpen(RcfServer &)
    {
        open();
    }

    void TcpAsioServerTransport::onServerClose(RcfServer &)
    {
        close();
    }

    void TcpAsioServerTransport::onServerStart(RcfServer &)
    {
        mStopFlag = false;
        if (mAcceptorPtr)
        {
            createSessionState()->invokeAsyncAccept();
        }
    }

    void TcpAsioServerTransport::onServerStop(RcfServer &)
    {
    }

    void TcpAsioServerTransport::setServer(RcfServer &server)
    {
        pServer = &server;
    }

    RcfServer &TcpAsioServerTransport::getServer()
    {
        return *pServer;
    }

    I_SessionManager &TcpAsioServerTransport::getSessionManager()
    {
        return *pServer;
    }

    TcpAsioServerTransport::TcpAsioServerTransport(int port) :
        mDemuxerPtr(),
        mReadWriteMutexPtr( new ReadWriteMutex(ReaderPriority) ),
        mPort(port),
        mAcceptorPtr(),
        mCycleTimerPtr(),
        mInterrupt(RCF_DEFAULT_INIT),
        mStopFlag(RCF_DEFAULT_INIT),
        pServer(RCF_DEFAULT_INIT)
    {}

    ServerTransportPtr TcpAsioServerTransport::clone()
    {
        return ServerTransportPtr(new TcpAsioServerTransport(mPort));
    }

} // namespace RCF
