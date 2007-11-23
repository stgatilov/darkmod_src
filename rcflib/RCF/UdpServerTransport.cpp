
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/UdpServerTransport.hpp>

#include <RCF/MethodInvocation.hpp>
#include <RCF/RcfServer.hpp>
#include <RCF/ThreadLocalData.hpp>
#include <RCF/Tools.hpp>

namespace RCF {

    UdpServerTransport::UdpServerTransport(int port) :
        mpSessionManager(RCF_DEFAULT_INIT),
        mPort(port),
        mFd(-1),
        mStopFlag(RCF_DEFAULT_INIT),
        mPollingDelayMs(RCF_DEFAULT_INIT)
    {
    }

    ServerTransportPtr UdpServerTransport::clone()
    {
        return ServerTransportPtr( new UdpServerTransport(mPort) );
    }

    void UdpServerTransport::setSessionManager(I_SessionManager &sessionManager)
    {
        mpSessionManager = &sessionManager;
    }

    I_SessionManager &UdpServerTransport::getSessionManager()
    {
        RCF_ASSERT(mpSessionManager);
        return *mpSessionManager;
    }

    void UdpServerTransport::setPort(int port)
    {
        this->mPort = port;
    }

    int UdpServerTransport::getPort() const
    {
        return mPort;
    }

    void UdpServerTransport::open()
    {
        RCF_TRACE("")(mPort)(getNetworkInterface());

        // create and bind a socket for receiving UDP messages
        if (mFd == -1 && mPort > 0)
        {
            int ret = 0;
            int err = 0;

            // create the socket
            mFd = static_cast<int>(socket(AF_INET, SOCK_DGRAM, 0));
            if (mFd == -1)
            {
                err = Platform::OS::BsdSockets::GetLastError();
                RCF_THROW(Exception(
                    RcfError_Socket, err, RcfSubsystem_Os, "socket() failed"))
                    (mFd);
            }

            // setup the address
            std::string networkInterface = getNetworkInterface();
            sockaddr_in serverAddr;
            memset(&serverAddr, 0, sizeof(serverAddr));
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons( static_cast<u_short>(mPort) );
            if (networkInterface.size() > 0)
            {
                if (isdigit(networkInterface.at(0)))
                {
                    serverAddr.sin_addr.s_addr = inet_addr(
                        networkInterface.c_str());
                }
                else
                {
                    hostent *h = gethostbyname(networkInterface.c_str());
                    if (h)
                    {
                        serverAddr.sin_addr = * (in_addr *) h->h_addr_list[0];
                    }
                }
            }
            else
            {
                serverAddr.sin_addr.s_addr = INADDR_ANY;
            }

            // bind the socket
            ret = bind(mFd, (struct sockaddr*) &serverAddr, sizeof(serverAddr));
            if (ret < 0)
            {
                err = Platform::OS::BsdSockets::GetLastError();
                RCF_THROW(Exception(
                    RcfError_Socket, err, RcfSubsystem_Os, "bind() failed"))
                    (mFd)(mPort)(networkInterface)(ret);
            }
            RCF_ASSERT( mFd != -1 )(mFd);

            // set the socket to nonblocking mode
            Platform::OS::BsdSockets::setblocking(mFd, false);
        }
    }

    void UdpServerTransport::close()
    {
        if (mFd != -1)
        {
            int ret = Platform::OS::BsdSockets::closesocket(mFd);
            int err = Platform::OS::BsdSockets::GetLastError();
            RCF_VERIFY(
                ret == 0,
                Exception(
                    RcfError_SocketClose, err, RcfSubsystem_Os,
                    "closesocket() failed"))(mFd);
            mFd = -1;
        }
    }

    void discardPacket(int fd)
    {
        char buffer[1];
        int len = recvfrom(fd, buffer, 1, 0, NULL, NULL);
        int err = Platform::OS::BsdSockets::GetLastError();
        RCF_VERIFY(
            len == 1 ||
            (len == -1 && err == Platform::OS::BsdSockets::ERR_EMSGSIZE) ||
            (len == -1 && err == Platform::OS::BsdSockets::ERR_ECONNRESET),
            Exception(
                RcfError_Socket,
                err,
                RcfSubsystem_Os,
                "recvfrom() failed"));
    }

