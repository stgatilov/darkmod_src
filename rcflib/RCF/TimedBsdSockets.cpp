
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/TimedBsdSockets.hpp>

#include <algorithm> //std::min/max

#include <RCF/Tools.hpp>

namespace RCF {

    // return -2 for timeout, -1 for error, 0 for ready
    int pollSocket(unsigned int endTimeMs, int fd, int &err, bool bRead)
    {
        fd_set fdSet;
        FD_ZERO(&fdSet);
        FD_SET( static_cast<SOCKET>(fd), &fdSet);
        unsigned int timeoutMs = generateTimeoutMs(endTimeMs);
        timeval timeout = {0};
        timeout.tv_sec = timeoutMs/1000;
        timeout.tv_usec = 1000*(timeoutMs%1000);
        RCF_ASSERT(timeout.tv_usec >= 0)(timeout.tv_usec);
        int ret = bRead ?
            Platform::OS::BsdSockets::select(fd+1, &fdSet, NULL, NULL, &timeout) :
            Platform::OS::BsdSockets::select(fd+1, NULL, &fdSet, NULL, &timeout);
        err = Platform::OS::BsdSockets::GetLastError();
        switch (ret)
        {
        case 0:  return -2;
        case 1:  return  0;
        default: return -1;
        };
    }

