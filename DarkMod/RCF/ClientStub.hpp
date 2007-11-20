
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_CLIENTSTUB_HPP
#define INCLUDE_RCF_CLIENTSTUB_HPP

#include <string>
#include <vector>
#include <memory>

#include <boost/shared_ptr.hpp>

#include <RCF/AsyncFilter.hpp>
#include <RCF/Endpoint.hpp>
#include <RCF/GetInterfaceName.hpp>
#include <RCF/MethodInvocation.hpp>
#include <RCF/Protocol/Protocol.hpp>
#include <RCF/SerializationProtocol.hpp>
#include <RCF/Token.hpp>

#ifdef RCF_USE_SF_SERIALIZATION
#include <SF/shared_ptr.hpp>
#include <SF/SerializeEnum.hpp>
#endif

#ifdef RCF_USE_BOOST_SERIALIZATION
#include <boost/serialization/shared_ptr.hpp>
#endif

namespace RCF {

    class ConnectionResetGuard;

    namespace IDL {

        class DoRequest;

    } // namespace IDL

    /// Indicates whether a client should use one-way or two-way semantics for remote calls.
    enum RemoteCallSemantics
    {
        Oneway,
        Twoway
    };

    void setDefaultConnectTimeoutMs(unsigned int connectTimeoutMs);
    unsigned int getDefaultConnectTimeoutMs();

    void setDefaultRemoteCallTimeoutMs(unsigned int remoteCallTimeoutMs);
    unsigned int getDefaultRemoteCallTimeoutMs();

    class ClientStub;

    typedef boost::shared_ptr<ClientStub> ClientStubPtr;

    class ClientProgress;
    typedef boost::shared_ptr<ClientProgress> ClientProgressPtr;

    class I_ClientTransport;
    typedef std::auto_ptr<I_ClientTransport> ClientTransportAutoPtr;

    /// Manages the client side of communications between server and client.
    class ClientStub
    {
    public:
        /// Constructor.
        ClientStub(const std::string &interfaceName);

        /// Constructor.
        /// \param objectName Name of the binding on the server which the client wants to invoke.
        ClientStub(const std::string &interfaceName, const std::string &objectName);

        /// Copy constructor. NB - transfers ownership of the client transport!
        ClientStub(const ClientStub &rhs);

        /// Assignment operator. NB - transfers ownership of the client transport!
        ClientStub &operator=(const ClientStub &rhs);

        /// Sets the server endpoint to which the client will call.
        /// \param endpoint Server endpoint.
        void setEndpoint(const I_Endpoint &endpoint);

        /// Sets the server endpoint to which the client will call.
        /// \param endpoint Server endpoint.
        void setEndpoint(EndpointPtr endpointPtr);

        /// Returns a copy of the currently set server endpoint.
        /// \return Shared pointer to an endpoint.
        EndpointPtr getEndpoint() const;

        // TODO: a lot of these get/set functions should not be publicly available
        /// Gets the token, if any, that the client is using.
        /// \return Returns a copy of the token the client is using. If none, then it returns a default constructed Token.
        Token getTargetToken() const;

        /// Sets the token that the client should use in its calls to the server.
        /// \param token Token that the client should pass in subsequent calls to the server.
        void setTargetToken(Token token);

        /// Gets the binding name on the server that the client is accessing.
        /// \return Name of the server binding.
        std::string getTargetName() const;

        /// Sets the binding name on the server that the client should access on subsequent calls.
        /// \param serverBindingName Name of the server binding.
        //void setServerBindingName(const std::string &serverBindingName);
        void setTargetName(const std::string &targetName);

        /// Gets the calling semantics that the client is currently using (oneway or twoway).
        /// \return Current calling semantics.
        RemoteCallSemantics getDefaultCallingSemantics() const;

        /// Sets the calling semantics that the client should use in subsequent calls (oneway or twoway).
        /// \param defaultCallingSemantics Calling semantics to be used.
        void setDefaultCallingSemantics(RemoteCallSemantics defaultCallingSemantics);

        /// Sets the serialization protocol.
        /// \param protocol Integer identifier of the desired serialization protocol.
        void setSerializationProtocol(int protocol);

        /// Gets the serialization protocol.
        /// \return Integer identifier of the currently set serialization protocol.
        int getSerializationProtocol() const;

        /// Enables pointer tracking for outbound SF serialization.
        /// \parameter Whether to enable or not.
        void enableSfSerializationPointerTracking(bool enable);

        /// Sets the client transport, releasing the currently configured transport.
        /// \param transport Client transport.
        void setTransport(ClientTransportAutoPtr transport);

        /// Returns a reference to currently configured client transport.
        /// \return Current client transport.
        I_ClientTransport& getTransport();

        /// Releases and returns the currently configured client transport.
        /// \return Auto pointer to client transport.
        ClientTransportAutoPtr releaseTransport();

        // /// Deletes the current client transport, and replaces it with a cloned copy.
        /// Calls reset() on the client transport.
        //void resetTransport();