    void UdpServerTransport::cycle(
        int timeoutMs,
        const volatile bool &stopFlag)
    {
        // poll the UDP socket for messages, and read a message if one is available

        //RCF_UNUSED_VARIABLE(stopFlag);
    	stopFlag == stopFlag;
        fd_set fdSet;
        FD_ZERO(&fdSet);
        FD_SET( static_cast<SOCKET>(mFd), &fdSet);
        timeval timeout;
        timeout.tv_sec = timeoutMs/1000;
        timeout.tv_usec = 1000*(timeoutMs%1000);

        int ret = Platform::OS::BsdSockets::select(
            mFd+1,
            &fdSet,
            NULL,
            NULL,
            timeoutMs < 0 ? NULL : &timeout);

        int err = Platform::OS::BsdSockets::GetLastError();
        if (ret == 1)
        {
            SessionPtr sessionPtr = getCurrentUdpSessionPtr();
            if (sessionPtr.get() == NULL)
            {
                SessionStatePtr sessionStatePtr(new SessionState());
                ProactorPtr proactorPtr( new UdpProactor(*this, sessionStatePtr));
                sessionPtr = getSessionManager().createSession();
                sessionPtr->setProactorPtr(proactorPtr);
                setCurrentUdpSessionPtr(sessionPtr);
            }


            SessionStatePtr sessionStatePtr =
                boost::static_pointer_cast<UdpProactor>(
                    sessionPtr->getProactorPtr())->mSessionStatePtr;
            {
                // read a message

                boost::shared_ptr<std::vector<char> > &readVecPtr =
                    sessionStatePtr->mReadVecPtr;

                if (readVecPtr.get() == NULL || !readVecPtr.unique())
                {
                    readVecPtr.reset( new std::vector<char>());
                }
                std::vector<char> &buffer = *readVecPtr;

                sockaddr from;
                int fromlen = sizeof(from);
                memset(&from, 0, sizeof(from));
                buffer.resize(4);

                int len = Platform::OS::BsdSockets::recvfrom(
                    mFd,
                    &buffer[0],
                    4,
                    MSG_PEEK,
                    &from,
                    &fromlen);

                err = Platform::OS::BsdSockets::GetLastError();
                if (isClientAddrAllowed( *(sockaddr_in *) &from ) &&
                    (len == 4 || (len == -1 && err == Platform::OS::BsdSockets::ERR_EMSGSIZE)))
                {
                    sockaddr_in *remoteAddr = reinterpret_cast<sockaddr_in*>(&from);
                    sessionStatePtr->remoteAddress = IpAddress(*remoteAddr);
                    // TODO: byte ordering
                    unsigned int dataLength = *(unsigned int *)(&buffer[0]);
                    if (dataLength <= static_cast<int>(getMaxMessageLength()))
                    {
                        buffer.resize(4+dataLength);
                        memset(&from, 0, sizeof(from));
                        fromlen = sizeof(from);

                        len = Platform::OS::BsdSockets::recvfrom(
                            mFd,
                            &buffer[0],
                            4+dataLength,
                            0,
                            &from,
                            &fromlen);

                        if (static_cast<unsigned int>(len) == 4+dataLength)
                        {
                            getSessionManager().onReadCompleted(sessionPtr);
                        }
                    }
                    else
                    {
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

                        const sockaddr_in &remoteAddr =
                            sessionStatePtr->remoteAddress.getSockAddr();

                        int len = sendto(
                            mFd,
                            buffer,
                            static_cast<int>(bufferLen),
                            0,
                            (const sockaddr *) &remoteAddr,
                            sizeof(remoteAddr));

                        RCF_UNUSED_VARIABLE(len);
                        discardPacket(mFd);
                    }
                }
                else
                {
                    // discard the message (sender ip not allowed, or message format bad)
                    discardPacket(mFd);
                }
            }
        }
        else if (ret == 0)
        {
            RCF_TRACE("server udp poll - no messages")(mFd)(mPort);
        }
        else if (ret == -1)
        {
            RCF_THROW(
                Exception(
                    RcfError_Socket,
                    err,
                    RcfSubsystem_Os,
                    "udp server select() failed "))
                (mFd)(mPort)(err);
        }

    }

