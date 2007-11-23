
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_TIMEDBSDSOCKETS_HPP
#define INCLUDE_RCF_TIMEDBSDSOCKETS_HPP

#include <RCF/ByteBuffer.hpp>
#include <RCF/ClientProgress.hpp>
#include <RCF/UsingBsdSockets.hpp>

namespace RCF {

    class I_PollingFunctor
    {
    public:
        virtual ~I_PollingFunctor() {}
        virtual int operator()(int, int &, bool) = 0;
    };

    // return -2 for timeout, -1 for error, 0 for ready
    int pollSocket(
        unsigned int endTimeMs,
        int fd,
        int &err,
        bool bRead);

    // return -2 for timeout, -1 for error, 0 for ready
    int pollSocketWithProgress(
        const ClientProgressPtr &ClientProgressPtr,
        ClientProgress::Activity activity,
        unsigned int endTimeMs,
        int fd,
        int &err,
        bool bRead);

    //******************************************************
    // nonblocking socket routines

    // returns -2 for timeout, -1 for error, otherwise 0
    int timedConnect(
        I_PollingFunctor &pollingFunctor,
        int &err,
        int fd,
        const sockaddr *addr,
        int addrLen);

    // returns -2 for timeout, -1 for error, otherwise number of bytes sent (> 0)
    int timedSend(
        I_PollingFunctor &pollingFunctor,
        int &err,
        int fd,
        const std::vector<ByteBuffer> &byteBuffers,
        std::size_t maxSendSize,
        int flags);

    // returns -2 for timeout, -1 for error, 0 for peer closure, otherwise size of packet read
    int timedRecv(
        I_PollingFunctor &pollingFunctor,
        int &err,
        int fd,
        const ByteBuffer &byteBuffer,
        std::size_t bytesRequested,
        int flags);

    bool isFdConnected(int fd);

} // namespace RCF


#endif // ! INCLUDE_RCF_TIMEDBSDSOCKETS_HPP
