
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_TCPCLIENTTRANSPORT_HPP
#define INCLUDE_RCF_TCPCLIENTTRANSPORT_HPP

#include <RCF/AsyncFilter.hpp>
#include <RCF/ByteOrdering.hpp>
#include <RCF/ClientProgress.hpp>
#include <RCF/ClientTransport.hpp>
#include <RCF/TimedBsdSockets.hpp>
#include <RCF/UsingBsdSockets.hpp>

namespace RCF {

    class TcpClientTransport;
    typedef boost::shared_ptr<TcpClientTransport> TcpClientTransportPtr;
    class TcpClientFilterProxy;

    class TcpClientTransport : public I_ClientTransport, public WithProgressCallback
    {
    public:
        typedef boost::function0<void> CloseFunctor;

        TcpClientTransport(const TcpClientTransport &rhs);
        TcpClientTransport(const std::string &ip, int port);
        TcpClientTransport(const sockaddr_in &remoteAddr);
        TcpClientTransport(int fd);
        ~TcpClientTransport();

        EndpointPtr             getEndpointPtr() const;
        void                    setCloseFunctor(const CloseFunctor &closeFunctor);
        void                    setRemoteAddr(const sockaddr_in &remoteAddr);
        const sockaddr_in &     getRemoteAddr() const;
        void                    close();
        int                     releaseFd();
        int                     getFd() const;
        void                    setMaxSendSize(std::size_t maxSendSize);
        std::size_t             getMaxSendSize();

    private:

        void                    bsdRecv(const ByteBuffer &byteBuffer, std::size_t bytesRequested);
        void                    bsdSend(const std::vector<ByteBuffer> &byteBuffers);
        std::size_t             timedSend(const std::vector<ByteBuffer> &data);
        std::size_t             timedReceive(ByteBuffer &byteBuffer, std::size_t bytesRequested);
        int                     send(const std::vector<ByteBuffer> &data, unsigned int timeoutMs);
        int                     receive(ByteBuffer &byteBuffer, unsigned int timeoutMs);

        void                    setTransportFilters(const std::vector<FilterPtr> &filters);
        void                    getTransportFilters(std::vector<FilterPtr> &filters);
        void                    connectTransportFilters();
        bool                    isConnected();
        void                    connect(unsigned int timeoutMs);
        void                    disconnect(unsigned int timeoutMs);
        ClientTransportAutoPtr  clone() const;
        int                     timedSend(const char *buffer, std::size_t bufferLen);
        int                     timedReceive(char *buffer, std::size_t bufferLen);

        void                    onReadCompleted(const ByteBuffer &byteBuffer, int error);
        void                    onWriteCompleted(std::size_t bytes, int error);

        sockaddr_in             mRemoteAddr;
        std::string             mIp;
        int                     mPort;
        int                     mFd;
        bool                    mOwn;
        std::size_t             mMaxSendSize;
        std::size_t             mBytesTransferred;
        std::size_t             mBytesSent;
        std::size_t             mBytesRead;
        std::size_t             mBytesTotal;
        int                     mError;
        unsigned int            mEndTimeMs;

        CloseFunctor                            mCloseFunctor;
        std::vector<FilterPtr>                  mTransportFilters;
        std::vector<ByteBuffer>                 mByteBuffers;
        std::vector<ByteBuffer>                 mSlicedByteBuffers;
        boost::shared_ptr<std::vector<char> >   mReadBufferPtr; // TODO: rename
        boost::shared_ptr<std::vector<char> >   mReadBuffer2Ptr; // TODO: rename

        friend class TcpClientFilterProxy;
    };

}

#endif // ! INCLUDE_RCF_TCPCLIENTTRANSPORT_HPP
