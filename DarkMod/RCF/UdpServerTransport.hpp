
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_UDPSERVERTRANSPORT_HPP
#define INCLUDE_RCF_UDPSERVERTRANSPORT_HPP

#include <string>
#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include <RCF/IpAddress.hpp>
#include <RCF/Service.hpp>
#include <RCF/ServerTransport.hpp>
#include <RCF/IpServerTransport.hpp>
#include <RCF/UsingBsdSockets.hpp>
#include <RCF/ThreadLibrary.hpp>

namespace RCF {
   
    class UdpServerTransport;

    typedef boost::shared_ptr<UdpServerTransport> UdpServerTransportPtr;

    class UdpServerTransport :
        public I_ServerTransport,
        public I_IpServerTransport,
        public I_Service,
        boost::noncopyable
    {
    public:

        UdpServerTransport(int port = 0);
        ServerTransportPtr clone();
        void setSessionManager(I_SessionManager &sessionManager);
        I_SessionManager &getSessionManager();
        void setPort(int port);
        int getPort() const;
        void open();
        void close();
        void cycle(int timeoutMs, const volatile bool &stopFlag);

        bool cycleTransportAndServer(
            RcfServer &server,
            int timeoutMs,
            const volatile bool &stopFlag);

        // I_Service implementation
    private:
        void onServiceAdded(RcfServer &server);
        void onServiceRemoved(RcfServer &server);
        void onServerOpen(RcfServer &server);
        void onServerClose(RcfServer &server);
        void onServerStart(RcfServer &server);
        void onServerStop(RcfServer &server);

    private:

        I_SessionManager *  mpSessionManager;
        int                 mPort;
        int                 mFd;
        volatile bool       mStopFlag;
        unsigned int        mPollingDelayMs;

        class SessionState;
        typedef boost::shared_ptr<SessionState> SessionStatePtr;

        void postWrite(
            const SessionStatePtr &sessionStatePtr,
            const std::vector<ByteBuffer> &byteBuffers);

        void postRead(
            const SessionStatePtr &sessionStatePtr);

        class SessionState
        {
        public:
            SessionState();

            boost::shared_ptr< std::vector<char> > mReadVecPtr;
            boost::shared_ptr< std::vector<char> > mWriteVecPtr;

            const I_RemoteAddress &getRemoteAddress() const;
            IpAddress remoteAddress;
        };

        class UdpProactor : public I_Proactor
        {
        public:
            UdpProactor(
                UdpServerTransport &transport,
                const SessionStatePtr &sessionStatePtr);

            I_ServerTransport &getServerTransport();
            SessionStatePtr getSessionStatePtr() const;
            const I_RemoteAddress &getRemoteAddress();
            void setTransportFilters(const std::vector<FilterPtr> &filters);
            const std::vector<FilterPtr> &getTransportFilters();
            ByteBuffer getReadByteBuffer();
            void postRead();
            void postWrite(const std::vector<ByteBuffer> &byteBuffers);
            void postClose();

        private:
            friend class UdpServerTransport;
            UdpServerTransport &    mTransport;
            SessionStatePtr         mSessionStatePtr;
        };

        friend class UdpServerTransport::UdpProactor;

    }; // class UdpServerTransport

} // namespace RCF

#endif // ! INCLUDE_RCF_UDPSERVERTRANSPORT_HPP
