
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_TCPSERVERTRANSPORT_HPP
#define INCLUDE_RCF_TCPSERVERTRANSPORT_HPP

#include <map>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <RCF/ServerTransport.hpp>
#include <RCF/Service.hpp>
#include <RCF/IpAddress.hpp>
#include <RCF/ThreadLibrary.hpp>

namespace RCF {

    class TcpServerTransport;

    typedef boost::shared_ptr<TcpServerTransport> TcpServerTransportPtr;

    class TcpServerTransport : public I_ServerTransport, public I_Service
    {

    public:
        TcpServerTransport(int port = 0);
        ServerTransportPtr  clone() { return ServerTransportPtr(new TcpServerTransport(port)); }
        void                setSessionManager(I_SessionManager &sessionManager);
        I_SessionManager &  getSessionManager();
        void                setPort(int port);
        int                 getPort();
        void                setNetworkInterface(const std::string &networkInterface);
        std::string         getNetworkInterface();
        void                setMaxPendingConnectionCount(unsigned int maxPendingConnectionCount);
        unsigned int        getMaxPendingConnectionCount();
        bool                isClientIpAllowed(sockaddr_in &addr);

    private:

        typedef int             Fd;

        class SessionState
        {
        public:
            SessionState(Fd fd);

            enum State
            {
                Ready,
                ReadingDataCount,
                ReadingData,
                WritingData,
            };

            State               state;
            std::vector<char>   readBuffer;
            std::size_t         readBufferRemaining;
            std::vector<char>   writeBuffer;
            std::size_t         writeBufferRemaining;
            Fd                  fd;
        };

        typedef boost::shared_ptr<SessionState>     SessionStatePtr;

        class TcpProactor : public I_Proactor
        {
        public:
            TcpProactor(TcpServerTransport &transport, SessionStatePtr sessionStatePtr);
            void                    postRead();
            void                    postWrite();
            void                    postClose();
            std::vector<char> &     getWriteBuffer();
            std::size_t             getWriteOffset();
            std::vector<char> &     getReadBuffer();
            std::size_t             getReadOffset();
            I_ServerTransport &     getServerTransport();
            const I_RemoteAddress & getRemoteAddress();
            void                    setTransportFilters(const std::vector<FilterPtr> &filters);
            const std::vector<FilterPtr> &getTransportFilters();

            ByteBuffer getReadByteBuffer() { return ByteBuffer(); }
            void postWrite(const std::vector<ByteBuffer> &byteBuffers) { RCF_UNUSED_VARIABLE(byteBuffers);}

        private:
            TcpServerTransport &    transport;
            SessionStatePtr         sessionStatePtr;
        };

        friend class TcpServerTransport::TcpProactor;


    private:
        void                open();
        void                close();
        Fd                  hash(Fd fd);
        void                createSession(Fd fd);
        int                 readSession(Fd fd);
        int                 writeSession(Fd fd);
        bool                closeSession(Fd fd);
        void                cycleRead(int timeoutMs);
        void                cycleWrite();
        void                cycleClose();
        void                cycleAccept();
        void                cycle(int timeoutMs, const volatile bool &stopFlag);
        void                postClose(SessionStatePtr sessionStatePtr);
        void                postWrite(SessionStatePtr sessionStatePtr);
        void                postRead(SessionStatePtr sessionStatePtr);
       
        bool                cycleTransportAndServer(RcfServer &server, int timeoutMs, const volatile bool &stopFlag);
        void                onServiceAdded(RcfServer &server);
        void                onServiceRemoved(RcfServer &server);
        void                onServerOpen(RcfServer &server);
        void                onServerClose(RcfServer &server);
        void                onServerStart(RcfServer &server);
        void                onServerStop(RcfServer &server);

        SessionState &      getSessionState(Fd fd);
        SessionStatePtr     getSessionStatePtr(Fd fd);
        I_Session &         getSession(Fd fd);
        SessionPtr          getSessionPtr(Fd fd);

    private:
        typedef int                                                             PortNumber;
        typedef int                                                             FdHash;
        const unsigned int                                                      FdPartitionCount;
        typedef std::map<FdHash, std::pair<SessionStatePtr, SessionPtr> >       SessionMap;
        typedef std::pair<boost::shared_ptr<Mutex>, SessionMap >                SynchronizedSessionMap;
        typedef std::vector<SynchronizedSessionMap>                             SessionMaps;

        SessionMaps                 sessionMaps;
        I_SessionManager *          pSessionManager;
        std::string                 acceptorInterface;
        PortNumber                  acceptorPort;
        Fd                          acceptorFd;
        volatile bool               mStopFlag;
        Mutex                       fdsReadyMutex;
        std::vector<Fd>             fdsReady;
        Mutex                       fdsToBeReadMutex;
        std::vector<Fd>             fdsToBeRead;
        Mutex                       fdsToBeWrittenMutex;
        std::vector<Fd>             fdsToBeWritten;
        Mutex                       fdsToBeClosedMutex;
        std::vector<Fd>             fdsToBeClosed;
        Mutex                       selectingMutex;
        bool                        selecting;
       
        ReadWriteMutex              allowedIpsMutex;
        std::vector<std::string>    allowedIps;
        std::vector<u_long>         allowedIpAddrs;

        int                         port;
        std::string                 networkInterface;
        unsigned int                maxPendingConnectionCount;

        // temp vectors, eventually supposed to be thread-specific
        std::vector<Fd>             fdsTemp1;
        std::vector<Fd>             fdsTemp2;
        std::vector<Fd>             fdsTemp3;
    };

} // namespace RCF

#endif // ! INCLUDE_RCF_TCPSERVERTRANSPORT_HPP
