
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_TCPIOCPSERVERTRANSPORT_HPP
#define INCLUDE_RCF_TCPIOCPSERVERTRANSPORT_HPP

#include <map>
#include <memory>
#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <RCF/AsyncFilter.hpp>
#include <RCF/IpAddress.hpp>
#include <RCF/IpServerTransport.hpp>
#include <RCF/ServerTask.hpp>
#include <RCF/ServerTransport.hpp>
#include <RCF/Service.hpp>
#include <RCF/ThreadLibrary.hpp>

namespace RCF {

    class RcfServer;

    namespace TcpIocp {

        class Iocp;

        class                                           SessionState;
        typedef boost::shared_ptr<SessionState>         SessionStatePtr;
        typedef boost::weak_ptr<SessionState>           SessionStateWeakPtr;
        class                                           ServerTransport;

        typedef int                                     Fd;
        typedef std::vector<ByteBuffer>                 ByteBuffers;
        typedef std::vector<FilterPtr>                  Filters;

        // Iocp

        class Iocp
        {
        public:
            Iocp(int nMaxConcurrency = -1);
            ~Iocp();

            BOOL Create(
                int nMaxConcurrency = 0);

            BOOL AssociateDevice(
                HANDLE hDevice,
                ULONG_PTR CompKey);

            BOOL AssociateSocket(
                SOCKET hSocket,
                ULONG_PTR CompKey);

            BOOL PostStatus(
                ULONG_PTR CompKey,
                DWORD dwNumBytes = 0,
                OVERLAPPED* po = NULL) ;

            BOOL GetStatus(
                ULONG_PTR* pCompKey,
                PDWORD pdwNumBytes,
                OVERLAPPED** ppo,
                DWORD dwMilliseconds = INFINITE);

        private:
            HANDLE m_hIOCP;
        };

        // FilterProxy

        class FilterProxy : public RCF::IdentityFilter
        {
        public:
            FilterProxy(
                SessionState &sessionState,
                Filter &filter,
                bool top);

        private:
            void read(
                const ByteBuffer &byteBuffer,
                std::size_t bytesRequested);

            void write(
                const std::vector<ByteBuffer> &byteBuffers);

            void onReadCompleted(
                const ByteBuffer &byteBuffer,
                int error);

            void onWriteCompleted(
                std::size_t bytesTransferred,
                int error);

            const FilterDescription &getFilterDescription() const
            {
                RCF_ASSERT(0);
                return * (const FilterDescription *) NULL;
            }

            SessionState &                      mSessionState;
            Filter &                            mFilter;
            bool                                mTop;
        };

        // SessionState
        class SessionState :
            public OVERLAPPED,
            public boost::enable_shared_from_this<SessionState>
        {
        public:

            enum State
            {
                Accepting,
                ReadingDataCount,
                ReadingData,
                WritingData,
                Ready
            };

            enum PostState
            {
                Reading,
                Writing
            };

            SessionState(
                ServerTransport &transport,
                Fd fd);

            ~SessionState();

            void setTransportFilters(
                const std::vector<FilterPtr> &filters);

            const std::vector<FilterPtr> &getTransportFilters();

            void clearOverlapped();

            int read(
                ByteBuffer &byteBuffer,
                std::size_t bufferLen);

            int write(
                const std::vector<ByteBuffer> &byteBuffers);

            void onReadWriteCompleted(
                std::size_t bytesTransferred,
                int error);

            void onFilteredReadCompleted(
                const ByteBuffer &byteBuffer,
                int error);

            void onFilteredWriteCompleted(
                std::size_t bytesTransferred,
                int error);

            void onAcceptCompleted();

        private:

            friend class ServerTransport;
            friend class Proactor;
            friend class FilterProxy;

            int                         mError;
            std::vector<WSABUF>         mWsabufs;

            SessionStateWeakPtr         mReflectionSessionStateWeakPtr;

            State                       mState;
            PostState                   mPostState;
            SessionPtr                  mSessionPtr;

            boost::shared_ptr<std::vector<char> > mReadBufferPtr;
            boost::shared_ptr<std::vector<char> > mReadBufferSecondaryPtr;

            ByteBuffer                  mTempByteBuffer;
            std::vector<ByteBuffer>     mWriteByteBuffers;
            std::size_t                 mReadBufferRemaining;
            std::vector<char>           mWriteBuffer;
            std::size_t                 mWriteBufferRemaining;
            const Fd                    mFd;
            std::vector<FilterPtr>      mTransportFilters;
            ServerTransport &           mTransport;
            IpAddress                   mLocalAddress;
            IpAddress                   mRemoteAddress;

            SessionStateWeakPtr         mWeakThisPtr;
            SessionStatePtr             mThisPtr;

            void wsaRecv(
                const ByteBuffer &,
                std::size_t);

            void wsaSend(
                const std::vector<ByteBuffer> &);

            void reflect(
                std::size_t bytesRead);

            std::vector<char> &         getReadBuffer();
            std::vector<char> &         getUniqueReadBuffer();
            ByteBuffer                  getReadByteBuffer() const;

            std::vector<char> &         getReadBufferSecondary();
            std::vector<char> &         getUniqueReadBufferSecondary();
            ByteBuffer                  getReadByteBufferSecondary() const;

        private:
            bool                        mCloseAfterWrite;
            bool                        mReflected;

            bool                        mSynchronized;
            bool                        mOwnFd;
            bool                        mHasBeenClosed;
            MutexPtr                    mMutexPtr;
        };

        // Proactor

        class Proactor : public I_Proactor
        {
        public:
            Proactor(
                ServerTransport &transport,
                const SessionStatePtr &sessionStatePtr);

