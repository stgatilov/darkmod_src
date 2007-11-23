
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/TcpServerTransport.hpp>

#include <RCF/RcfServer.hpp>


#include <RCF/UsingBsdSockets.hpp>

namespace RCF {

    TcpServerTransport::SessionState::SessionState(Fd fd) :
    	state(Ready),
        fd(fd)
    {}

    TcpServerTransport::TcpProactor::TcpProactor(
        TcpServerTransport &transport,
        SessionStatePtr sessionStatePtr) :
            transport(transport),
            sessionStatePtr(sessionStatePtr)
    {}

    void TcpServerTransport::TcpProactor::postRead()
    {
        transport.postRead(sessionStatePtr);
    }

    void TcpServerTransport::TcpProactor::postWrite()
    {
        transport.postWrite(sessionStatePtr);
    }

    void TcpServerTransport::TcpProactor::postClose()
    {
        transport.postClose(sessionStatePtr);
    }

    std::vector<char> &TcpServerTransport::TcpProactor::getWriteBuffer()
    {
        return sessionStatePtr->writeBuffer;
    }

    std::size_t TcpServerTransport::TcpProactor::getWriteOffset()
    {
        return 4;
    }

    std::vector<char> &TcpServerTransport::TcpProactor::getReadBuffer()
    {
        return sessionStatePtr->readBuffer;
    }

    std::size_t TcpServerTransport::TcpProactor::getReadOffset()
    {
        return 0;
    }

    I_ServerTransport &TcpServerTransport::TcpProactor::getServerTransport()
    {
        return transport;
    }

    const I_RemoteAddress &TcpServerTransport::TcpProactor::getRemoteAddress()
    {
        // TODO
        RCF_ASSERT(0);
        I_RemoteAddress *ptr = NULL;
        return *ptr;
    }

    void TcpServerTransport::TcpProactor::setTransportFilters(const std::vector<FilterPtr> &filters)
    {
        RCF_UNUSED_VARIABLE(filters);
        // TODO
        RCF_ASSERT(0);
    }

    const std::vector<FilterPtr> &TcpServerTransport::TcpProactor::getTransportFilters()
    {
        // TODO
        RCF_ASSERT(0);
        static std::vector<FilterPtr> dummy;
        return dummy;
    }

    TcpServerTransport::TcpServerTransport(int port /*= 0*/) :
    	FdPartitionCount(10),
    	pSessionManager(RCF_DEFAULT_INIT),
    	acceptorFd(-1),
    	mStopFlag(RCF_DEFAULT_INIT),
    	selecting(RCF_DEFAULT_INIT),
    	allowedIpsMutex(WriterPriority),
        port(port),
        networkInterface("0.0.0.0"),
        maxPendingConnectionCount(100)
    {}

    void TcpServerTransport::setSessionManager(I_SessionManager &sessionManager)
    {
        pSessionManager = &sessionManager;
    }

    I_SessionManager &TcpServerTransport::getSessionManager()
    {
        return *pSessionManager;
    }

    void TcpServerTransport::setPort(int port)
    {
        this->port = port;
    }

    int TcpServerTransport::getPort()
    {
        return port;
    }

    void TcpServerTransport::setNetworkInterface(const std::string &networkInterface)
    {
        this->networkInterface = networkInterface;
    }

    std::string TcpServerTransport::getNetworkInterface()
    {
        return networkInterface;
    }

    void TcpServerTransport::setMaxPendingConnectionCount(unsigned int maxPendingConnectionCount)
    {
        this->maxPendingConnectionCount = maxPendingConnectionCount;
    }

    unsigned int TcpServerTransport::getMaxPendingConnectionCount()
    {
        return maxPendingConnectionCount;
    }

    bool TcpServerTransport::isClientIpAllowed(sockaddr_in &addr)
    {
        RCF_UNUSED_VARIABLE(addr);
        return true;
    }

