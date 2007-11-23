
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_TCPASIOSERVERTRANSPORT_HPP
#define INCLUDE_RCF_TCPASIOSERVERTRANSPORT_HPP

#include <vector>

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <RCF/IpAddress.hpp>
#include <RCF/IpServerTransport.hpp>
#include <RCF/ServerTransport.hpp>
#include <RCF/Service.hpp>
#include <RCF/ThreadLibrary.hpp>

namespace RCF {

    class RcfServer;
    class TcpAsioSynchronizedSocket;

    typedef boost::asio::io_service             Demuxer;
    typedef boost::shared_ptr<Demuxer>          DemuxerPtr;
    typedef boost::asio::ip::tcp::acceptor      SocketAcceptor;
    typedef boost::shared_ptr<SocketAcceptor>   SocketAcceptorPtr;
    typedef boost::asio::deadline_timer         DeadlineTimer;
    typedef boost::shared_ptr<DeadlineTimer>    DeadlineTimerPtr;

    class TcpAsioServerTransport :
        public I_ServerTransport,
        public I_ServerTransportEx,
        public I_IpServerTransport,
        public I_Service
    {
    public:

        TcpAsioServerTransport(int port);
        ServerTransportPtr clone();

        typedef boost::asio::ip::tcp::socket    Socket;
        typedef boost::shared_ptr<Socket>       SocketPtr;

    private:

        typedef boost::weak_ptr<I_Session>              SessionWeakPtr;
        typedef TcpAsioSynchronizedSocket               SynchronizedSocket;
        typedef boost::shared_ptr<SynchronizedSocket>   SynchronizedSocketPtr;

        class TcpAsioProactor;

        class SessionState :
            public boost::enable_shared_from_this<SessionState>,
            boost::noncopyable
        {
        public:

            typedef boost::weak_ptr<SessionState>       SessionStateWeakPtr;
            typedef boost::shared_ptr<SessionState>     SessionStatePtr;

            SessionState(
                TcpAsioServerTransport &transport,
                DemuxerPtr demuxerPtr);

            ~SessionState();

            void            setSessionPtr(SessionPtr sessionPtr)    { mSessionPtr = sessionPtr; }
            SessionPtr      getSessionPtr()                         { return mSessionPtr; }

            typedef TcpAsioServerTransport::SocketPtr SocketPtr;
            void            forceClose();
            SocketPtr       getSocketPtr() { return mSocketPtr; }
            void            invokeAsyncAccept();

        private:

            void            read(const ByteBuffer &byteBuffer, std::size_t bytesRequested);
            void            write(const std::vector<ByteBuffer> &byteBuffers);

            void            onReadCompletion(const boost::asio::error &error, size_t bytesTransferred);
            void            onWriteCompletion(const boost::asio::error &error, size_t bytesTransferred);

            void            setTransportFilters(const std::vector<FilterPtr> &filters);
            const std::vector<FilterPtr> &getTransportFilters();
            void            invokeAsyncRead();
            void            invokeAsyncWrite();
            void            onAccept(const boost::asio::error& error);
            void            onReadWrite(size_t bytesTransferred, const boost::asio::error& error);
            void            onReflectedReadWrite(const boost::asio::error& error, size_t bytesTransferred);

            // TODO: too many friends
            friend class    TcpAsioServerTransport;
            friend class    TcpAsioProactor;
            friend class    FilterAdapter;

            enum State
            {
                Ready,
                Accepting,
                ReadingDataCount,
                ReadingData,
                WritingData
            };

            State                               mState;
            std::size_t                         mReadBufferRemaining;
            std::size_t                         mWriteBufferRemaining;
            SessionPtr                          mSessionPtr;
            std::vector<FilterPtr>              mTransportFilters;
            DemuxerPtr                          mDemuxerPtr;

            SocketPtr                           mSocketPtr;
            IpAddress                           mIpAddress;
            TcpAsioServerTransport &            mTransport;



            std::vector<ByteBuffer>             mWriteByteBuffers;

            std::vector<char> &         getReadBuffer();
            std::vector<char> &         getUniqueReadBuffer();
            ByteBuffer                  getReadByteBuffer() const;

            std::vector<char> &         getReadBufferSecondary();
            std::vector<char> &         getUniqueReadBufferSecondary();
            ByteBuffer                  getReadByteBufferSecondary() const;

            boost::shared_ptr<std::vector<char> > mReadBufferPtr;
            boost::shared_ptr<std::vector<char> > mReadBufferSecondaryPtr;

            ByteBuffer                  mTempByteBuffer;

            FilterPtr                   mFilterAdapterPtr;

            MutexAutoPtr                mMutexAutoPtr;
            bool                        mClosed;
            bool                        mSynchronized;
            SessionStateWeakPtr         mReflecteeWeakPtr;
            SessionStatePtr             mReflecteePtr;
            bool                        mReflecting;
        };

        typedef boost::shared_ptr<SessionState> SessionStatePtr;
        typedef boost::weak_ptr<SessionState> SessionStateWeakPtr;

        class TcpAsioProactor : public I_Proactor
        {
        private:
            // TODO: too many friends
            friend class SessionState;
            friend class TcpAsioServerTransport;

            TcpAsioProactor(TcpAsioServerTransport &tcpAsioServerTransport);

            void                        postRead();
            ByteBuffer                  getReadByteBuffer();
            void                        postWrite(const std::vector<ByteBuffer> &byteBuffers);
            void                        postClose();

            I_ServerTransport &         getServerTransport();
            const I_RemoteAddress &     getRemoteAddress();
            void                        setTransportFilters(const std::vector<FilterPtr> &filters);
            const std::vector<FilterPtr> &getTransportFilters();

            SessionStateWeakPtr         mSessionStateWeakPtr;
            TcpAsioServerTransport &    mTcpAsioServerTransport;
        };

        typedef boost::shared_ptr<TcpAsioProactor> TcpAsioProactorPtr;

        SessionStatePtr createSessionState();
        void forceCloseSession(SessionStateWeakPtr sessionStateWeakPtr, SocketPtr socketPtr);

        // I_IpServerTransport implementation
        int                 getPort() const;

        // I_ServerTransportEx implementation
        ClientTransportAutoPtr          createClientTransport(const I_Endpoint &endpoint);
        SessionPtr                      createServerSession(ClientTransportAutoPtr clientTransportAutoPtr);
        ClientTransportAutoPtr          createClientTransport(SessionPtr sessionPtr);
        bool                            reflect(const SessionPtr &sessionPtr1, const SessionPtr &sessionPtr2);
        bool                            isConnected(const SessionPtr &sessionPtr);

        // I_Service implementation
        void                open();
        void                close();
        bool                cycle(int timeoutMs, const volatile bool &stopFlag, bool returnEarly);
        void                stop();
        void                stopCycle(const boost::asio::error &error);
        void                onServiceAdded(RcfServer &server);
        void                onServiceRemoved(RcfServer &server);
        void                onServerOpen(RcfServer &server);
        void                onServerClose(RcfServer &server);
        void                onServerStart(RcfServer &server);
        void                onServerStop(RcfServer &server);
        void                setServer(RcfServer &server);

        RcfServer &         getServer();
        I_SessionManager &  getSessionManager();

    private:

        friend class        SessionState;
        friend class        FilterAdapter;

        DemuxerPtr          mDemuxerPtr;
        ReadWriteMutexPtr   mReadWriteMutexPtr;
        int                 mPort;
        SocketAcceptorPtr   mAcceptorPtr;
        DeadlineTimerPtr    mCycleTimerPtr;
        bool                mInterrupt;
        volatile bool       mStopFlag;
        RcfServer *         pServer;

    };

} // namespace RCF


#endif // ! INCLUDE_RCF_TCPASIOSERVERTRANSPORT_HPP