        /// Attempts to connect the underlying client transport.
        void createTransport(); // TODO: private?
        void verifyTransport();
        void connect();
        void disconnect();

        /// Attempts to determine if the underlying client transport is connected.
        /// \return True if the transport is connected, false otherwise.
        bool isConnected();

        /// Sets the payload filtering sequence.
        /// \param filters Vector of filters, enumerated in the order in which they should be applied to unfiltered data
        void setMessageFilters(const std::vector<FilterPtr> &filters);

        /// Sets the payload filtering sequence.
        /// \param filterPtr Single filter.
        void setMessageFilters(FilterPtr filterPtr);

        /// Sets the transport filtering sequence.
        /// \param filters Vector of filters, enumerated in the order in which they should be applied to unfiltered data
        //void setTransportFilters(const std::vector<FilterPtr> &filters);

        /// Sets the transport filtering sequence.
        /// \param filterPtr Single filter.
        //void setTransportFilters(FilterPtr filterPtr = FilterPtr());

        void requestTransportFilters(const std::vector<FilterPtr> &filters);
        void requestTransportFilters(FilterPtr filterPtr);
        bool queryForTransportFilters(const std::vector<FilterPtr> &filters);
        bool queryForTransportFilters(FilterPtr filterPtr);
        void clearTransportFilters();

        const std::vector<FilterPtr> &getMessageFilters();

        /// Sets the remote call timeout value. By default, the timeout is set to 10 seconds.
        /// \param remoteCallTimeoutMs Timeout value, in milliseconds.
        void setRemoteCallTimeoutMs(unsigned int remoteCallTimeoutMs);

        /// Gets the remote call timeout value.
        /// \return Timeout value, in milliseconds.
        unsigned int getRemoteCallTimeoutMs() const;

        /// Sets the connect timeout value. By default, the timeout is set to xx seconds.
        /// \param connectTimeoutMs Timeout value, in milliseconds.
        void setConnectTimeoutMs(unsigned int connectTimeoutMs);

        /// Gets the connect timeout value.
        /// \return Timeout value, in milliseconds.
        unsigned int getConnectTimeoutMs() const;

        void setAutoReconnect(bool autoReconnect);
        bool getAutoReconnect() const;

        void setAutoVersioning(bool autoVersioning);
        bool getAutoVersioning() const;

        void setRcfRuntimeVersion(int version);
        int getRcfRuntimeVersion() const;

        void setClientProgressPtr(ClientProgressPtr ClientProgressPtr);
        ClientProgressPtr getClientProgressPtr() const;

        void setTries(std::size_t tries);
        std::size_t getTries() const;

        template<typename Archive> void serialize(Archive &ar, const unsigned int)
        {
            ar  & mToken
                & mDefaultCallingSemantics
                & mProtocol
                & mEndpointName
                & mObjectName
                & mInterfaceName
                & mRemoteCallTimeoutMs
                & mAutoReconnect
                & mEndpoint;
        }

        void createRemoteObject(const std::string &objectName = "");
        void deleteRemoteObject();

        void createRemoteSessionObject(const std::string &objectName = "");
        void deleteRemoteSessionObject();

    private:

        Token                   mToken;
        RemoteCallSemantics     mDefaultCallingSemantics;
        int                     mProtocol;
        std::string             mEndpointName;
        std::string             mObjectName;
        std::string             mInterfaceName;

        unsigned int            mRemoteCallTimeoutMs;
        unsigned int            mConnectTimeoutMs;
        bool                    mAutoReconnect;
        bool                    mConnected;
        std::size_t                mTries;

        EndpointPtr             mEndpoint;
        ClientTransportAutoPtr  mTransport;

        //VectorFilter            mTransportFilters;
        VectorFilter            mMessageFilters;

        ClientProgressPtr       mClientProgressPtr;

        bool                    mAutoVersioning;
        int                        mRcfRuntimeVersion;

        friend class IDL::DoRequest;

        // TODO: make these private
    public:
        MethodInvocationRequest mRequest;
        SerializationProtocolIn mIn;
        SerializationProtocolOut mOut;
    };

    // thread specific client stub
    void setCurrentClientStubPtr(ClientStubPtr clientStubPtr);
    ClientStubPtr getCurrentClientStubPtr();


    class CurrentClientStubPtrSentry
    {
    public:
        CurrentClientStubPtrSentry(ClientStubPtr clientStubPtr)
        {
            setCurrentClientStubPtr(clientStubPtr);
        }

        ~CurrentClientStubPtrSentry()
        {
            setCurrentClientStubPtr(ClientStubPtr());
        }
    };

} // namespace RCF

#ifdef RCF_USE_SF_SERIALIZATION

namespace SF {

    SF_CTOR(RCF::ClientStub, RCF::ClientStub("", ""))

} // namespace SF

#endif

#endif // ! INCLUDE_RCF_CLIENTSTUB_HPP