    void TcpServerTransport::open()
    {
        // reset sessionMaps
        sessionMaps.clear();
        for (unsigned int i=0; i<FdPartitionCount; i++)
        {
            MutexPtr mutexPtr(new Mutex);
            sessionMaps.push_back(SynchronizedSessionMap(mutexPtr, SessionMap()));
        }

        // start listening
        if (acceptorFd == -1 && port > 0)
        {
            int ret = 0;
            int err = 0;
            acceptorFd = static_cast<int>(socket(PF_INET, SOCK_STREAM, IPPROTO_TCP));
            if (acceptorFd == -1)
            {
                err = Platform::OS::BsdSockets::GetLastError();
                RCF_THROW(RCF::Exception(
                    RcfError_Socket, err, RcfSubsystem_Os, "socket() failed"))
                    (acceptorFd);
            }
            sockaddr_in serverAddr;
            memset(&serverAddr, 0, sizeof(serverAddr));
            serverAddr.sin_family = AF_INET;
            if (isdigit(networkInterface.at(0)))
            {
                serverAddr.sin_addr.s_addr = inet_addr( networkInterface.c_str() );
            }
            else
            {
                hostent *h = gethostbyname(networkInterface.c_str());
                if (h)
                {
                    serverAddr.sin_addr = * (in_addr *) h->h_addr_list[0];
                }
            }
            serverAddr.sin_port = htons( static_cast<u_short>(port) );
            ret = bind(acceptorFd, (struct sockaddr*) &serverAddr, sizeof(serverAddr));
            if (ret < 0)
            {
                err = Platform::OS::BsdSockets::GetLastError();
                RCF_THROW(RCF::Exception(
                    RcfError_Socket, err, RcfSubsystem_Os, "bind() failed"))
                    (acceptorFd)(port)(networkInterface)(ret);
            }
            ret = listen(acceptorFd, maxPendingConnectionCount);
            if (ret < 0)
            {
                err = Platform::OS::BsdSockets::GetLastError();
                RCF_THROW(RCF::Exception(
                    RcfError_Socket, err, RcfSubsystem_Os, "listen() failed"))
                    (acceptorFd)(ret);
            }
            RCF_ASSERT(acceptorFd != -1)(acceptorFd);

        }

    }

    void TcpServerTransport::close()
    {
        if (acceptorFd != -1)
        {
            int ret = Platform::OS::BsdSockets::closesocket(acceptorFd);
            if (ret != 0)
            {
                RCF_ASSERT(0)(acceptorFd);
            }
        }

        // NB: following will probably result in a core dump if there are any worker threads still going
        sessionMaps.clear();
    }

    TcpServerTransport::Fd TcpServerTransport::hash(Fd fd)
    {
        return fd % FdPartitionCount;
    }

    void TcpServerTransport::createSession(Fd fd)
    {
        RCF_TRACE("")(fd);

        SessionStatePtr sessionStatePtr( new SessionState(fd) );
        sessionStatePtr->state = SessionState::ReadingDataCount;
        sessionStatePtr->readBuffer.clear();
        sessionStatePtr->readBuffer.resize(4);
        sessionStatePtr->readBufferRemaining = 4;
        ProactorPtr proactorPtr( new TcpProactor(*this, sessionStatePtr) );
        SessionPtr sessionPtr = getSessionManager().createSession();
        sessionPtr->setProactorPtr(proactorPtr);
        {
            Mutex &sessionMapMutex = *sessionMaps[ hash(fd) ].first;
            Lock lock(sessionMapMutex); RCF_UNUSED_VARIABLE(lock);
            SessionMap &sessionMap = sessionMaps[ hash(fd) ].second;
           
            RCF_ASSERT(
                sessionMap[fd].first.get() == NULL && sessionMap[fd].second.get() == NULL)
                (sessionMap[fd].first.get())(sessionMap[fd].second.get());

            sessionMap[fd] = std::make_pair(sessionStatePtr, sessionPtr);
        }
    }

