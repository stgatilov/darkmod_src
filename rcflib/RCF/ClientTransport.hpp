
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_CLIENTTRANSPORT_HPP
#define INCLUDE_RCF_CLIENTTRANSPORT_HPP

#include <memory>
#include <string>
#include <vector>

#include <RCF/AsyncFilter.hpp>
#include <RCF/ByteBuffer.hpp>

namespace RCF {

    class I_Endpoint;
    typedef boost::shared_ptr<I_Endpoint> EndpointPtr;

    /// Base class for client transports.
    class I_ClientTransport
    {
    public:
        /// Constructor.
        I_ClientTransport();

        /// Virtual destructor.
        virtual ~I_ClientTransport()
        {}

        /// Clones this transport (deep copy).
        /// \return Auto pointer to a new client transport, cloned from this one.
        virtual std::auto_ptr<I_ClientTransport> clone() const = 0;

        /// Obtains the remote endpoint of the client transport.
        /// \return Pointer to endpoint.
        virtual EndpointPtr getEndpointPtr() const = 0;

        /// Resets the client transport's connection
        //virtual void reset() = 0;
       
        /// Sends data over the client transport, under the given timeout.
        /// \param data Data to send.
        /// \param timeoutMs Timeout value, specified in milliseconds.
        /// \return -2 for timeout, -1 for error, 0 for peer closure, 1 for ok.
        //virtual int send(const std::string &data, unsigned int timeoutMs) = 0;

        virtual int send(const std::vector<ByteBuffer> &data, unsigned int timeoutMs) = 0;

        /// Attempts to receive data over the client transport, under the given timeout.
        /// \param data Buffer in which to receive data.
        /// \param timeoutMs Timeout value, specified in milliseconds.
        /// \return -2 for timeout, -1 for error, 0 for peer closure, 1 for ok.
        //virtual int receive(std::string &data, unsigned int timeoutMs) = 0;

        virtual int receive(ByteBuffer &byteBuffer, unsigned int timeoutMs) = 0;

        /// Attempts to determine if the transport is connected, i.e. if send() or receive() can proceed without errors.
        /// \return By default, the return value is true. If it can be determined that the transport is not connected, the return valúe is false.
        virtual bool isConnected() = 0;
        virtual void connect(unsigned int timeoutMs) = 0;
        virtual void disconnect(unsigned int timeoutMs = 0) = 0;

        /// Sets the transport filter sequence of the transport.
        /// \param filters Sequence of filters to install.
        virtual void setTransportFilters(const std::vector<FilterPtr> &filters) = 0;
       
        virtual void getTransportFilters(std::vector<FilterPtr> &filters) = 0;

        /// Sets the maximum message length.
        /// \param maxMessageLength Maximum allowed message length.
        void setMaxMessageLength(std::size_t maxMessageLength);

        /// Gets the maximum message length.
        /// \return Maximum allowed message length.
        std::size_t getMaxMessageLength() const;

    private:
        std::size_t mMaxMessageLength;

    };

    typedef boost::shared_ptr<I_ClientTransport> ClientTransportPtr;

    typedef std::auto_ptr<I_ClientTransport> ClientTransportAutoPtr;

} // namespace RCF

#endif // ! INCLUDE_RCF_CLIENTTRANSPORT_HPP
