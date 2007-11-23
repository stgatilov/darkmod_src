
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_UDPCLIENTTRANSPORT_HPP
#define INCLUDE_RCF_UDPCLIENTTRANSPORT_HPP

#include <RCF/ClientTransport.hpp>
#include <RCF/UsingBsdSockets.hpp>

namespace RCF {

    class UdpClientTransport;

    typedef boost::shared_ptr<UdpClientTransport> UdpClientTransportPtr;
   
    class UdpClientTransport : public I_ClientTransport
    {
    private:
        std::string     mIp;
        int             mPort;
        int             mSock;
        sockaddr_in     mSrcAddr;
        sockaddr_in     mDestAddr;

        boost::shared_ptr< std::vector<char> > mReadVecPtr;
        boost::shared_ptr< std::vector<char> > mWriteVecPtr;

    public:
        UdpClientTransport(const std::string &ip, int port);
        UdpClientTransport(const sockaddr &dest);
        UdpClientTransport(const UdpClientTransport &rhs);
         ~UdpClientTransport();
        ClientTransportAutoPtr clone() const;
        EndpointPtr getEndpointPtr() const;
        void connect(unsigned int timeoutMs);
        void disconnect(unsigned int timeoutMs);
        int send(const std::vector<ByteBuffer> &data, unsigned int timeoutMs);
        int receive(ByteBuffer &byteBuffer, unsigned int timeoutMs);
        void close();
        bool isConnected();
        void setTransportFilters(const std::vector<FilterPtr> &filters);
        void getTransportFilters(std::vector<FilterPtr> &filters);
    };

} // namespace RCF

#endif // ! INCLUDE_RCF_UDPCLIENTTRANSPORT_HPP