    int TcpServerTransport::readSession(Fd fd)
    {
        SessionState &sessionState = getSessionState(fd);
        std::vector<char> &readBuffer = sessionState.readBuffer;
        std::size_t &readBufferRemaining = sessionState.readBufferRemaining;
        std::size_t readBufferSize = static_cast<unsigned int>(readBuffer.size());
       
        RCF_ASSERT(
            readBufferRemaining > 0 &&
            readBufferSize > 0 &&
            readBufferRemaining <= readBufferSize)
            (readBufferSize)(readBufferRemaining);

        int ret = Platform::OS::BsdSockets::recv(fd, &readBuffer[readBufferSize-readBufferRemaining], static_cast<int>(readBufferRemaining), 0);
        if (ret == -1)
        {
            RCF_TRACE("error reading fd")(fd)(Platform::OS::BsdSockets::GetLastError());
            return -1;
        }
        else if (ret == 0)
        {
            return -1;
        }
        else if (ret > 0 && ret < static_cast<int>(readBufferRemaining))
        {
            readBufferRemaining -= ret;
            return 0;
        }
        else if (ret == static_cast<int>(readBufferRemaining))
        {
            // full packet has now been read
            if (sessionState.state == SessionState::ReadingDataCount)
            {
                RCF_ASSERT(sizeof(unsigned int) == 4);
                RCF_ASSERT(readBuffer.size() == 4)(readBuffer.size());
                unsigned int packetLength = * (unsigned int *) (&readBuffer[0]);
                RCF::networkToMachineOrder(&packetLength, 4, 1);
                readBufferRemaining = packetLength;
                readBuffer.resize(readBufferRemaining); // TODO: might throw, need sanity check on parameter
                sessionState.state = SessionState::ReadingData;
                return 0;
            }
            else if (sessionState.state == SessionState::ReadingData)
            {
                readBufferRemaining = 0;
                return 1;
            }
            else
            {
                RCF_ASSERT(0);
                return -1;
            }
        }
        else
        {
            RCF_ASSERT(0);
            return -1;
        }
    }

    int TcpServerTransport::writeSession(Fd fd)
    {
        // write data corresponding to the session
        // return true if all data has been sent

        SessionState &sessionState = getSessionState(fd);
        std::vector<char> &writeBuffer = sessionState.writeBuffer;
        std::size_t &writeBufferRemaining = sessionState.writeBufferRemaining;
        std::size_t writeBufferSize = static_cast<unsigned int>(writeBuffer.size());
       
        RCF_ASSERT(
            writeBufferRemaining > 0 &&
            writeBufferSize > 0 &&
            writeBufferRemaining <= writeBufferSize)
            (writeBufferSize)(writeBufferRemaining);

        int ret = Platform::OS::BsdSockets::send(fd, &writeBuffer[writeBufferSize-writeBufferRemaining], static_cast<int>(writeBufferRemaining), 0);
        if (ret > 0)
        {
            writeBufferRemaining -= ret;
            if (writeBufferRemaining == 0)
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }
        else if (ret == 0)
        {
            return -1;
        }
        else if (ret == -1 && Platform::OS::BsdSockets::GetLastError() == Platform::OS::BsdSockets::ERR_EWOULDBLOCK)
        {
            return 0;
        }
        else
        {
            return -1;
        }
    }

    bool TcpServerTransport::closeSession(Fd fd)
    {
        RCF_TRACE("")(fd);

        {
            // TODO: actually remove the element corrresponding to fd, not just blank it?
            Mutex &sessionMapMutex = *sessionMaps[ hash(fd) ].first;
            Lock lock(sessionMapMutex); RCF_UNUSED_VARIABLE(lock);
            SessionMap &sessionMap = sessionMaps[ hash(fd) ].second;
            sessionMap[fd] = std::make_pair(SessionStatePtr(), SessionPtr());
        }

        int ret = Platform::OS::BsdSockets::closesocket(fd);
        int err = Platform::OS::BsdSockets::GetLastError();
        RCF_ASSERT(ret == 0)(err)(fd)(ret);
        return true;
    }