            void                        postRead();
            void                        postWrite(const ByteBuffers &byteBuffers);
            void                        postClose();
            ByteBuffer                  getReadByteBuffer();
            I_ServerTransport &         getServerTransport();
            SessionState &              getSessionState();
            SessionStatePtr             getSessionStatePtr() const;
            const I_RemoteAddress &     getRemoteAddress();
            void                        setTransportFilters(const Filters &filters);
            const std::vector<FilterPtr> &getTransportFilters();

        private:
            // need to break the cycle SessionState->Session->Proactor->SessionState
            boost::weak_ptr<SessionState>   sessionStatePtr;
            ServerTransport &               transport;
        };


        // ServerTransport

        class ServerTransport :
            public I_ServerTransport,
            public I_ServerTransportEx,
            public I_IpServerTransport,
            public I_Service,
            boost::noncopyable
        {
        private:

            typedef TcpIocp::SessionState               SessionState;
            typedef TcpIocp::Proactor                   Proactor;
            typedef TcpIocp::FilterProxy                FilterProxy;
            typedef TcpIocp::Iocp                       Iocp;
            typedef TcpIocp::SessionStatePtr            SessionStatePtr;
            typedef TcpIocp::SessionStateWeakPtr        SessionStateWeakPtr;
            typedef TcpIocp::Fd                         Fd;

            friend class TcpIocp::SessionState;
            friend class TcpIocp::Proactor;

        public:
            ServerTransport(int port = 0);
            ServerTransport(const std::string &networkInterface, int port = 0);

            ServerTransportPtr clone();

            void open();

            void close();

            void cycle(
                int timeoutMs,
                const volatile bool &stopFlag);

            bool cycleTransportAndServer(
                RcfServer &server,
                int timeoutMs,
                const volatile bool &stopFlag);

            void setPort(
                int port);

            int getPort() const;

            // this is the size going into wsasend()/wsarecv(), not the max message size!
            void setMaxSendRecvSize(
                std::size_t maxSendRecvSize);

            std::size_t getMaxSendRecvSize() const;

            void setMaxPendingConnectionCount(
                std::size_t maxPendingConnectionCount);

            std::size_t getMaxPendingConnectionCount() const;

            void setSessionManager(
                I_SessionManager &sessionManager);

            I_SessionManager &getSessionManager();

        private:

            SessionStatePtr createSession(
                int fd);

            void closeSession(
                const SessionStateWeakPtr &sessionStateWeakPtr,
                int fd = -1);

            void transition(
                const SessionStatePtr &sessionStatePtr);

            void stopAccepts();

            bool cycleAccepts(
                int timeoutMs,
                const volatile bool &stopFlag);

            void generateAccepts();

            void onReadWriteCompleted(
                const SessionStateWeakPtr &sessionStateWeakPtr,
                std::size_t bytesTransferred,
                int error);

            void onFilteredReadCompleted(
                const SessionStateWeakPtr &sessionStateWeakPtr,
                const ByteBuffer &byteBuffer,
                int error);

            void onFilteredWriteCompleted(
                const SessionStateWeakPtr &sessionStateWeakPtr,
                std::size_t bytesTransferred,
                int error);

            void postWrite(
                const SessionStatePtr &sessionStatePtr);

            void postWrite(
                const SessionStatePtr &sessionStatePtr,
                const std::vector<ByteBuffer> &byteBuffers);

            void postRead(
                const SessionStatePtr &sessionStatePtr);

            void flushIocp() const;

            // I_ServerTransportEx implementation
        private:

            ClientTransportAutoPtr createClientTransport(
                const I_Endpoint &endpoint);

            ClientTransportAutoPtr createClientTransport(
                SessionPtr sessionPtr);

            SessionPtr createServerSession(
                ClientTransportAutoPtr clientTransportAutoPtr);

            bool reflect(
                const SessionPtr &sessionPtr1,
                const SessionPtr &sessionPtr2);

            bool reflect(
                const SessionStatePtr &sessionStatePtr1,
                const SessionStatePtr &sessionStatePtr2);

            bool isConnected(const SessionPtr &sessionPtr);

            // I_Service implementation
        private:
            void onServiceAdded(RcfServer &server);
            void onServiceRemoved(RcfServer &server);
            void onServerStart(RcfServer &server);
            void onServerStop(RcfServer &server);
            void onServerOpen(RcfServer &server);
            void onServerClose(RcfServer &server);
            bool mOpen;

            // member variables
        private:

            I_SessionManager *          mpSessionManager;
            std::string                 mAcceptorInterface;
            int                         mAcceptorPort;
            Fd                          mAcceptorFd;
            int                         mPort;
            std::size_t                 mMaxPendingConnectionCount;
            std::size_t                 mMaxSendRecvSize;
            std::auto_ptr<Iocp>         mIocpAutoPtr;

            Mutex                       mQueuedAcceptsMutex;
            Condition                   mQueuedAcceptsCondition;

            // access via InterlockedIncrement()/InterlockedDecrement()
            unsigned int                mQueuedAccepts;

            const unsigned int          mQueuedAcceptsThreshold;
            const unsigned int          mQueuedAcceptsAugment;

            LPFN_ACCEPTEX               mlpfnAcceptEx;
            LPFN_GETACCEPTEXSOCKADDRS   mlpfnGetAcceptExSockAddrs;

            volatile bool               mStopFlag;
        };

    } // namespace TcpIocp

    typedef TcpIocp::ServerTransport                    TcpIocpServerTransport;

    typedef boost::shared_ptr<TcpIocpServerTransport>   TcpIocpServerTransportPtr;

} // namespace RCF

#endif // ! INCLUDE_RCF_TCPIOCPSERVERTRANSPORT_HPP