    // return -2 for timeout, -1 for error, 0 for ready
    int pollSocketWithProgress(
        const ClientProgressPtr &ClientProgressPtr,
        ClientProgress::Activity activity,
        unsigned int endTimeMs,
        int fd,
        int &err,
        bool bRead)
    {
        bool bTimer = ClientProgressPtr->mTriggerMask & ClientProgress::Timer ? true : false;
        unsigned int pollingIntervalMs = bTimer ? ClientProgressPtr->mTimerIntervalMs : -1;
        while (true)
        {
            fd_set fdSet;
            FD_ZERO(&fdSet);
            FD_SET( static_cast<SOCKET>(fd), &fdSet);
           
            unsigned int timeoutMs =
                RCF_MIN(generateTimeoutMs(endTimeMs), pollingIntervalMs);

            timeval timeout = {0};
            timeout.tv_sec = timeoutMs/1000;
            timeout.tv_usec = 1000*(timeoutMs%1000);
            RCF_ASSERT(timeout.tv_usec >= 0)(timeout.tv_usec);
           
            int ret = bRead ?
                Platform::OS::BsdSockets::select(fd+1, &fdSet, NULL, NULL, &timeout) :
                Platform::OS::BsdSockets::select(fd+1, NULL, &fdSet, NULL, &timeout);
           
                err = Platform::OS::BsdSockets::GetLastError();
            switch (ret)
            {
            case 0:
                if (generateTimeoutMs(endTimeMs) == 0)
                {
                    return -2;
                }
                else if (bTimer)
                {
                    ClientProgress::Action action = ClientProgress::Continue;

                    ClientProgressPtr->mProgressCallback(
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
                break;

            case 1:
                return 0;
           
            default:
                return -1;
            };
        }
    }
   

    //******************************************************
    // nonblocking socket routines

    // returns -2 for timeout, -1 for error, otherwise 0
    int timedConnect(
        I_PollingFunctor &pollingFunctor,
        int &err,
        int fd,
        const sockaddr *addr,
        int addrLen)
    {
        int ret = Platform::OS::BsdSockets::connect(fd, addr, addrLen);
        err = Platform::OS::BsdSockets::GetLastError();
        if (
            (ret == -1 && err == Platform::OS::BsdSockets::ERR_EWOULDBLOCK) ||
            (ret == -1 && err == Platform::OS::BsdSockets::ERR_EINPROGRESS))
        {
            return pollingFunctor(fd, err, false);
        }
        else if (ret == 0)
        {
            err = 0;
            return 0;
        }
        else
        {
            err = Platform::OS::BsdSockets::GetLastError();
            return -1;
        }
    }

#ifdef BOOST_WINDOWS

    void appendWsabuf(std::vector<WSABUF> &wsabufs, const ByteBuffer &byteBuffer)
    {
        WSABUF wsabuf = {0};
        wsabuf.buf = byteBuffer.getPtr() ;
        wsabuf.len = static_cast<u_long>(byteBuffer.getLength());
        wsabufs.push_back(wsabuf);
    }

#else

    void appendWsabuf(std::vector<WSABUF> &wsabufs, const ByteBuffer &byteBuffer)
    {
        WSABUF wsabuf = {0};
        wsabuf.iov_base = (void *) byteBuffer.getPtr() ;
        wsabuf.iov_len = static_cast<std::size_t>(byteBuffer.getLength());
        wsabufs.push_back(wsabuf);
    }

#endif

    // returns -2 for timeout, -1 for error, otherwise number of bytes sent (> 0)
    int timedSend(
        I_PollingFunctor &pollingFunctor,
        int &err,
        int fd,
        const std::vector<ByteBuffer> &byteBuffers,
        std::size_t maxSendSize,
        int flags)
    {
        RCF_UNUSED_VARIABLE(flags);
        std::size_t bytesRemaining = lengthByteBuffers(byteBuffers);
        std::size_t bytesSent = 0;
        while (true)
        {
            std::size_t bytesToSend = RCF_MIN(bytesRemaining, maxSendSize);

            ThreadLocalCached< std::vector<WSABUF> > tlcWsabufs;
            std::vector<WSABUF> &wsabufs = tlcWsabufs.get();

            //DWORD cbSent = 0;

            forEachByteBuffer(
                boost::bind(&appendWsabuf, boost::ref(wsabufs), _1),
                byteBuffers,
                bytesSent,
                bytesToSend);
/*
            int ret = WSASend(
                fd,
                &wsabufs[0],
                static_cast<DWORD>(wsabufs.size()),
                &cbSent,
                0,
                NULL,
                NULL);

            int myErr = WSAGetLastError();
            if (ret == 0)
            {
                RCF_ASSERT(
                    cbSent <= static_cast<int>(bytesRemaining))
                    (cbSent)(bytesRemaining);

                bytesRemaining -= cbSent;
                bytesSent += cbSent;
                err = 0;
                return static_cast<int>(bytesSent);
            }
            else if (myErr == WSAEWOULDBLOCK)
            {
                // can't get WSA_IO_PENDING here, since the socket isn't overlapped
                ret = pollingFunctor(fd, myErr, false);
                if (ret  != 0)
                {
                    err = myErr;
                    return ret;
                }
            }
            else
            {
                err = myErr;
                return -1;
            }

            // TODO: same as above, but using sendmsg() for non-Windows systems
  */         


            int count = 0;
            int myErr = 0;

#ifdef BOOST_WINDOWS
            {
                DWORD cbSent = 0;
                int ret = WSASend(
                    fd, 
                    &wsabufs[0], 
                    static_cast<DWORD>(wsabufs.size()), 
                    &cbSent, 
                    0, 
                    NULL, 
                    NULL);

                count = (ret == 0) ? cbSent : -1;
                myErr = Platform::OS::BsdSockets::GetLastError();
            }            
#else
            {
                msghdr hdr = {0};
                hdr.msg_iov = &wsabufs[0];
                hdr.msg_iovlen = wsabufs.size();
                count = sendmsg(fd, &hdr, 0);
                myErr = Platform::OS::BsdSockets::GetLastError();
            }
#endif

            //int myErr = WSAGetLastError();
            //int myErr = Platform::OS::BsdSockets::GetLastError()
            //if (ret == 0)
            if (count >= 0)
            {
                RCF_ASSERT(
                    count <= static_cast<int>(bytesRemaining))
                    (count)(bytesRemaining);

                bytesRemaining -= count;//cbSent;
                bytesSent += count;//cbSent;
                err = 0;
                return static_cast<int>(bytesSent);
            }
            //else if (myErr == WSAEWOULDBLOCK)
            else if (myErr == Platform::OS::BsdSockets::ERR_EWOULDBLOCK)
            {
                // can't get WSA_IO_PENDING here, since the socket isn't overlapped
                int ret = pollingFunctor(fd, myErr, false);
                if (ret  != 0)
                {
                    err = myErr;
                    return ret;
                }
            }
            else
            {
                err = myErr;
                return -1;
            }


        }
    }

    // -2 for timeout, -1 for error, 0 for peer closure, otherwise size of packet read
    int timedRecv(
        I_PollingFunctor &pollingFunctor,
        int &err,
        int fd,
        const ByteBuffer &byteBuffer,
        std::size_t bytesRequested,
        int flags)
    {
        RCF_ASSERT(!byteBuffer.isEmpty());
       
        std::size_t bytesToRead =
            RCF_MIN(byteBuffer.getLength(), bytesRequested);

        while (true)
        {
            int ret = Platform::OS::BsdSockets::recv(
                fd,
                byteBuffer.getPtr(),
                static_cast<int>(bytesToRead),
                flags);

            err = Platform::OS::BsdSockets::GetLastError();
            if (ret >= 0)
            {
                err = 0;
                return ret;
            }
            else if (
                ret == -1 &&
                err == Platform::OS::BsdSockets::ERR_EWOULDBLOCK)
            {
                int ret = pollingFunctor(fd, err, true);
                if (ret  != 0)
                {
                    return ret;
                }
            }
            else if (ret == -1)
            {
                err = Platform::OS::BsdSockets::GetLastError();
                return -1;
            }
        }
    }

    bool isFdConnected(int fd)
    {
        bool connected = false;
        if (fd != -1)
        {
            timeval tv = {0,0};
            fd_set readFds;
            FD_ZERO(&readFds);
            FD_SET( static_cast<SOCKET>(fd), &readFds);

            int ret = Platform::OS::BsdSockets::select(
                fd+1,
                &readFds,
                NULL,
                NULL,
                &tv);

            if (ret == 0)
            {
                connected = true;
            }
            else if (ret == 1)
            {
                const int length = 1;
                char buffer[length];

                int ret = Platform::OS::BsdSockets::recv(
                    fd,
                    buffer,
                    length,
                    MSG_PEEK);

                if (ret == -1)
                {
                    ret = Platform::OS::BsdSockets::GetLastError();
                    if (
                        ret != Platform::OS::BsdSockets::ERR_ECONNRESET &&
                        ret != Platform::OS::BsdSockets::ERR_ECONNABORTED &&
                        ret != Platform::OS::BsdSockets::ERR_ECONNREFUSED)
                    {
                        connected = true;
                    }
                }
            }
        }
        return connected;
    }

} // namespace RCF