    void TcpServerTransport::cycleRead(int timeoutMs)
    {

        // ****** read *****
        // 1. If fdsToBeRead is not empty
        //      1.1 Pop fd
        //      1.2 Push session to appropriate sessionqueue
        //      1.3 Finished

        // 2. If fdsToBeRead is empty
        //      2.1 Extract fdsRead
        //      2.2 Call select() on the fds
        //      2.3 For each fd, append available data to corresponding sessions
        //      2.4 For each fd with a complete packet, add to fdsToBeRead
        //      2.5 For each fd without a complete packet, add to fdsReady
        //      2.6 If fdsToBeRead not empty, pop an fd and push the session
        //      2.7 Finished

        bool triggerSelect = false;
        {
            Lock lock(fdsToBeReadMutex);
            if (!fdsToBeRead.empty())
            {
                // TODO: exception safety concerning fd?
                Fd fd = fdsToBeRead.back();
                fdsToBeRead.pop_back();
                getSessionManager().onReadCompleted(getSessionPtr(fd));
            }
            else
            {
                Lock lock(selectingMutex);
                if (!selecting)
                {
                    triggerSelect = true;
                    selecting = true;
                }
            }
        }

        if (triggerSelect)
        {

            std::vector<Fd> &ready = fdsTemp1;
            std::vector<Fd> &packetReady = fdsTemp2;
            std::vector<Fd> &packetNotReady = fdsTemp3;

            ready.clear();
            packetReady.clear();
            packetNotReady.clear();

            {
                Lock lock(fdsReadyMutex);
                ready.swap(fdsReady);
            }
            if (!ready.empty())
            {
                fd_set readFds;
                FD_ZERO(&readFds);
                for (unsigned int i=0; i<ready.size(); i++)
                {
                    FD_SET( static_cast<SOCKET>(ready[i]), &readFds);
                }
                timeval tv;
                tv.tv_sec = timeoutMs/1000;
                tv.tv_usec = (timeoutMs%1000)*1000;
                int ret = Platform::OS::BsdSockets::select(FD_SETSIZE, &readFds, NULL, NULL, &tv);
                if (ret == -1)
                {
                    int err = Platform::OS::BsdSockets::GetLastError();
                    RCF_THROW(RCF::Exception(
                        RcfError_Socket, err, RcfSubsystem_Os, "select() failed"));
                }
                else if (ret >= 0) // TODO: optimize when ret == 0
                {
                    for (unsigned int i=0; i<ready.size(); i++)
                    {
                        Fd fd = ready[i];
                        if (FD_ISSET(fd, &readFds))
                        {
                            int ret = readSession(fd); // TODO: read data count _and_ data in one go if possible
                            RCF_ASSERT(ret == 1 || ret == 0 || ret == -1);
                            if (1 == ret)
                            {
                                packetReady.push_back(fd);
                            }
                            else if (0 == ret)
                            {
                                packetNotReady.push_back(fd);
                            }
                            else if (-1 == ret)
                            {
                                closeSession(fd);
                            }
                        }
                        else
                        {
                            packetNotReady.push_back(fd);
                        }
                    }
                    {
                        Lock lock(fdsToBeReadMutex);
                        if (fdsToBeRead.empty())
                        {
                            fdsToBeRead.swap(packetReady);
                        }
                        else
                        {
                            for (unsigned int i=0; i<packetReady.size(); i++)
                            {
                                fdsToBeRead.push_back(packetReady[i]);
                            }
                        }
                    }
                    {
                        Lock lock(fdsReadyMutex);
                        if (fdsReady.empty())
                        {
                            fdsReady.swap(packetNotReady);
                        }
                        else
                        {
                            for (unsigned int i=0; i<packetNotReady.size(); i++)
                            {
                                fdsReady.push_back(packetNotReady[i]);
                            }
                        }
                    }
                }
            }

            // TODO: this needs to be executed, even when exceptions are thrown
            {
                Lock lock(selectingMutex);
                selecting = false;
            }

            // and finally we can dispatch a fd
            {
                Lock lock(fdsToBeReadMutex);
                if (!fdsToBeRead.empty())
                {
                    // TODO: what happens to fd if an exception is thrown within onReadCompleted()?
                    Fd fd = fdsToBeRead.back();
                    fdsToBeRead.pop_back();
                    getSessionManager().onReadCompleted(getSessionPtr(fd));
                    //std::vector<char> &readBuffer = getSessionState(fd).readBuffer;
                    //std::string data(&readBuffer[0], readBuffer.size());
                    //getSessionManager().onReadCompleted(getSession(fd), data);
                }
            }
        }

    }