    void UdpServerTransport::postWrite(
        const SessionStatePtr &sessionStatePtr,
        const std::vector<ByteBuffer> &byteBuffers)
    {
        // prepend data length and send the data

        boost::shared_ptr<std::vector<char> > &writeVecPtr =
            sessionStatePtr->mWriteVecPtr;

        if (writeVecPtr.get() == NULL || !writeVecPtr.unique())
        {
            writeVecPtr.reset( new std::vector<char>());
        }

        std::vector<char> &writeBuffer = *writeVecPtr;
        unsigned int dataLength = static_cast<unsigned int>(lengthByteBuffers(byteBuffers));
        writeBuffer.resize(4+dataLength);
        *(int *)(&writeBuffer[0]) = dataLength; // TODO: byte ordering
        copyByteBuffers(byteBuffers, &writeBuffer[4]);

        const sockaddr_in &remoteAddr =
            sessionStatePtr->remoteAddress.getSockAddr();
       
        int len = sendto(
            mFd,
            &writeBuffer[0],
            static_cast<int>(writeBuffer.size()),
            0,
            (const sockaddr *) &remoteAddr,
            sizeof(remoteAddr));

        if (len != static_cast<int>(writeBuffer.size()))
        {
            int err = Platform::OS::BsdSockets::GetLastError();
            RCF_THROW(Exception(
                RcfError_Socket, err, RcfSubsystem_Os, "sendto() failed"))
                (mFd)(len)(writeBuffer.size());
        }
    }

    void UdpServerTransport::postRead(const SessionStatePtr &sessionStatePtr)
    {
        RCF_UNUSED_VARIABLE(sessionStatePtr);
    }

    bool UdpServerTransport::cycleTransportAndServer(
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

    void UdpServerTransport::onServiceAdded(RcfServer &server)
    {
        setSessionManager(server);
        WriteLock writeLock( getTaskEntriesMutex() );
        getTaskEntries().clear();

        getTaskEntries().push_back(
            TaskEntry(
                boost::bind(
                    &UdpServerTransport::cycleTransportAndServer,
                    this,
                    boost::ref(server),
                    _1,
                    _2),
                StopFunctor(),
                "RCF udp server"));

        mStopFlag = false;
    }

    void UdpServerTransport::onServiceRemoved(RcfServer &)
    {}

    UdpServerTransport::SessionState::SessionState()
    {}

    void UdpServerTransport::onServerOpen(RcfServer &)
    {
        open();
    }

    void UdpServerTransport::onServerClose(RcfServer &)
    {
        close();
    }

    void UdpServerTransport::onServerStart(RcfServer &)
    {
    }

    void UdpServerTransport::onServerStop(RcfServer &)
    {
        mStopFlag = false;
    }
   
    const I_RemoteAddress &UdpServerTransport::SessionState::getRemoteAddress() const
    {
        return remoteAddress;
    }

    UdpServerTransport::UdpProactor::UdpProactor(
        UdpServerTransport &transport,
        const SessionStatePtr &sessionStatePtr) :
            mTransport(transport),
            mSessionStatePtr(sessionStatePtr)
    {}

    void UdpServerTransport::UdpProactor::postRead()
    {
        mTransport.postRead(mSessionStatePtr);
    }

    void UdpServerTransport::UdpProactor::postWrite(
        const std::vector<ByteBuffer> &byteBuffers)
    {
        mTransport.postWrite(mSessionStatePtr, byteBuffers);
    }

    void UdpServerTransport::UdpProactor::postClose()
    {}

    ByteBuffer UdpServerTransport::UdpProactor::getReadByteBuffer()
    {
        return ByteBuffer(
            &mSessionStatePtr->mReadVecPtr->front() + 4,
            mSessionStatePtr->mReadVecPtr->size() - 4,
            4,
            mSessionStatePtr->mReadVecPtr);
    }

    I_ServerTransport &UdpServerTransport::UdpProactor::getServerTransport()
    {
        return mTransport;
    }

    UdpServerTransport::SessionStatePtr UdpServerTransport::UdpProactor::getSessionStatePtr() const
    {
        return mSessionStatePtr;
    }

    const I_RemoteAddress &UdpServerTransport::UdpProactor::getRemoteAddress()
    {
        return mSessionStatePtr->getRemoteAddress();
    }

    void UdpServerTransport::UdpProactor::setTransportFilters(const std::vector<FilterPtr> &)
    {
        RCF_ASSERT(0);
    }

    const std::vector<FilterPtr> &UdpServerTransport::UdpProactor::getTransportFilters()
    {
        RCF_ASSERT(0);
        static std::vector<FilterPtr> dummy;
        return dummy;
    }

} // namespace RCF