    void TcpServerTransport::cycleWrite()
    {

        // ****** write *****
        // 1. Extract fdsToBeWritten
        // 2. For each fd that is successfully written, add it to fdsReady
        // 3. For each fd that is not successfully written, add it to fdsToBeWritten

        std::vector<Fd> &toBeWritten = fdsTemp1;
        std::vector<Fd> &writeNotOk = fdsTemp3;

        toBeWritten.clear();
        writeNotOk.clear();

        {
            Lock lock(fdsToBeWrittenMutex);
            toBeWritten.swap(fdsToBeWritten);
        }
        for (unsigned int i=0; i<toBeWritten.size(); i++)
        {
            Fd fd = toBeWritten[i];
            int ret = writeSession(fd);
            RCF_ASSERT(1==ret || 0== ret || -1==ret);
            if (1 == ret)
            {
                // wrote all the data, nothing more to do. when a read is requested, the fd goes to fdsReady.
                getSessionManager().onWriteCompleted(getSessionPtr(fd));
            }
            else if (0 == ret)
            {
                writeNotOk.push_back(fd);                   
            }
            else if (-1 == ret)
            {
                closeSession(fd);
            }

            {
                Lock lock(fdsToBeWrittenMutex);
                if (fdsToBeWritten.empty())
                {
                    fdsToBeWritten.swap(writeNotOk);
                }
                else
                {
                    for (unsigned int i=0; i<writeNotOk.size(); i++)
                    {
                        fdsToBeWritten.push_back(writeNotOk[i]);
                    }
                }
            }
        }
    }

    void TcpServerTransport::cycleClose()
    {
        // TODO: check if fdsToBeClosed is empty

        // ****** close *****
        // 1. Extract fdsToBeClosed
        // 2. Close the fds

        std::vector<Fd> &toBeClosed = fdsTemp1;
        toBeClosed.clear();

        {
            Lock lock(fdsToBeClosedMutex);
            toBeClosed.swap(fdsToBeClosed);
        }
        for (unsigned int i=0; i<toBeClosed.size(); i++)
        {
            Fd fd = toBeClosed[i];
            closeSession(fd);
        }
    }

    void TcpServerTransport::cycleAccept()
    {
        // accept a connection if there is one waiting
        if (acceptorFd != -1)
        {
            fd_set readFds;
            FD_ZERO(&readFds);
            FD_SET( static_cast<SOCKET>(acceptorFd), &readFds);
            timeval tv = {0,0};
            int ret = Platform::OS::BsdSockets::select(acceptorFd+1, &readFds, NULL, NULL, &tv);
            if (ret == 1)
            {
                RCF_ASSERT( FD_ISSET(acceptorFd, &readFds) );
                sockaddr_in addr;
                memset(&addr, 0, sizeof(sockaddr_in));
                int addrSize = sizeof(sockaddr_in);
                Fd fdNew = static_cast<int>( Platform::OS::BsdSockets::accept(acceptorFd, (sockaddr *) &addr, &addrSize) );
                RCF_TRACE( "Accepted new connection" )(fdNew);
                Platform::OS::BsdSockets::setblocking(fdNew, false);
                if (isClientIpAllowed(addr))
                {
                    createSession(fdNew);
                    Lock lock(fdsReadyMutex);
                    fdsReady.push_back(fdNew);
                }
                else
                {
                    std::string ip = ::inet_ntoa( addr.sin_addr );
                    RCF_TRACE("Client ip not allowed")(fdNew)(ip);
                    int ret = Platform::OS::BsdSockets::closesocket(fdNew);
                    RCF_ASSERT(ret == 0)(ret)(fdNew)(Platform::OS::BsdSockets::GetLastError());
                }
            }
        }
    }

    void TcpServerTransport::cycle(int timeoutMs, const volatile bool &stopFlag)
    {
        //RCF_UNUSED_VARIABLE(stopFlag);
    	stopFlag == stopFlag;
        cycleWrite();
        cycleClose();
        cycleAccept();
        cycleRead(timeoutMs);
    }

    void TcpServerTransport::postClose(SessionStatePtr sessionStatePtr)
    {
        // TODO; synchronous close if possible
        Lock lock(fdsToBeClosedMutex);
        fdsToBeClosed.push_back(sessionStatePtr->fd);

        // TODO: fdsToBeClosed, fdsToBeWritten etc, should contain SessionStatePtr's, not fds?
    }

    void TcpServerTransport::postWrite(SessionStatePtr sessionStatePtr)
    {
        // prepend data length and queue the session for a write
        // TODO: synchronous write if possible
        sessionStatePtr->state = SessionState::WritingData;
        sessionStatePtr->writeBufferRemaining = static_cast<unsigned int>(sessionStatePtr->writeBuffer.size());
        RCF_ASSERT(sizeof(unsigned int) == 4);
        RCF_ASSERT(sessionStatePtr->writeBuffer.size() >= 4)(sessionStatePtr->writeBuffer.size());
        *(unsigned int*) &sessionStatePtr->writeBuffer[0] = static_cast<unsigned int>(sessionStatePtr->writeBuffer.size()-4);
        RCF::machineToNetworkOrder(&sessionStatePtr->writeBuffer[0], 4, 1);
        Lock lock(fdsToBeWrittenMutex);
        fdsToBeWritten.push_back(sessionStatePtr->fd);
    }

    void TcpServerTransport::postRead(SessionStatePtr sessionStatePtr)
    {
        // queue the session for a read
        // TODO: synchronous read if possible
        sessionStatePtr->state = SessionState::ReadingDataCount;
        sessionStatePtr->readBuffer.resize(4);
        sessionStatePtr->readBufferRemaining = 4;
        Lock lock(fdsReadyMutex);
        fdsReady.push_back(sessionStatePtr->fd);
    }

    TcpServerTransport::SessionState &TcpServerTransport::getSessionState(Fd fd)
    {
        return *getSessionStatePtr(fd);
    }

    TcpServerTransport::SessionStatePtr TcpServerTransport::getSessionStatePtr(Fd fd)
    {
        Lock lock( *sessionMaps[hash(fd)].first );
        return sessionMaps[hash(fd)].second[fd].first;
    }

    I_Session &TcpServerTransport::getSession(Fd fd)
    {
        return *getSessionPtr(fd);
    }

    SessionPtr TcpServerTransport::getSessionPtr(Fd fd)
    {
        Lock lock( *sessionMaps[hash(fd)].first );
        return sessionMaps[hash(fd)].second[fd].second;
    }

    bool TcpServerTransport::cycleTransportAndServer(RcfServer &server, int timeoutMs, const volatile bool &stopFlag)
    {
        if (!stopFlag && !mStopFlag)
        {
            cycle(timeoutMs/2, stopFlag);
            server.cycleSessions(timeoutMs/2, stopFlag);
        }
        return stopFlag || mStopFlag;
    }

    void TcpServerTransport::onServiceAdded(RcfServer &server)
    {
        setSessionManager(server);
        WriteLock writeLock( getTaskEntriesMutex() );
        getTaskEntries().clear();
        getTaskEntries().push_back(
            TaskEntry(
            boost::bind(&TcpServerTransport::cycleTransportAndServer, this, boost::ref(server), _1, _2),
            StopFunctor(),
            ""));
    }

    void TcpServerTransport::onServiceRemoved(RcfServer &server)
    {
        RCF_UNUSED_VARIABLE(server);
    }

    void TcpServerTransport::onServerOpen(RcfServer &server)
    {
        RCF_UNUSED_VARIABLE(server);
        open();
    }

    void TcpServerTransport::onServerClose(RcfServer &server)
    {
        RCF_UNUSED_VARIABLE(server);
        close();
    }

    void TcpServerTransport::onServerStart(RcfServer &server)
    {
        RCF_UNUSED_VARIABLE(server);
        mStopFlag = false;
    }

    void TcpServerTransport::onServerStop(RcfServer &server)
    {
        RCF_UNUSED_VARIABLE(server);
    }

} // namespace RCF
